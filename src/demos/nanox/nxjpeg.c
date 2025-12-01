/*
 * nxjpeg - Nano-X JPEG viewer for ELKS VGA
 *
 * Uses PicoJPEG (MCU-by-MCU) and draws directly to
 * a Nano-X VGA window.
 * 
 */

/*
 * nxjpeg.c - Grayscale JPEG viewer for ELKS Nano-X (16-color mode)
 *
 * - Always converts JPEG to grayscale
 * - Uses simple horizontal Floyd–Steinberg-style error diffusion
 *   inside each MCU row (safe, no geometry breakage)
 * - Maps grayscale only to 4 Nano-X palette entries:
 *      index 0  = black        (0,0,0)
 *      index 8  = dark gray    (128,128,128)
 *      index 7  = light gray   (192,192,192)
 *      index 15 = white        (255,255,255)
 * - Supports:
 *      -r       PicoJPEG reduce (1/8)
 *      -d n     extra scale /2^n
 *
 * Log file: /tmp/nxjpeg.log
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "picojpeg.h"

#define MAX_WIDTH  640
#define MAX_HEIGHT 480

/* ------------------------------------------------------------------ */
/* Logging                                                            */
/* ------------------------------------------------------------------ */

static FILE *logfp = NULL;
#define LOG(fmt, ...) \
    do { if (logfp) { fprintf(logfp, fmt "\n", ##__VA_ARGS__); fflush(logfp); } } while (0)

/* ------------------------------------------------------------------ */
/* PicoJPEG externs                                                   */
/* ------------------------------------------------------------------ */

extern unsigned char gMCUBufR[256];
extern unsigned char gMCUBufG[256];
extern unsigned char gMCUBufB[256];

/* Options */
static int reduce_flag = 0;
static int scale_shift = 0;

/* Palette from Nano-X */
static GR_PALETTE pal;

/* JPEG I/O wrapper */
typedef struct {
    FILE *fp;
} JPEG_FILE;

/* PicoJPEG callback */
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

/* ------------------------------------------------------------------ */
/* Utility                                                            */
/* ------------------------------------------------------------------ */

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Convert RGB to grayscale (0..255) */
static inline unsigned char to_gray(unsigned char r,
                                    unsigned char g,
                                    unsigned char b)
{
    return (unsigned char)((r * 30 + g * 59 + b * 11) / 100);
}

/*
 * Map grayscale to 4 fixed palette entries:
 *
 *   idx 0  -> black       (0)
 *   idx 8  -> dark gray   (~128)
 *   idx 7  -> light gray  (~192)
 *   idx 15 -> white       (255)
 *
 * We assume your Nano-X palette matches these values.
 */
static unsigned char map_gray_to_palette(unsigned char gray)
{
    /* target brightness for each chosen entry */
    const int pal_index[4] = { 0, 8, 7, 15 };
    const int pal_gray[4]  = { 0, 128, 192, 255 };

    int best = pal_index[0];
    int bestd = 9999;

    for (int i = 0; i < 4; i++) {
        int d = (int)gray - pal_gray[i];
        if (d < 0) d = -d;
        if (d < bestd) {
            bestd = d;
            best = pal_index[i];
        }
    }
    return (unsigned char)best;
}

/* ------------------------------------------------------------------ */
/* Emulated "GrArea" using RLE + GrLine                              */
/* ------------------------------------------------------------------ */

static void EmuGrArea(GR_WINDOW_ID wid, GR_GC_ID gc,
                      GR_COORD x, GR_COORD y,
                      GR_SIZE w, GR_SIZE h,
                      const unsigned char *buf,
                      GR_SIZE pitch)
{
    for (int iy = 0; iy < (int)h; iy++) {
        const unsigned char *row = buf + iy * pitch;

        int run_start = 0;
        unsigned char run_idx = row[0];

        for (int ix = 1; ix < (int)w; ix++) {
            if (row[ix] != run_idx) {
                GrSetGCForeground(gc,
                    GR_RGB(pal.palette[run_idx].r,
                           pal.palette[run_idx].g,
                           pal.palette[run_idx].b));
                GrLine(wid, gc,
                       x + run_start, y + iy,
                       x + ix - 1,    y + iy);
                run_start = ix;
                run_idx = row[ix];
            }
        }

        GrSetGCForeground(gc,
            GR_RGB(pal.palette[run_idx].r,
                   pal.palette[run_idx].g,
                   pal.palette[run_idx].b));
        GrLine(wid, gc,
               x + run_start,  y + iy,
               x + (int)w - 1, y + iy);
    }
}

/* ------------------------------------------------------------------ */
/* Get final JPEG size after -r and -d                               */
/* ------------------------------------------------------------------ */

static int get_final_jpeg_size(const char *file, GR_SIZE *pw, GR_SIZE *ph)
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("get_final_jpeg_size: cannot open %s", file);
        return -1;
    }

    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info,
                               pjpeg_need_bytes_callback,
                               &jf,
                               (unsigned char)reduce_flag);
    fclose(fp);

    if (rc) {
        LOG("get_final_jpeg_size: pjpeg_decode_init rc=%d", rc);
        return -1;
    }

    unsigned w = info.m_width;
    unsigned h = info.m_height;

    if (scale_shift > 0) {
        w >>= scale_shift;
        h >>= scale_shift;
    }

    if (w < 1) w = 1;
    if (h < 1) h = 1;

    *pw = (GR_SIZE)w;
    *ph = (GR_SIZE)h;

    LOG("Final JPEG size after -r/-d: %ux%u", w, h);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Decode and draw MCUs (always grayscale + horizontal FS dithering)  */
/* ------------------------------------------------------------------ */

static int decode_and_draw_mcus(const char *file,
                                GR_WINDOW_ID wid,
                                GR_GC_ID gc,
                                GR_SIZE ww,
                                GR_SIZE wh)
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("decode_and_draw_mcus: cannot open %s", file);
        return -1;
    }

    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info,
                               pjpeg_need_bytes_callback,
                               &jf,
                               (unsigned char)reduce_flag);
    if (rc) {
        LOG("decode_and_draw_mcus: pjpeg_decode_init rc=%d", rc);
        fclose(fp);
        return -1;
    }

    unsigned mcu_w = info.m_MCUWidth;
    unsigned mcu_h = info.m_MCUHeight;
    LOG("MCU: %ux%u  MCUs: %ux%u  image: %ux%u",
        mcu_w, mcu_h,
        info.m_MCUSPerRow, info.m_MCUSPerCol,
        info.m_width, info.m_height);

    unsigned img_w = info.m_width;
    unsigned img_h = info.m_height;

    if (scale_shift > 0) {
        img_w >>= scale_shift;
        img_h >>= scale_shift;
    }

    if (img_w > (unsigned)ww) img_w = (unsigned)ww;
    if (img_h > (unsigned)wh) img_h = (unsigned)wh;
    if (img_w > MAX_WIDTH) img_w = MAX_WIDTH;

    LOG("Drawing area: %ux%u (window %dx%d)", img_w, img_h, ww, wh);

    /* MCU pixel index buffer (palette indices) */
    unsigned char mcu_idx[256]; /* up to 16x16 = 256 pixels */

    /* Per-row error buffer INSIDE each MCU (horizontal error diffusion) */
    int err_line[16]; /* mcu_w <= 16 always for JPEG Baseline */

    for (unsigned my = 0; my < info.m_MCUSPerCol; my++) {
        for (unsigned mx = 0; mx < info.m_MCUSPerRow; mx++) {

            rc = pjpeg_decode_mcu();
            if (rc == PJPG_NO_MORE_BLOCKS) break;
            if (rc) {
                LOG("decode_and_draw_mcus: pjpeg_decode_mcu rc=%d", rc);
                fclose(fp);
                return -1;
            }

            /* Screen origin (sx0, sy0) of this MCU block */
            unsigned sx0 = mx * mcu_w;
            unsigned sy0 = my * mcu_h;

            if (scale_shift > 0) {
                sx0 >>= scale_shift;
                sy0 >>= scale_shift;
            }

            if (sx0 >= img_w || sy0 >= img_h)
                continue;

            /* Nominal block size */
            unsigned sw = mcu_w;
            unsigned sh = mcu_h;

            if (scale_shift > 0) {
                sw >>= scale_shift;
                sh >>= scale_shift;
            }

            if (sx0 + sw > img_w) sw = img_w - sx0;
            if (sy0 + sh > img_h) sh = img_h - sy0;

            if (sw == 0 || sh == 0)
                continue;

            if (sw * sh > sizeof(mcu_idx)) {
                LOG("WARNING: MCU block too large: %ux%u", sw, sh);
                continue;
            }

            memset(mcu_idx, 0, sw * sh);

            /* Fill MCU block with grayscale + local horizontal error diffusion */
            unsigned src_i = 0;

            for (unsigned iy = 0; iy < mcu_h; iy++) {

                /* For each MCU row (iy), reset local error buffer */
                memset(err_line, 0, sizeof(err_line));

                for (unsigned ix = 0; ix < mcu_w; ix++, src_i++) {

                    unsigned gx = mx * mcu_w + ix;
                    unsigned gy = my * mcu_h + iy;

                    if (gx >= info.m_width || gy >= info.m_height)
                        continue;

                    unsigned sx = gx;
                    unsigned sy = gy;

                    if (scale_shift > 0) {
                        sx >>= scale_shift;
                        sy >>= scale_shift;
                    }

                    if (sx < sx0 || sy < sy0) continue;
                    if (sx >= sx0 + sw || sy >= sy0 + sh) continue;

                    unsigned lx = sx - sx0; /* local X inside MCU */
                    unsigned ly = sy - sy0; /* local Y inside MCU */

                    if (lx >= sw || ly >= sh)
                        continue;

                    unsigned di = ly * sw + lx;

                    int r = gMCUBufR[src_i];
                    int g = gMCUBufG[src_i];
                    int b = gMCUBufB[src_i];

                    /* 1) Convert to grayscale */
                    int gray = to_gray((unsigned char)r,
                                       (unsigned char)g,
                                       (unsigned char)b);

                    /* 2) Add horizontal error for this row (local to MCU) */
                    gray = clampi(gray + err_line[lx], 0, 255);

                    /* 3) Map grayscale to 4 gray palette entries */
                    unsigned char idx = map_gray_to_palette((unsigned char)gray);
                    mcu_idx[di] = idx;

                    /* 4) Compute quantization error using actual palette RGB */
                    int pr = pal.palette[idx].r;
                    int pg = pal.palette[idx].g;
                    int pb = pal.palette[idx].b;
                    int pgray = (pr * 30 + pg * 59 + pb * 11) / 100;

                    int e = gray - pgray;

                    /* 5) Simple 1D Floyd–Steinberg horizontally:
                     *       X   7/16
                     *   We only propagate to next pixel in this MCU row
                     */
                    if (lx + 1 < sw) {
                        err_line[lx + 1] += (e * 7) / 16;
                    }
                }
            }

            /* Draw this MCU block */
            EmuGrArea(wid, gc,
                      (GR_COORD)sx0, (GR_COORD)sy0,
                      (GR_SIZE)sw,   (GR_SIZE)sh,
                      mcu_idx,
                      (GR_SIZE)sw);
        }
    }

    fclose(fp);
    LOG("decode_and_draw_mcus: completed");
    return 0;
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
    const char *file = NULL;

    logfp = fopen("/tmp/nxjpeg.log", "w");
    LOG("nxjpeg starting: always grayscale, horizontal FS dithering");

    /* Parse arguments: -r, -d n, filename */
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r")) {
            reduce_flag = 1;
            LOG("Option: -r (PicoJPEG reduce)");
        } else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            scale_shift = atoi(argv[++i]);
            if (scale_shift < 0) scale_shift = 0;
            if (scale_shift > 3) scale_shift = 3;
            LOG("Option: -d %d", scale_shift);
        } else if (argv[i][0] != '-') {
            file = argv[i];
            LOG("Filename: %s", file);
        }
    }

    if (!file) {
        LOG("ERROR: no JPEG file specified");
        if (logfp) fclose(logfp);
        return 1;
    }

    if (GrOpen() < 0) {
        LOG("ERROR: GrOpen failed");
        if (logfp) fclose(logfp);
        return 1;
    }

    GR_SCREEN_INFO si;
    GrGetScreenInfo(&si);
    LOG("Screen: %dx%d, bpp=%d, ncolors=%d",
        si.cols, si.rows, si.bpp, si.ncolors);

    /* Load system palette (we only really care about 0,7,8,15) */
    pal.count = 256;
    GrGetSystemPalette(&pal);
    for (int i = 0; i < 16; i++) {
        LOG("PAL[%d] = R=%d G=%d B=%d",
            i,
            pal.palette[i].r,
            pal.palette[i].g,
            pal.palette[i].b);
    }

    /* Determine image size after -r/-d */
    GR_SIZE imgw = si.cols;
    GR_SIZE imgh = si.rows;
    if (get_final_jpeg_size(file, &imgw, &imgh) < 0) {
        LOG("WARNING: using full screen size");
        imgw = si.cols;
        imgh = si.rows;
    }

    if (imgw > si.cols) imgw = si.cols;
    if (imgh > si.rows) imgh = si.rows;
    if (imgw < 1) imgw = 1;
    if (imgh < 1) imgh = 1;

    LOG("Window size: %dx%d", imgw, imgh);

    GR_WINDOW_ID wid =
        GrNewWindowEx(GR_WM_PROPS_APPWINDOW,
                      "nxjpeg",
                      GR_ROOT_WINDOW_ID,
                      0, 0, imgw, imgh,
                      BLACK);

    GrSelectEvents(wid, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(wid);

    GR_GC_ID gc = GrNewGC();
    int done = 0;

    while (1) {
        GR_EVENT ev;
        GrGetNextEvent(&ev);

        if (ev.type == GR_EVENT_TYPE_CLOSE_REQ) {
            LOG("Close request");
            GrClose();
            if (logfp) fclose(logfp);
            return 0;
        }

        if (ev.type == GR_EVENT_TYPE_EXPOSURE && !done) {
            LOG("Exposure: start drawing");
            decode_and_draw_mcus(file, wid, gc, imgw, imgh);
            done = 1;
        }
    }

    /* not reached */
    return 0;
}

