/*
 * nxjpeg - Nano-X JPEG viewer for ELKS VGA
 *
 * Uses PicoJPEG (MCU-by-MCU) and draws directly to
 * a Nano-X VGA window.
 *
 * - Converts per-pixel RGB → grayscale (0–255).
 * - Quantizes grayscale to 4 VGA gray entries: indices 0, 8, 7, 15.
 * - Renders immediately using EmuGrArea (RLE -> GrLine).
 * - NO malloc, no full image buffer. Only MCU strip buffer.
 * - Logs to /tmp/nxjpeg.log
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "picojpeg.h"

#define MAX_WIDTH   640
#define MAX_HEIGHT  480

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

/* System palette */
static GR_PALETTE pal;

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

/* Map grayscale to fixed VGA gray steps: indices 0, 8, 7, 15 */
static inline unsigned char quantize_gray4(unsigned char g)
{
    if (g < 64)   return 0;   /* black */
    if (g < 128)  return 8;   /* dark gray */
    if (g < 192)  return 7;   /* light gray */
    return 15;                /* white */
}

/* ----------------------------------------------------------- */
/* Emulated GrArea (RLE to GrLine)                             */
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

        /* finish last run */
        GrSetGCForeground(gc,
            GR_RGB(pal.palette[run_idx].r,
                   pal.palette[run_idx].g,
                   pal.palette[run_idx].b));
        GrLine(wid, gc,
               x + run_start,  y + iy,
               x + (int)w - 1, y + iy);
    }
}

/* ----------------------------------------------------------- */
/* Stream JPEG (MCU → grayscale → quantize → draw per strip)   */
/* ----------------------------------------------------------- */

static int stream_jpeg_and_draw(const char *file,
                                GR_WINDOW_ID wid,
                                GR_GC_ID gc)
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("stream_jpeg_and_draw: cannot open %s", file);
        return -1;
    }

    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info,
                               pjpeg_need_bytes_callback,
                               &jf,
                               0 /* no reduce */);
    if (rc) {
        LOG("stream_jpeg_and_draw: decode_init rc=%d", rc);
        fclose(fp);
        return -1;
    }

    unsigned imgw = info.m_width;
    unsigned imgh = info.m_height;

    if (imgw > MAX_WIDTH)  imgw = MAX_WIDTH;
    if (imgh > MAX_HEIGHT) imgh = MAX_HEIGHT;

    unsigned mcu_w = info.m_MCUWidth;
    unsigned mcu_h = info.m_MCUHeight;

    unsigned blocks_x = mcu_w / 8;
    unsigned blocks_y = mcu_h / 8;
    if (!blocks_x) blocks_x = 1;
    if (!blocks_y) blocks_y = 1;

    LOG("Streaming JPEG %ux%u, MCU=%ux%u (blocks %ux%u)",
         imgw, imgh, mcu_w, mcu_h, blocks_x, blocks_y);

    /* Max MCU width for 4:2:0 is 16; 32 is very safe. */
    #define MCU_MAX_WIDTH 32
    static unsigned char stripbuf[MCU_MAX_WIDTH];

    int mcu_index = 0;

    while (1) {
        rc = pjpeg_decode_mcu();
        if (rc == PJPG_NO_MORE_BLOCKS)
            break;
        if (rc) {
            LOG("stream: decode_mcu rc=%d at idx=%d", rc, mcu_index);
            fclose(fp);
            return -1;
        }

        unsigned mx = mcu_index % info.m_MCUSPerRow;
        unsigned my = mcu_index / info.m_MCUSPerRow;
        mcu_index++;

        unsigned px = mx * mcu_w;
        unsigned py = my * mcu_h;

        for (unsigned ly = 0; ly < mcu_h; ly++) {
            unsigned gy = py + ly;
            if (gy >= imgh) break;

            /* visible width of this MCU strip */
            unsigned valid_w = mcu_w;
            if (px + valid_w > imgw)
                valid_w = imgw - px;
            if (valid_w > MCU_MAX_WIDTH)
                valid_w = MCU_MAX_WIDTH;

            /* convert MCU-row → stripbuf */
            for (unsigned lx = 0; lx < valid_w; lx++) {

                /* correct JPEG MCU indexing */
                unsigned block_x = lx / 8;
                unsigned block_y = ly / 8;
                if (block_x >= blocks_x) block_x = blocks_x - 1;
                if (block_y >= blocks_y) block_y = blocks_y - 1;

                unsigned bx = lx % 8;
                unsigned by = ly % 8;

                unsigned bindex = block_y * blocks_x + block_x;
                unsigned sindex = bindex * 64 + by * 8 + bx;
                if (sindex >= 256) sindex = 255;

                unsigned char r = gMCUBufR[sindex];
                unsigned char g = gMCUBufG[sindex];
                unsigned char b = gMCUBufB[sindex];

                unsigned char gray = to_gray(r, g, b);
                stripbuf[lx] = quantize_gray4(gray);
            }

            /* draw only this MCU strip */
            if (valid_w > 0) {
                EmuGrArea(wid, gc,
                          px, gy,
                          valid_w, 1,
                          stripbuf,
                          valid_w);
            }
        }
    }

    fclose(fp);
    LOG("stream_jpeg_and_draw: done");
    return 0;
}

/* ----------------------------------------------------------- */
/* MAIN                                                        */
/* ----------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *file = NULL;

    logfp = fopen("/tmp/nxjpeg.log", "w");
    LOG("nxjpeg starting (MCU streaming, no buffer)");

    for (int i = 1; i < argc; i++)
        if (argv[i][0] != '-')
            file = argv[i];

    if (!file) {
        LOG("No JPEG file supplied");
        return 1;
    }

    if (GrOpen() < 0) {
        LOG("GrOpen failed");
        return 1;
    }

    GR_SCREEN_INFO si;
    GrGetScreenInfo(&si);
    LOG("Screen %dx%d, bpp=%d, colors=%d",
        si.cols, si.rows, si.bpp, si.ncolors);

    /* Load system palette */
    pal.count = 256;
    GrGetSystemPalette(&pal);
    for (int i = 0; i < 16; i++)
        LOG("PAL[%d] = %d %d %d",
            i, pal.palette[i].r, pal.palette[i].g, pal.palette[i].b);

    /* Get JPEG size (header only) */
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("cannot open for size");
        return 1;
    }
    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info,
                               pjpeg_need_bytes_callback,
                               &jf, 0);
    fclose(fp);

    if (rc) {
        LOG("decode_init for size rc=%d", rc);
        return 1;
    }

    GR_SIZE w = info.m_width;
    GR_SIZE h = info.m_height;

    if (w > MAX_WIDTH)  w = MAX_WIDTH;
    if (h > MAX_HEIGHT) h = MAX_HEIGHT;

    LOG("Window size = %dx%d", w, h);

    GR_WINDOW_ID wid =
        GrNewWindowEx(GR_WM_PROPS_APPWINDOW,
                      "nxjpeg",
                      GR_ROOT_WINDOW_ID,
                      0, 0, w, h,
                      BLACK);

    GrSelectEvents(wid, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(wid);

    GR_GC_ID gc = GrNewGC();
    int drawn = 0;

    while (1) {
        GR_EVENT ev;
        GrGetNextEvent(&ev);

        if (ev.type == GR_EVENT_TYPE_CLOSE_REQ) {
            LOG("close");
            GrClose();
            fclose(logfp);
            return 0;
        }

        if (ev.type == GR_EVENT_TYPE_EXPOSURE && !drawn) {
            LOG("exposure -> streaming decode");
            stream_jpeg_and_draw(file, wid, gc);
            drawn = 1;
        }
    }
}

