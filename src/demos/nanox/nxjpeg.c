/*
 * nxjpeg - Nano-X JPEG viewer for VGA (optimized for low memory and ELKS)
 *
 * Uses PicoJPEG (MCU-by-MCU) and draws to a Nano-X VGA window. 
 * Nano X on VGA supports only 16 colors.
 * 
 * - Always decodes JPEG in "bands" (one MCU row at a time) for color.
 * - For band-based mode, decodes to per-pixel palette indices (0–15),
 *   then optionally smooths using neighboring bands to stabilize colors.
 * - Uses a classical 16-color mapping by closest color in EGA 16 palette.
 * - Optional 4-level grayscale mode (-g) using VGA gray entries:
 *   indices 0, 8, 7, 15.
 * - Optional 8-level grayscale mode (-g -8) using 8 VGA gray entries
 * - Optional MCU-based grayscale renderer with uniform-MCU
 *   batching (-g -m), optimized to draw several identical MCUs together
 * - Logs to /tmp/nxjpeg.log
 *
 * Input JPEG image must be:
 * - baseline (SOF0), non-progressive, Huffman-coded, 8×8 DCT blocks,
 *   no arithmetic coding, no restart markers
 * - image size should be less than VGA 640×480 to fit to screen in Nano X 
 * - on Linux use:
 *   convert input.png -resize 400x400\> -colorspace RGB -strip \
 *           -sampling-factor 1x1 -define jpeg:dct-method=integer \
 *           -quality 85 -interlace none -depth 8 -type truecolor \
 *           -compress JPEG output.jpg
 *
 * Modes:
 *   Color (default):
 *     - Band-based renderer (2 bands, 1 is rendered)
 *     - EGA 16-color mapping
 *     - optional smoothing:
 *         -s0: off
 *         -s1: mild (default)
 *         -s2: medium
 *         -s3: strong
 *
 *   Grayscale (-g):
 *     - 4 or 8 gray colors
 *     	   -default 4-level grayscale with colors (0, 8, 7, 15) 
 *         -8 option sets 8 gray colors (-m -8) improving image quality
 *     - No smoothing (keeps original sharp look)
 *
 *   Grayscale + Per-MCU streaming renderer (-g -m):
 *     - can speed up grayscale rendering
 *     - OR+AND uniform detection + batching
 *     - Non-uniform MCUs drawn per scanline
 *
 * For slow systems (rendering speed fast -> slow):
 *    -g -m (grayscale optimized per MCU rendering)
 *    -g (grayscale per band rendering)
 *    -s0 (color, no smoothing)
 *    -s1 (default for color, smoothing level 1)
 *
 * This program uses code or is inspired by:
 *   - https://github.com/rafael2k/elks-viewer
 *   - https://github.com/richgel999/picojpeg
 * 
 * Author(s): Anton ANDREEV
 * 
 * History:
 *   3/12/2025 version 1.0
 *   7/12/2025 version 1.1 added 8 color gray scaled mode
 */

/*
 * nxjpeg - Nano-X JPEG viewer for VGA (optimized for low memory and ELKS)
 *
 * Uses PicoJPEG (MCU-by-MCU) and draws to a Nano-X VGA window. 
 * Nano X on VGA supports only 16 colors.
 *
 * --- (unchanged header omitted for brevity) ---
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "picojpeg.h"
#include <arch/io.h>   // for outb()
#include <unistd.h>
#include <sys/time.h>

#define MAX_WIDTH   620
#define MAX_HEIGHT  460

/* Max MCU height from picojpeg is 16 (H2V2) */
#define MAX_MCU_H   16
#define MAX_ROWBUF  (MAX_WIDTH * MAX_MCU_H)

/* Logging */
static FILE *logfp = NULL;
#define LOG(fmt, ...) \
    do { if (logfp) { fprintf(logfp, fmt "\n", ##__VA_ARGS__); fflush(logfp);} } while (0)

/* PicoJPEG externs */
extern unsigned char gMCUBufR[256];
extern unsigned char gMCUBufG[256];
extern unsigned char gMCUBufB[256];

/* System palette */
static GR_PALETTE pal;
static GR_COLOR color_from_index[256];

/* Smoothing parameter */
static int smoothing = 1;

/* Grayscale flag (-g) */
static int use_gray = 0;

/* 8-gray-level flag (-8) */
static int use_gray8 = 0;

/* MCU-based grayscale renderer (-g -m) */
static int use_mcu_renderer = 0;

/* EGA 16-color palette (unchanged) */
static const struct { unsigned char r, g, b; } ega16[16] = {
    { 0,   0,   0 }, { 0,   0,   128 }, { 0,   128, 0 },   { 0,   128, 128 },
    { 128, 0,   0 }, { 128, 0,   128 }, { 128, 64,  0 },   { 192, 192, 192 },
    { 128, 128, 128 }, { 0, 0, 255 },  { 0, 255, 0 },      { 0, 255, 255 },
    { 255, 0, 0 },  { 255, 0, 255 },  { 255, 255, 0 },     { 255, 255, 255 }
};

/* JPEG wrapper */
typedef struct { FILE *fp; } JPEG_FILE;

/* Callback for PicoJPEG */
static unsigned char
pjpeg_need_bytes_callback(unsigned char *buf, unsigned char size,
                          unsigned char *actual, void *userdata)
{
    JPEG_FILE *jf = (JPEG_FILE *)userdata;
    size_t n = fread(buf, 1, size, jf->fp);
    if (n == 0 && ferror(jf->fp)) {
        *actual = 0;
        return PJPG_STREAM_READ_ERROR;
    }
    *actual = (unsigned char)n;
    return 0;
}

/* ----------------------------------------------------------- */
/* Grayscale + quantization (for -g)                           */
/* ----------------------------------------------------------- */
static inline unsigned char to_gray(unsigned r, unsigned g, unsigned b)
{
    return (unsigned char)((r*30 + g*59 + b*11) / 100);
}

/* 4-level mapping */
static inline unsigned char quantize_gray4(unsigned char g)
{
    if (g < 64)   return 0;
    if (g < 128)  return 8;
    if (g < 192)  return 7;
    return 15;
}

/* 8-level grayscale quantizer */
static inline unsigned char quantize_gray8(unsigned char g)
{
    if (g < 32)   return 0;    /* black */
    if (g < 80)   return 1;    /* 64 */
    if (g < 112)  return 2;    /* 96 */
    if (g < 144)  return 8;    /* 128 (existing medium gray) */
    if (g < 176)  return 3;    /* 160 */
    if (g < 208)  return 7;    /* 192 (existing light gray) */
    if (g < 240)  return 4;    /* 224 */
    return 15;                 /* white */
}

/* ----------------------------------*/
/* VGA DAC grayscale palette loader  */
/* ----------------------------------*/
static void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    r >>= 2; g >>= 2; b >>= 2;
    outb(index, 0x3C8);
    outb(r,     0x3C9);
    outb(g,     0x3C9);
    outb(b,     0x3C9);
}

static void load_gray8_palette(void)
{
    vga_set_palette(1, 64,  64,  64);
    vga_set_palette(2, 96,  96,  96);
    vga_set_palette(3, 160, 160, 160);
    vga_set_palette(4, 224, 224, 224);
}

/* --------------------------------------------------------------------- */
/* map_rgb_to_index(): supports 3 modes 4 and 16 color gray, 16 color    */
/* --------------------------------------------------------------------- */

static inline unsigned char
map_rgb_to_index(unsigned r, unsigned g, unsigned b)
{
    if (use_gray) {
        unsigned char g8 = to_gray(r, g, b);
        if (use_gray8)
            return quantize_gray8(g8);
        return quantize_gray4(g8);
    }

    int best = 0;
    long bestdist = 0x7fffffffL;

    for (int i = 0; i < 16; i++) {
        int dr = (int)r - (int)ega16[i].r;
        int dg = (int)g - (int)ega16[i].g;
        int db = (int)b - (int)ega16[i].b;
        long dist = (long)dr*dr*3 + (long)dg*dg*6 + (long)db*db*1;
        if (dist < bestdist) { bestdist = dist; best = i; }
    }
    return (unsigned char)best;
}

/* ----------------------------------------------------------- */
/* EmuGrArea for indexed strips                                */
/* ----------------------------------------------------------- */
static void EmuGrArea(GR_WINDOW_ID wid, GR_GC_ID gc,
                      GR_COORD x, GR_COORD y,
                      GR_SIZE w, GR_SIZE h,
                      const unsigned char *buf,
                      GR_SIZE pitch)
{
    if (w == 0 || h == 0)
        return;

    for (int iy = 0; iy < (int)h; iy++) {
        const unsigned char *row = buf + iy * pitch;

        int run_start = 0;
        unsigned char run_idx = row[0];

        for (int ix = 1; ix < (int)w; ix++) {
            if (row[ix] != run_idx) {
                GrSetGCForeground(gc, color_from_index[run_idx]);
                GrLine(wid, gc,
                       x + run_start, y + iy,
                       x + ix - 1,    y + iy);

                run_start = ix;
                run_idx = row[ix];
            }
        }

        GrSetGCForeground(gc, color_from_index[run_idx]);
        GrLine(wid, gc,
               x + run_start,  y + iy,
               x + (int)w - 1, y + iy);
    }
}

static int stream_jpeg_and_draw_band(const char *file,
                                     GR_WINDOW_ID wid,
                                     GR_GC_ID gc);
static int stream_jpeg_and_draw_mcu_gray(const char *file,
                                                GR_WINDOW_ID wid,
                                                GR_GC_ID gc);

/* ----------------------------------------------------------- */
/* Band decoding + neighborhood smoothing                      */
/* ----------------------------------------------------------- */
/*
 * Decode one MCU-row ("band") into a raw band buffer.
 * The buffer holds palette indices produced by map_rgb_to_index().
 *
 * info       - PicoJPEG image info (maintains decode state)
 * imgw/imgh  - cropped image width/height
 * mcu_w/h    - MCU dimensions in pixels
 * blocks_x/y - number of 8x8 blocks in x/y per MCU
 * band_index - which band (0..MCUsPerCol-1) we're decoding
 * dst_band   - output buffer (size at least row_pitch * mcu_h)
 * row_pitch  - number of pixels per row (imgw)
 *
 * Returns 0 on success, or a PicoJPEG error code.
 */
static int
decode_band_row(pjpeg_image_info_t *info,
                unsigned imgw, unsigned imgh,
                unsigned mcu_w, unsigned mcu_h,
                unsigned blocks_x, unsigned blocks_y,
                unsigned band_index,
                unsigned char *dst_band,
                unsigned row_pitch)
{
    unsigned mcus_per_row = info->m_MCUSPerRow;
    unsigned band_py = band_index * mcu_h;
    int rc;

    memset(dst_band, 0, row_pitch * mcu_h);

    for (unsigned mx = 0; mx < mcus_per_row; mx++) {
        rc = pjpeg_decode_mcu();
        if (rc == PJPG_NO_MORE_BLOCKS)
            return 0;
        if (rc)
            return rc;

        unsigned px = mx * mcu_w;

        for (unsigned ly = 0; ly < mcu_h; ly++) {
            unsigned gy = band_py + ly;
            if (gy >= imgh)
                break;

            for (unsigned lx = 0; lx < mcu_w; lx++) {
                unsigned gx = px + lx;
                if (gx >= imgw)
                    break;

                unsigned block_x = lx / 8;
                unsigned block_y = ly / 8;
                if (block_x >= blocks_x) block_x = blocks_x - 1;
                if (block_y >= blocks_y) block_y = blocks_y - 1;

                unsigned bx = lx % 8;
                unsigned by = ly % 8;
                unsigned bindex = block_y * blocks_x + block_x;
                unsigned sindex = bindex * 64 + by * 8 + bx;
                if (sindex >= 256) sindex = 255;

                unsigned char q = map_rgb_to_index(
                    (unsigned)gMCUBufR[sindex],
                    (unsigned)gMCUBufG[sindex],
                    (unsigned)gMCUBufB[sindex]);

                dst_band[ly * row_pitch + gx] = q;
            }
        }
    }
    return 0;
}

/*
 * Postprocess one band using its neighbors:
 *  - prev_sm  : previous band (already smoothed), or NULL for first band
 *  - cur_raw  : raw current band (just decoded)
 *  - next_raw : next band raw (already decoded), or NULL for last band
 *  Output goes into out_band.
 *
 * Smoothing strength is controlled by "smoothing":
 *   0 -> no smoothing (copy cur_raw to out_band)
 *   1 -> mild smoothing
 *   2 -> medium smoothing
 *   3 -> strong smoothing
 *
 * When use_gray is set (-g), smoothing is disabled and cur_raw is copied.
 */
static void smooth_band(unsigned imgw, unsigned band_h,
						unsigned row_pitch,
						const unsigned char *prev_sm,
						const unsigned char *cur_raw,
						const unsigned char *next_raw,
						unsigned char *out_band)
{
    if (smoothing <= 0 || use_gray) {
        memcpy(out_band, cur_raw, row_pitch * band_h);
        return;
    }

    int base_w, neigh_w, band_w, adv;

    if (smoothing == 1) {
        base_w  = 3; neigh_w = 1; band_w = 1; adv = 2;
    } else if (smoothing == 2) {
        base_w  = 2; neigh_w = 2; band_w = 2; adv = 1;
    } else {
        base_w  = 1; neigh_w = 3; band_w = 3; adv = 0;
    }

    for (unsigned y = 0; y < band_h; y++) {
        for (unsigned x = 0; x < imgw; x++) {
            unsigned idx = y * row_pitch + x;
            unsigned char center = cur_raw[idx];

            int counts[16];
            memset(counts, 0, sizeof(counts));

            counts[center] += base_w;

            if (x > 0)
                counts[cur_raw[idx - 1]] += neigh_w;
            if (x + 1 < imgw)
                counts[cur_raw[idx + 1]] += neigh_w;
            if (y > 0)
                counts[cur_raw[idx - row_pitch]] += neigh_w;
            if (y + 1 < band_h)
                counts[cur_raw[idx + row_pitch]] += neigh_w;

            if (prev_sm)
                counts[prev_sm[idx]] += band_w;

            if (next_raw)
                counts[next_raw[idx]] += band_w;

            int best_col = center;
            int best_votes = counts[center];

            for (int c = 0; c < 16; c++) {
                if (counts[c] > best_votes) {
                    best_votes = counts[c];
                    best_col = c;
                }
            }

            if (best_col != center && best_votes < counts[center] + adv)
                best_col = center;

            out_band[idx] = (unsigned char)best_col;
        }
    }
}

/* ----------------------------------------------------------- */
/* Band-based renderer (color or grayscale)                    */
/* ----------------------------------------------------------- */
static int stream_jpeg_and_draw_band(const char *file,
                                     GR_WINDOW_ID wid,
                                     GR_GC_ID gc)
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("Cannot open: %s", file);
        return -1;
    }

    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info, pjpeg_need_bytes_callback, &jf, 0);

    if (rc) {
        LOG("pjpeg_decode_init rc=%d", rc);
        fclose(fp);
        return -1;
    }

    unsigned imgw = (info.m_width  > MAX_WIDTH)  ? MAX_WIDTH  : info.m_width;
    unsigned imgh = (info.m_height > MAX_HEIGHT) ? MAX_HEIGHT : info.m_height;

    unsigned mcu_w = info.m_MCUWidth;
    unsigned mcu_h = info.m_MCUHeight;
    unsigned blocks_x = mcu_w/8 ? mcu_w/8 : 1;
    unsigned blocks_y = mcu_h/8 ? mcu_h/8 : 1;

    unsigned bands = info.m_MCUSPerCol;

    LOG("BAND-mode: JPEG %ux%u  MCU %ux%u  MCUs/row=%u MCUs/col=%u smoothing=%d use_gray=%d use_gray8=%d",
        imgw, imgh, mcu_w, mcu_h, info.m_MCUSPerRow, bands, smoothing, use_gray, use_gray8);

    if (bands == 0) {
        fclose(fp);
        return -1;
    }

    static unsigned char band_prev_sm[MAX_ROWBUF];
    static unsigned char band_raw0[MAX_ROWBUF];
    static unsigned char band_raw1[MAX_ROWBUF];

    unsigned row_pitch = imgw;

	/* Decode first band into band_raw0 */
    rc = decode_band_row(&info, imgw, imgh,
                         mcu_w, mcu_h,
                         blocks_x, blocks_y,
                         0, band_raw0, row_pitch);
    if (rc) {
        LOG("decode_band_row(0) rc=%d", rc);
        fclose(fp);
        return -1;
    }

    if (bands == 1) {

		/* Only one band: smooth (or just copy) using no neighbors. */
        smooth_band(imgw, mcu_h, row_pitch,
                    NULL, band_raw0, NULL, band_prev_sm);

        for (unsigned ly = 0; ly < mcu_h; ly++) {
            unsigned gy = ly;
            if (gy >= imgh) break;
            EmuGrArea(wid, gc,
                      0, gy,
                      imgw, 1,
                      &band_prev_sm[ly * row_pitch],
                      row_pitch);
        }

        fclose(fp);
        LOG("stream_jpeg_and_draw_band: done (1 band)");
        return 0;
    }

	/* Decode second band into band_raw1 */
    rc = decode_band_row(&info, imgw, imgh,
                         mcu_w, mcu_h,
                         blocks_x, blocks_y,
                         1, band_raw1, row_pitch);
    if (rc) {
        LOG("decode_band_row(1) rc=%d", rc);
        fclose(fp);
        return -1;
    }

	/* Process band 0: current = band_raw0, next = band_raw1 */
    smooth_band(imgw, mcu_h, row_pitch,
                NULL, band_raw0, band_raw1, band_prev_sm);
    for (unsigned ly = 0; ly < mcu_h; ly++) {
        unsigned gy = ly;
        if (gy >= imgh) break;
        EmuGrArea(wid, gc,
                  0, gy,
                  imgw, 1,
                  &band_prev_sm[ly * row_pitch],
                  row_pitch);
    }

    int cur_idx = 1;  /* band_raw1 currently holds band 1 */
    int next_idx = 0; /* band_raw0 will be reused for band 2 */

	/* Process remaining bands 1..bands-1 */
    for (unsigned band = 1; band < bands; band++) {

        unsigned char *cur_raw  = (cur_idx ? band_raw1 : band_raw0);
        unsigned char *next_raw = NULL;

		/* Decode next band (if any) into the other raw buffer */
        if (band + 1 < bands) {
            unsigned char *dst = (next_idx ? band_raw1 : band_raw0);
            rc = decode_band_row(&info, imgw, imgh,
                                 mcu_w, mcu_h,
                                 blocks_x, blocks_y,
                                 band + 1, dst, row_pitch);
            if (rc) {
                LOG("decode_band_row(%u) rc=%d", band + 1, rc);
                fclose(fp);
                return -1;
            }
            next_raw = dst;
        }

        unsigned band_py = band * mcu_h;

		/* For the last band, next_raw is NULL. */
        smooth_band(imgw, mcu_h, row_pitch,
                    band_prev_sm, cur_raw, next_raw, band_prev_sm);

        for (unsigned ly = 0; ly < mcu_h; ly++) {
            unsigned gy = band_py + ly;
            if (gy >= imgh) break;
            EmuGrArea(wid, gc,
                      0, gy,
                      imgw, 1,
                      &band_prev_sm[ly * row_pitch],
                      row_pitch);
        }

		/* rotate raw buffers for next iteration */
        int tmp = cur_idx;
        cur_idx = next_idx;
        next_idx = tmp;
    }

    fclose(fp);
    LOG("stream_jpeg_and_draw_band: done (bands)");
    return 0;
}

/* ----------------------------------------------------------- */
/* Per-MCU grayscale renderer with batching (-g -m)            */
/*  - OR+AND uniform detection + batching                      */
/* ----------------------------------------------------------- */
static int stream_jpeg_and_draw_mcu_gray(const char *file,
										 GR_WINDOW_ID wid,
										 GR_GC_ID gc)
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("Cannot open: %s", file);
        return -1;
    }

    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info, pjpeg_need_bytes_callback, &jf, 0);
    if (rc) {
        LOG("pjpeg_decode_init rc=%d", rc);
        fclose(fp);
        return -1;
    }

    unsigned imgw = (info.m_width  > MAX_WIDTH)  ? MAX_WIDTH  : info.m_width;
    unsigned imgh = (info.m_height > MAX_HEIGHT) ? MAX_HEIGHT : info.m_height;

    unsigned mcu_w = info.m_MCUWidth;
    unsigned mcu_h = info.m_MCUHeight;
    unsigned blocks_x = mcu_w/8 ? mcu_w/8 : 1;
    unsigned blocks_y = mcu_h/8 ? mcu_h/8 : 1;

    LOG("MCU-gray-mode: JPEG %ux%u  MCU %ux%u  MCUs/row=%u",
        imgw, imgh, mcu_w, mcu_h, info.m_MCUSPerRow);

	/* small strip buffer for one MCU row */
    #define MCU_MAX_WIDTH 32
    static unsigned char stripbuf[MCU_MAX_WIDTH];
	
	/* full-MCU quantized buffer (max 256 samples) */
    static unsigned char qbuf[256];

    int mcu_index = 0;

	/* batching state: up to 6 uniform MCUs */
    int batch_active = 0;
    unsigned batch_my = 0;
    unsigned batch_px_start = 0;
    unsigned batch_py = 0;
    unsigned batch_mcu_count = 0;
    unsigned char batch_color = 0;
    const unsigned max_batch = 6;

    #define FLUSH_BATCH() \
        do { \
            if (batch_active && batch_mcu_count > 0) { \
                int x1 = batch_px_start; \
                int x2 = x1 + batch_mcu_count * mcu_w - 1; \
                int y1 = batch_py; \
                int y2 = y1 + mcu_h - 1; \
                if (x2 >= (int)imgw) x2 = imgw - 1; \
                if (y2 >= (int)imgh) y2 = imgh - 1; \
                if (x1 <= x2 && y1 <= y2) { \
                    GrSetGCForeground(gc, color_from_index[batch_color]); \
                    for (int yy = y1; yy <= y2; yy++) \
                        GrLine(wid, gc, x1, yy, x2, yy); \
                } \
                batch_active = 0; \
                batch_mcu_count = 0; \
            } \
        } while (0)

    while (1) {
        rc = pjpeg_decode_mcu();
        if (rc == PJPG_NO_MORE_BLOCKS)
            break;
        if (rc) {
            LOG("decode_mcu rc=%d at index=%d", rc, mcu_index);
            FLUSH_BATCH();
            fclose(fp);
            return -1;
        }

        unsigned mx = (unsigned)mcu_index % info.m_MCUSPerRow;
        unsigned my = (unsigned)mcu_index / info.m_MCUSPerRow;
        mcu_index++;

        unsigned px = mx * mcu_w;
        unsigned py = my * mcu_h;

		/* Skip MCUs completely outside cropped image */
        if (py >= imgh) {
            if (batch_active && my != batch_my)
                FLUSH_BATCH();
            continue;
        }
        if (px >= imgw) {
            if (batch_active && my != batch_my)
                FLUSH_BATCH();
            continue;
        }

		/* ---- FULL MCU uniform detection with OR+AND ---- */
		
        int samples = (int)(blocks_x * blocks_y * 64);
        if (samples > 256) samples = 256;

		/* use map_rgb_to_index, which in grayscale mode is quantize_gray4(to_gray) */
        unsigned char first = map_rgb_to_index(
            (unsigned)gMCUBufR[0],
            (unsigned)gMCUBufG[0],
            (unsigned)gMCUBufB[0]);
        qbuf[0] = first;

        unsigned char all_or  = first;
        unsigned char all_and = first;

        for (int si = 1; si < samples; si++) {
            unsigned char q = map_rgb_to_index(
                (unsigned)gMCUBufR[si],
                (unsigned)gMCUBufG[si],
                (unsigned)gMCUBufB[si]);
            qbuf[si] = q;
            all_or  |= q;
            all_and &= q;
        }

        int mcu_uniform = (all_or == all_and);
        unsigned char uniform_color = all_or;

		/* ---- Uniform MCU batching ---- */
		
        if (mcu_uniform) {
            if (!batch_active ||
                my != batch_my ||
                uniform_color != batch_color ||
                px != batch_px_start + batch_mcu_count * mcu_w ||
                batch_mcu_count >= max_batch)
            {
                FLUSH_BATCH();
                batch_active    = 1;
                batch_my        = my;
                batch_px_start  = px;
                batch_py        = py;
                batch_color     = uniform_color;
                batch_mcu_count = 1;
            } else {
                batch_mcu_count++;
            }

            continue;
        }

		/* ---- Non-uniform MCU ---- */
		
        if (batch_active)
            FLUSH_BATCH();

        for (unsigned ly = 0; ly < mcu_h; ly++) {

            unsigned gy = py + ly;
            if (gy >= imgh) break;

            unsigned valid_w = mcu_w;
            if (px + valid_w > imgw)
                valid_w = imgw - px;
            if (valid_w > MCU_MAX_WIDTH)
                valid_w = MCU_MAX_WIDTH;

            for (unsigned lx = 0; lx < valid_w; lx++) {
                unsigned gx = px + lx;
                if (gx >= imgw) break;

                unsigned block_x = lx / 8;
                unsigned block_y = ly / 8;
                if (block_x >= blocks_x) block_x = blocks_x - 1;
                if (block_y >= blocks_y) block_y = blocks_y - 1;

                unsigned bx = lx % 8;
                unsigned by = ly % 8;
                unsigned bindex = block_y * blocks_x + block_x;
                unsigned sindex = bindex * 64 + by * 8 + bx;
                if (sindex >= 256) sindex = 255;

                stripbuf[lx] = qbuf[sindex];
            }

            EmuGrArea(wid, gc,
                      px, gy,
                      valid_w, 1,
                      stripbuf,
                      valid_w);
        }
    }

    FLUSH_BATCH();

    #undef FLUSH_BATCH
    #undef MCU_MAX_WIDTH

    fclose(fp);
    LOG("stream_jpeg_and_draw_mcu_gray: done");
    return 0;
}

/* ----------------------------------------------------------- */
/* Dispatcher: picks renderer based on -g and -m               */
/* ----------------------------------------------------------- */

static int stream_jpeg_and_draw(const char *file,
                                GR_WINDOW_ID wid,
                                GR_GC_ID gc)
{
	/* grayscaled has 2 modes */
    if (use_gray) {
        if (use_mcu_renderer)
            return stream_jpeg_and_draw_mcu_gray(file, wid, gc);
        else
            return stream_jpeg_and_draw_band(file, wid, gc);
    }

	/* color always uses band-based renderer */
    return stream_jpeg_and_draw_band(file, wid, gc);
}

static int use_alt = 0;
static int fixed_colors[5] = { 0, 7, 8, 15, 14 };

/* Timer */
static unsigned long get_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)(tv.tv_sec * 1000UL + tv.tv_usec / 1000UL);
}

/* Palette helpers */
static inline int to_ega(int r, int g, int b)
{
    int rr = (r >> 6) & 3;
    int gg = (g >> 6) & 3;
    int bb = (b >> 6) & 3;
    return (rr << 4) | (gg << 2) | bb;
}

static void VGA_WriteEGAColorRaw(int index, int ega)
{
    inb(0x3DA);
    outb(0x3C0, index & 0x1F);
    outb(0x3C0, ega & 0x3F);
}

static void VGA_WriteEGAColor(int index, int r, int g, int b)
{
    int rr = (r >> 6) & 3;
    int gg = (g >> 6) & 3;
    int bb = (b >> 6) & 3;

    int ega = (rr << 4) | (gg << 2) | bb;

    inb(0x3DA);
    outb(0x3C0, index & 0x1F);
    outb(0x3C0, ega);
    inb(0x3DA);
    outb(0x3C0, 0x20);
}

void CustomGrSetSystemPalette(const GR_PALETTE *pal_in)
{
    if (!pal_in || pal_in->count < 16)
        return;

    for (int i = 0; i < 16; i++) {
        int ega = to_ega(
            pal_in->palette[i].r,
            pal_in->palette[i].g,
            pal_in->palette[i].b
        );
        VGA_WriteEGAColorRaw(i, ega);
    }

    inb(0x3DA);
    outb(0x3C0, 0x20);
}

/* ---------------------------- MAIN ---------------------------- */

int main(int argc, char **argv)
{
    const char *file = NULL;

    logfp = fopen("/tmp/nxjpeg.log", "w");
    LOG("nxjpeg starting");

	/* parse arguments:
     * -sN  -> smoothing level (0..3) for band-based color
     * -g   -> grayscale mode (4-level: 0,8,7,15)
     * -m   -> use per-MCU grayscale when combined with -g
	 * -8   -> switch from 4 gray colors to 8 gray colors
     * other non-dash arg -> JPEG filename
     */
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 's') {
            int v = argv[i][2] ? (argv[i][2] - '0') : 1;
            if (v < 0) v = 0;
            if (v > 3) v = 3;
            smoothing = v;
        } else if (!strcmp(argv[i], "-g")) {
            use_gray = 1;
            LOG("Grayscale mode");
        } else if (!strcmp(argv[i], "-m")) {
            use_mcu_renderer = 1;
            LOG("MCU renderer for grayscale");
        } else if (!strcmp(argv[i], "-e")) {
            use_alt = 1;
            LOG("Palette alternate mode (-e)");
        } else if (!strcmp(argv[i], "-8")) {
            use_gray8 = 1;
            LOG("8-gray mode (-8) enabled");
        } else if (argv[i][0] != '-') {
            file = argv[i];
        }
    }

    LOG("smoothing=%d use_gray=%d use_mcu_renderer=%d use_alt=%d use_gray8=%d",
        smoothing, use_gray, use_mcu_renderer, use_alt, use_gray8);

    if (!file) {
        LOG("No JPEG file supplied.");
        if (logfp) fclose(logfp);
        return 1;
    }

    if (GrOpen() < 0) {
        LOG("GrOpen failed");
        if (logfp) fclose(logfp);
        return 1;
    }

    LOG("Writing VGA palette test");
    VGA_WriteEGAColorRaw(1, 0x3F);
    inb(0x3DA);
    outb(0x3C0, 0x20);

    GR_SCREEN_INFO si;
    GrGetScreenInfo(&si);
    LOG("Screen %dx%d bpp=%d colors=%d",
        si.cols, si.rows, si.bpp, si.ncolors);

    pal.count = 256;
    GrGetSystemPalette(&pal);

    for (int i = 0; i < pal.count; i++) {
        color_from_index[i] = GR_RGB(
            pal.palette[i].r,
            pal.palette[i].g,
            pal.palette[i].b);
    }

    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("Cannot open JPEG for size");
        GrClose();
        if (logfp) fclose(logfp);
        return 1;
    }

    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info, pjpeg_need_bytes_callback, &jf, 0);
    fclose(fp);

    if (rc) {
        LOG("decode_init rc=%d", rc);
        GrClose();
        if (logfp) fclose(logfp);
        return 1;
    }

    GR_SIZE w = (info.m_width  > MAX_WIDTH)  ? MAX_WIDTH  : info.m_width;
    GR_SIZE h = (info.m_height > MAX_HEIGHT) ? MAX_HEIGHT : info.m_height;
    LOG("Window %dx%d", w, h);

    GR_WINDOW_ID wid =
        GrNewWindowEx(GR_WM_PROPS_APPWINDOW,
                      "nxjpeg",
                      GR_ROOT_WINDOW_ID,
                      0, 0, w, h,
                      BLACK);

    GrSelectEvents(wid,
        GR_EVENT_MASK_EXPOSURE |
        GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(wid);

    GR_GC_ID gc = GrNewGC();

    GR_PALETTE pal_base = pal;
    GR_PALETTE pal_alt  = pal;

    if (use_alt && !use_gray) {

        for (int i = 0; i < 16; i++) {
            int is_fixed = 0;
            for (int k = 0; k < 5; k++) {
                if (i == fixed_colors[k]) {
                    is_fixed = 1;
                    break;
                }
            }

            int ega = to_ega(
                pal_base.palette[i].r,
                pal_base.palette[i].g,
                pal_base.palette[i].b
            );

            if (!is_fixed)
                ega ^= 0x3F;

            pal_alt.palette[i].r = ((ega >> 4) & 3) * 85;
            pal_alt.palette[i].g = ((ega >> 2) & 3) * 85;
            pal_alt.palette[i].b = ( ega        & 3) * 85;
        }

        CustomGrSetSystemPalette(&pal_base);
    }

    int pal_state = 0;
    unsigned long last_toggle = get_ms();

    while (1) {

        GR_EVENT ev;
        GrGetNextEventTimeout(&ev, 50);

        if (ev.type == GR_EVENT_TYPE_CLOSE_REQ) {
            LOG("close");
            GrClose();
            if (logfp) fclose(logfp);
            return 0;
        }

        if (ev.type == GR_EVENT_TYPE_EXPOSURE) {
            LOG("exposure -> redraw");

            /* update 8-gray VGA DAC palette at start of exposure */
            if (use_gray && use_gray8) {
                load_gray8_palette();
            }

            stream_jpeg_and_draw(file, wid, gc);

            if (use_alt && !use_gray)
                CustomGrSetSystemPalette(pal_state ? &pal_alt : &pal_base);
        }

        if (use_alt && !use_gray) {

            unsigned long now = get_ms();

            if (now - last_toggle >= 100) {
                last_toggle = now;
                pal_state ^= 1;

                if (pal_state)
                    LOG("Switching to ALT palette");
                else
                    LOG("Switching to BASE palette");

                CustomGrSetSystemPalette(pal_state ? &pal_alt : &pal_base);
            }
        }
    }
}
