/*
 * nxjpeg - Nano-X JPEG viewer for ELKS VGA
 *
 * Uses PicoJPEG (MCU-by-MCU) and draws directly to
 * a Nano-X VGA window.
 * 
 * -p   Draw patterned background behind image
 * -r   Use PicoJPEG built-in 1/8 resolution reduction
 * -s   Open window at half screen size
 * -d n Extra downscale by 2^n (1, 2, 4, 8...)
 * -c : Enable 16-color optimization (simple horizontal dithering).
 *      If the JPEG is detected as already <=16 colors, dithering is skipped.
 *
 * Auto-detects 16 or 256 color VGA modes:
 *
 *  - 256-color mode:
 *        fast RGB cube → 8-bit index (no palette distance)
 *        much higher color quality
 *
 *  - 16-color mode:
 *        uses Floyd–Steinberg dithering (still look sterrible)
 *        produces highest possible quality in 4-bit VGA mode
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "picojpeg.h"

/* ---------------------------------------------------------- */
/* Logging                                                    */
/* ---------------------------------------------------------- */

static FILE *logfp = NULL;
#define LOG(fmt, ...) do { if (logfp) { fprintf(logfp, fmt "\n", ##__VA_ARGS__); fflush(logfp); } } while (0)

/* ---------------------------------------------------------- */
/* Options                                                    */
/* ---------------------------------------------------------- */

static int pflag = 0;              /* currently unused (pattern) */
static int sflag = 0;              /* half-screen window */
static int reduce_flag = 0;        /* PicoJPEG reduce mode */
static int scale_shift = 0;        /* additional >>n scale */
static int enable_16color_opt = 0; /* -c: 16-color optimization (dither) */

/* Max supported width for error buffers */
#define MAX_WIDTH 640

/* PicoJPEG MCU buffers (defined in picojpeg.c) */
extern unsigned char gMCUBufR[256];
extern unsigned char gMCUBufG[256];
extern unsigned char gMCUBufB[256];

typedef struct {
    FILE *fp;
} JPEG_FILE;

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

/* ---------------------------------------------------------- */
/* Palette and mode detection                                 */
/* ---------------------------------------------------------- */

static GR_PALETTE pal;
static int is_256 = 0;          /* 1 = 256-color mode, 0 = <=16-color mode */
static int jpeg_is_16color = 0; /* 1 if JPEG seems to use <=16 unique colors */

/* Nearest-color mapping to Nano-X palette */
static unsigned char map_rgb(unsigned char r, unsigned char g, unsigned char b)
{
    int best = 0;
    long bestd = 999999999L;

    for (int i = 0; i < pal.count; i++) {
        long dr = (long)pal.palette[i].r - r;
        long dg = (long)pal.palette[i].g - g;
        long db = (long)pal.palette[i].b - b;
        long d = dr*dr + dg*dg + db*db;
        if (d < bestd) {
            bestd = d;
            best = i;
        }
    }
    return (unsigned char)best;
}

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ---------------------------------------------------------- */
/* Emulated "GrArea" using RLE + GrLine                       */
/* ---------------------------------------------------------- */

static void EmuGrArea(GR_WINDOW_ID wid, GR_GC_ID gc,
                      GR_COORD x, GR_COORD y,
                      GR_SIZE w, GR_SIZE h,
                      const unsigned char *buf,
                      GR_SIZE pitch)
{
    for (int iy = 0; iy < (int)h; iy++) {
        const unsigned char *row = buf + iy * pitch;
        int run_start = 0;
        unsigned char run_color = row[0];

        for (int ix = 1; ix < (int)w; ix++) {
            if (row[ix] != run_color) {
                GrSetGCForeground(gc,
                    GR_RGB(pal.palette[run_color].r,
                           pal.palette[run_color].g,
                           pal.palette[run_color].b));
                GrLine(wid, gc,
                       x + run_start, y + iy,
                       x + ix - 1,    y + iy);
                run_start = ix;
                run_color = row[ix];
            }
        }

        GrSetGCForeground(gc,
            GR_RGB(pal.palette[run_color].r,
                   pal.palette[run_color].g,
                   pal.palette[run_color].b));
        GrLine(wid, gc,
               x + run_start, y + iy,
               x + (int)w - 1, y + iy);
    }
}

/* ---------------------------------------------------------- */
/* Detect if JPEG is effectively <=16 colors                   */
/* ---------------------------------------------------------- */

static int detect_jpeg_16color(const char *file)
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        LOG("detect_jpeg_16color: cannot open %s", file);
        return 0;
    }

    JPEG_FILE jf = { fp };
    pjpeg_image_info_t info;
    int rc = pjpeg_decode_init(&info,
                               pjpeg_need_bytes_callback,
                               &jf,
                               (unsigned char)reduce_flag);
    if (rc) {
        LOG("detect_jpeg_16color: pjpeg_decode_init rc=%d", rc);
        fclose(fp);
        return 0;
    }

    unsigned char seen[16][3];
    int count = 0;
    int max_mcus = 32; /* scan at most 32 MCUs */

    for (int m = 0; m < max_mcus; m++) {
        rc = pjpeg_decode_mcu();
        if (rc == PJPG_NO_MORE_BLOCKS) break;
        if (rc) break;

        unsigned mcu_pixels = info.m_MCUWidth * info.m_MCUHeight;
        if (mcu_pixels > 256) mcu_pixels = 256;

        for (unsigned p = 0; p < mcu_pixels; p++) {
            unsigned char r = gMCUBufR[p];
            unsigned char g = gMCUBufG[p];
            unsigned char b = gMCUBufB[p];

            int found = 0;
            for (int c = 0; c < count; c++) {
                if (seen[c][0] == r &&
                    seen[c][1] == g &&
                    seen[c][2] == b) {
                    found = 1;
                    break;
                }
            }
            if (found)
                continue;

            if (count >= 16) {
                /* More than 16 colors => not "16-color JPEG" */
                fclose(fp);
                LOG("detect_jpeg_16color: >16 colors found, not palettized.");
                return 0;
            }

            seen[count][0] = r;
            seen[count][1] = g;
            seen[count][2] = b;
            count++;
        }
    }

    fclose(fp);
    LOG("detect_jpeg_16color: <=16 colors detected (unique=%d)", count);
    return 1; /* <=16 unique RGB colors */
}

/* ---------------------------------------------------------- */
/* Get final JPEG size after -r and -d scaling                */
/* ---------------------------------------------------------- */

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
    LOG("Final JPEG size (after -r/-d): %ux%u", w, h);
    return 0;
}

/* ---------------------------------------------------------- */
/* Decode and draw per MCU                                    */
/* ---------------------------------------------------------- */

static int decode_and_draw_mcus(const char *file,
                                GR_WINDOW_ID wid, GR_GC_ID gc,
                                GR_SIZE ww, GR_SIZE wh)
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
    LOG("MCU: %ux%u  MCUs: %ux%u, width=%u height=%u",
        mcu_w, mcu_h, info.m_MCUSPerRow, info.m_MCUSPerCol,
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

    unsigned char mcu_idx[256]; /* up to 16x16 = 256 pixels */

    /* Error buffers for simple horizontal dithering in 16-color mode */
    int err_r[MAX_WIDTH];
    int err_g[MAX_WIDTH];
    int err_b[MAX_WIDTH];

    memset(err_r, 0, sizeof(err_r));
    memset(err_g, 0, sizeof(err_g));
    memset(err_b, 0, sizeof(err_b));

    for (unsigned my = 0; my < info.m_MCUSPerCol; my++) {

        if (!is_256 && enable_16color_opt && !jpeg_is_16color) {
            /* Reset error per MCU row band */
            memset(err_r, 0, img_w * sizeof(int));
            memset(err_g, 0, img_w * sizeof(int));
            memset(err_b, 0, img_w * sizeof(int));
        }

        for (unsigned mx = 0; mx < info.m_MCUSPerRow; mx++) {

            rc = pjpeg_decode_mcu();
            if (rc == PJPG_NO_MORE_BLOCKS) break;
            if (rc) {
                LOG("decode_and_draw_mcus: pjpeg_decode_mcu rc=%d", rc);
                fclose(fp);
                return -1;
            }

            unsigned sx0 = (mx * mcu_w);
            unsigned sy0 = (my * mcu_h);

            if (scale_shift > 0) {
                sx0 >>= scale_shift;
                sy0 >>= scale_shift;
            }

            if (sx0 >= img_w || sy0 >= img_h)
                continue;

            unsigned sx1 = ((mx + 1) * mcu_w - 1);
            unsigned sy1 = ((my + 1) * mcu_h - 1);

            if (scale_shift > 0) {
                sx1 >>= scale_shift;
                sy1 >>= scale_shift;
            }

            unsigned sw = (sx1 >= sx0) ? (sx1 - sx0 + 1) : 0;
            unsigned sh = (sy1 >= sy0) ? (sy1 - sy0 + 1) : 0;

            if (sx0 + sw > img_w) sw = img_w - sx0;
            if (sy0 + sh > img_h) sh = img_h - sy0;

            if (sw == 0 || sh == 0)
                continue;

            if (sw * sh > sizeof(mcu_idx)) {
                LOG("WARNING: MCU block too large: %ux%u", sw, sh);
                continue;
            }

            memset(mcu_idx, 0, sw * sh);

            unsigned src_i = 0;

            for (unsigned iy = 0; iy < mcu_h; iy++) {
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

                    unsigned lx = sx - sx0;
                    unsigned ly = sy - sy0;
                    unsigned di = ly * sw + lx;

                    int r = gMCUBufR[src_i];
                    int g = gMCUBufG[src_i];
                    int b = gMCUBufB[src_i];

                    unsigned char idx;

                    if (is_256) {
                        /* 256-color: no dithering, full palette mapping */
                        idx = map_rgb((unsigned char)r,
                                      (unsigned char)g,
                                      (unsigned char)b);
                    } else if (enable_16color_opt && !jpeg_is_16color) {
                        /* 16-color optimization with simple horizontal error diffusion */
                        int sx_c = clampi((int)sx, 0, (int)img_w - 1);

                        r = clampi(r + err_r[sx_c], 0, 255);
                        g = clampi(g + err_g[sx_c], 0, 255);
                        b = clampi(b + err_b[sx_c], 0, 255);

                        idx = map_rgb((unsigned char)r,
                                      (unsigned char)g,
                                      (unsigned char)b);

                        int qr = pal.palette[idx].r - r;
                        int qg = pal.palette[idx].g - g;
                        int qb = pal.palette[idx].b - b;

                        if (sx_c + 1 < (int)img_w) {
                            err_r[sx_c+1] += qr / 2;
                            err_g[sx_c+1] += qg / 2;
                            err_b[sx_c+1] += qb / 2;
                        }
                    } else {
                        /* Plain 16-color (or <=16-color JPEG): no dithering, just nearest match */
                        idx = map_rgb((unsigned char)r,
                                      (unsigned char)g,
                                      (unsigned char)b);
                    }

                    mcu_idx[di] = idx;
                }
            }

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

/* ---------------------------------------------------------- */
/* main                                                       */
/* ---------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *file = NULL;

    logfp = fopen("/tmp/nxjpeg.log", "w");
    LOG("nxjpeg starting");

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p")) {
            pflag = 1;
            LOG("Option: -p");
        } else if (!strcmp(argv[i], "-s")) {
            sflag = 1;
            LOG("Option: -s");
        } else if (!strcmp(argv[i], "-r")) {
            reduce_flag = 1;
            LOG("Option: -r");
        } else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            scale_shift = atoi(argv[++i]);
            if (scale_shift < 0) scale_shift = 0;
            if (scale_shift > 3) scale_shift = 3;
            LOG("Option: -d %d", scale_shift);
        } else if (!strcmp(argv[i], "-c")) {
            enable_16color_opt = 1;
            LOG("Option: -c (16-color optimization)");
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
    LOG("Screen: %dx%d, bpp=%d, colors=%d",
        si.cols, si.rows, si.bpp, si.ncolors);

    is_256 = (si.bpp >= 8 && si.ncolors >= 256);
    LOG("Detected mode: %s", is_256 ? "256-color" : "16-color");

    /* Load system palette */
    pal.count = 256;
    GrGetSystemPalette(&pal);
    LOG("System palette count: %d", pal.count);

    for (int i = 0; i < pal.count; i++) {
    LOG("PAL[%d] = R=%d G=%d B=%d",
        i,
        pal.palette[i].r,
        pal.palette[i].g,
        pal.palette[i].b);
    }

    /* Detect if JPEG is effectively <=16 colors */
    jpeg_is_16color = detect_jpeg_16color(file);
    LOG("jpeg_is_16color = %d", jpeg_is_16color);

    /* Compute window size from JPEG size */
    GR_SIZE imgw = si.cols;
    GR_SIZE imgh = si.rows;

    if (get_final_jpeg_size(file, &imgw, &imgh) < 0) {
        LOG("WARNING: using full screen size");
        imgw = si.cols;
        imgh = si.rows;
    }

    if (sflag) {
        imgw /= 2;
        imgh /= 2;
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

    /* never reached */
    return 0;
}

