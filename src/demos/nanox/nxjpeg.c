/*
 * nxjpeg - Nano-X JPEG viewer for VGA (optimized for low memory and ELKS)
 *
 * Uses PicoJPEG (MCU-by-MCU) and draws to a Nano-X VGA window. 
 * Nano X on VGA supports only 16 colors. A grayscale output was 
 * preferred to a 16-color image match.
 * 
 * - Converts per-pixel RGB → grayscale (0–255).
 * - Quantizes grayscale to 4 VGA gray entries: indices 0, 8, 7, 15.
 * - Logs to /tmp/nxjpeg.log
 *
 * Input JPEG image must be:
 * - baseline (SOF0), non-progressive, Huffman-coded, 8×8 DCT blocks, no arithmetic coding, no restart markers
 * - image size should be less than VGA 640×480 to fit to screen in Nano X 
 * - on Linux use: convert input.png -resize 400x400\> -colorspace RGB -strip -sampling-factor 1x1 -define jpeg:dct-method=integer -quality 85 -interlace none -depth 8 -type truecolor -compress JPEG output.jpg
 *
 * Modes:
 *   ROW_BUFFER = 0  -> per-MCU streaming renderer
 *                     - 4-level grayscale (0, 8, 7, 15)
 *                     - OR+AND uniform MCU detection
 *                     - batch up to 6 uniform MCUs into one filled rect (speed optimization)
 *                     - non-uniform MCUs drawn per scanline via EmuGrArea
 *
 *   ROW_BUFFER = 1  -> row-buffer renderer
 *                     - reserves buffer large enough to hold one full
 *                       "band" of MCUs: width × MCU height (up to 400x16)
 *                     - decodes a whole MCU row into a band buffer
 *                     - draws entire band row-by-row using own EmuGrArea
 * Which mode is faster depends on the image.
 *
 * This program uses code or is inspired by:
 *   - https://github.com/rafael2k/elks-viewer
 *   - https://github.com/richgel999/picojpeg
 * 
 * History:
 *   2/12/2025 version 1.0 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "picojpeg.h"

#define MAX_WIDTH   620
#define MAX_HEIGHT  460

/* Switch between strategies */
#define ROW_BUFFER 1       /* set to 1 to enable row-buffer mode */

/* ----------------------------------------------------------- */
/* Logging                                                     */
/* ----------------------------------------------------------- */

static FILE *logfp = NULL;
#define LOG(fmt, ...) \
    do { if (logfp) { fprintf(logfp, fmt "\n", ##__VA_ARGS__); fflush(logfp);} } while (0)

/* ----------------------------------------------------------- */
/* PicoJPEG externs                                            */
/* ----------------------------------------------------------- */

extern unsigned char gMCUBufR[256];
extern unsigned char gMCUBufG[256];
extern unsigned char gMCUBufB[256];

/* System palette and precomputed RGB */
static GR_PALETTE pal;
static GR_COLOR color_from_index[256];

/* JPEG file wrapper */
typedef struct {
    FILE *fp;
} JPEG_FILE;

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
/* Grayscale + quantization                                    */
/* ----------------------------------------------------------- */

static inline unsigned char to_gray(unsigned char r,
                                    unsigned char g,
                                    unsigned char b)
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

/* ----------------------------------------------------------- */
/* Per-MCU streaming renderer (ROW_BUFFER == 0)                */
/*  - OR+AND uniform detection + batching                      */
/* ----------------------------------------------------------- */

#if !ROW_BUFFER

static int stream_jpeg_and_draw_mcu(const char *file,
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

    LOG("MCU-mode: JPEG %ux%u  MCU %ux%u  MCUs/row=%u",
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

        unsigned mx = mcu_index % info.m_MCUSPerRow;
        unsigned my = mcu_index / info.m_MCUSPerRow;
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

        int samples = blocks_x * blocks_y * 64;
        if (samples > 256) samples = 256;

        unsigned char first = quantize_gray4(
            to_gray(gMCUBufR[0], gMCUBufG[0], gMCUBufB[0]));
        qbuf[0] = first;

        unsigned char all_or  = first;
        unsigned char all_and = first;

        for (int si = 1; si < samples; si++) {
            unsigned char q = quantize_gray4(
                to_gray(gMCUBufR[si], gMCUBufG[si], gMCUBufB[si]));
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

                unsigned char gray = to_gray(
                    gMCUBufR[sindex], gMCUBufG[sindex], gMCUBufB[sindex]);
                stripbuf[lx] = quantize_gray4(gray);
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

    fclose(fp);
    LOG("stream_jpeg_and_draw_mcu: done");
    return 0;
}

#endif /* !ROW_BUFFER */

/* ----------------------------------------------------------- */
/* Row-buffer renderer (ROW_BUFFER == 1)                       */
/*  - buffer holds one MCU row: width x MCUHeight              */
/* ----------------------------------------------------------- */

#if ROW_BUFFER

static int stream_jpeg_and_draw_rowbuf(const char *file,
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

    LOG("ROWBUF-mode: JPEG %ux%u  MCU %ux%u  MCUs/row=%u",
        imgw, imgh, mcu_w, mcu_h, info.m_MCUSPerRow);

    /* row buffer: holds imgw x mcu_h pixels (max 400x16 = 6400 bytes) */
    #define MAX_MCU_H 16
    #define MAX_ROWBUF (MAX_WIDTH * MAX_MCU_H)
    static unsigned char rowbuf[MAX_ROWBUF];

    unsigned row_pitch = imgw;

    int mcu_index = 0;
    int current_my = -1;      /* current MCU row index in buffer */
    unsigned band_py = 0;     /* y coordinate of band top */

    /* helper to flush one full band to screen */
    #define FLUSH_BAND() \
        do { \
            if (current_my >= 0) { \
                for (unsigned ly = 0; ly < mcu_h; ly++) { \
                    unsigned gy = band_py + ly; \
                    if (gy >= imgh) break; \
                    EmuGrArea(wid, gc, \
                              0, gy, \
                              imgw, 1, \
                              &rowbuf[ly * row_pitch], \
                              row_pitch); \
                } \
                current_my = -1; \
            } \
        } while (0)

    while (1) {
        rc = pjpeg_decode_mcu();
        if (rc == PJPG_NO_MORE_BLOCKS)
            break;
        if (rc) {
            LOG("decode_mcu rc=%d at index=%d", rc, mcu_index);
            FLUSH_BAND();
            fclose(fp);
            return -1;
        }

        unsigned mx = mcu_index % info.m_MCUSPerRow;
        unsigned my = mcu_index / info.m_MCUSPerRow;
        mcu_index++;

        unsigned px = mx * mcu_w;
        unsigned py = my * mcu_h;

        /* if we started a new MCU row, flush previous band */
        if (current_my >= 0 && (int)my != current_my) {
            FLUSH_BAND();
        }

        if (current_my < 0) {
            /* starting a new band */
            current_my = (int)my;
            band_py = py;
            /* clear band buffer (optional, but safe) */
            memset(rowbuf, 0, row_pitch * mcu_h);
        }

        /* Skip MCUs completely below image */
        if (py >= imgh)
            continue;

        /* fill band buffer for this MCU */
        for (unsigned ly = 0; ly < mcu_h; ly++) {
            unsigned gy = py + ly;
            if (gy >= imgh) break;

            for (unsigned lx = 0; lx < mcu_w; lx++) {
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

                unsigned char gray = to_gray(
                    gMCUBufR[sindex], gMCUBufG[sindex], gMCUBufB[sindex]);
                unsigned char q = quantize_gray4(gray);

                unsigned band_ly = ly;
                unsigned band_x  = gx;          /* 0..imgw-1 */
                rowbuf[band_ly * row_pitch + band_x] = q;
            }
        }
    }

    /* flush last band */
    FLUSH_BAND();

    #undef FLUSH_BAND

    fclose(fp);
    LOG("stream_jpeg_and_draw_rowbuf: done");
    return 0;
}

#endif /* ROW_BUFFER */

/* ----------------------------------------------------------- */
/* Dispatcher: picks renderer based on ROW_BUFFER              */
/* ----------------------------------------------------------- */

static int stream_jpeg_and_draw(const char *file,
                                GR_WINDOW_ID wid,
                                GR_GC_ID gc)
{
#if ROW_BUFFER
    return stream_jpeg_and_draw_rowbuf(file, wid, gc);
#else
    return stream_jpeg_and_draw_mcu(file, wid, gc);
#endif
}

/* ----------------------------------------------------------- */
/* MAIN                                                        */
/* ----------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *file = NULL;

    logfp = fopen("/tmp/nxjpeg.log", "w");
    LOG("nxjpeg starting (ROW_BUFFER=%d)", ROW_BUFFER);

    for (int i = 1; i < argc; i++)
        if (argv[i][0] != '-')
            file = argv[i];

    if (!file) {
        LOG("No JPEG file supplied or invalid path");
        return 1;
    }

    if (GrOpen() < 0) {
        LOG("GrOpen failed");
        return 1;
    }

    GR_SCREEN_INFO si;
    GrGetScreenInfo(&si);
    LOG("Screen %dx%d bpp=%d colors=%d", si.cols, si.rows, si.bpp, si.ncolors);

    pal.count = 256;
    GrGetSystemPalette(&pal);
    for (int i = 0; i < pal.count; i++)
        color_from_index[i] = GR_RGB(
            pal.palette[i].r, pal.palette[i].g, pal.palette[i].b);

    for (int i = 0; i < 16; i++)
        LOG("PAL[%d] = %d %d %d",
            i, pal.palette[i].r, pal.palette[i].g, pal.palette[i].b);

    /* read JPEG header for size */
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("Cannot open JPEG for size");
        return 1;
    }
    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info, pjpeg_need_bytes_callback, &jf, 0);
    fclose(fp);

    if (rc) {
        LOG("decode_init for size rc=%d", rc);
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

    while (1) {
        GR_EVENT ev;
        GrGetNextEvent(&ev);

        if (ev.type == GR_EVENT_TYPE_CLOSE_REQ) {
            LOG("close");
            GrClose();
            fclose(logfp);
            return 0;
        }

        if (ev.type == GR_EVENT_TYPE_EXPOSURE) {
            LOG("exposure -> redraw");
            stream_jpeg_and_draw(file, wid, gc);
        }
    }
}
