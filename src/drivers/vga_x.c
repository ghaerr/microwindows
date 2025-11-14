/* nxvga_x.c - Nano-X Mode X VGA driver for ELKS
 *
 * 16-bit, ELKS compatible. Uses Mode X unchained 320x200x256, plane-aware VRAM writes.
 * Back buffer stores only window contents; desktop background is a single color.
 * 
 */

#include <stdlib.h>
#include <string.h>
#include "nano-X.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200
#define SCREEN_SIZE   (SCREEN_WIDTH*SCREEN_HEIGHT/4)
#define SC_INDEX      0x03c4
#define SC_DATA       0x03c5
#define CRTC_INDEX    0x03d4
#define CRTC_DATA     0x03d5
#define MAP_MASK      0x02
#define MEMORY_MODE   0x04
#define HIGH_ADDRESS  0x0C
#define LOW_ADDRESS   0x0D

typedef unsigned char byte;

static byte *VGA = (byte*)0xA0000L;   /* Mode X VRAM */
static byte *backbuf = NULL;          /* back buffer for window regions */
static byte bg_color = 0;             /* desktop background color */

/* ------------------- Hardware Helper Functions ------------------- */

/* Set VGA into 320x200 Mode X unchained */
static int vga_set_modeX(void) {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = 0x13; /* 320x200 256-color */
    int86(0x10,&regs,&regs);

    /* Verify mode 0x13 */
    if (regs.h.al != 0x13) return -1;

    /* Unchain mode (turn off chain4) */
    outp(SC_INDEX, MEMORY_MODE);
    outp(SC_DATA, 0x06);

    /* Clear all planes */
    outpw(SC_INDEX, 0xff02);
    for(int i=0;i<0x4000;i++) ((unsigned int*)VGA)[i]=0;

    return 0;
}

/* Plane-aware write of rectangle from back buffer to VRAM */
static void vga_update_region(int x, int y, int w, int h) {
    int plane, px, py;
    for (plane=0; plane<4; plane++) {
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);
        for (py=0; py<h; py++) {
            for (px=0; px<w; px++) {
                int vx = x+px;
                int vy = y+py;
                if ((vx&3)==plane) {
                    VGA[(vy<<6)+(vy<<4)+(vx>>2)] = backbuf[vy*SCREEN_WIDTH + vx];
                }
            }
        }
    }
}

/* ------------------- Nano-X Driver Callbacks ------------------- */

static GR_BOOL nxvga_open(GR_SCREEN_INFO *pinfo) {
    if (vga_set_modeX() < 0) return GR_FALSE;

    /* Allocate back buffer for window regions */
    backbuf = malloc(SCREEN_WIDTH*SCREEN_HEIGHT);
    if (!backbuf) return GR_FALSE;

    bg_color = 0; /* default background color */
    memset(backbuf,bg_color,SCREEN_WIDTH*SCREEN_HEIGHT);

    /* Copy full buffer to VRAM */
    vga_update_region(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

    /* Fill screen info */
    pinfo->Width = SCREEN_WIDTH;
    pinfo->Height = SCREEN_HEIGHT;
    pinfo->Planes = 1;
    pinfo->Bpp = 8;
    pinfo->RefreshRate = 60;

    return GR_TRUE;
}

static void nxvga_close(void) {
    if (backbuf) free(backbuf);
    backbuf = NULL;

    /* Restore text mode */
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = 0x03; /* 80x25 text mode */
    int86(0x10,&regs,&regs);
}

static void nxvga_setpixel(int x,int y,GR_PIXELVAL color) {
    if (x<0 || x>=SCREEN_WIDTH || y<0 || y>=SCREEN_HEIGHT) return;

    backbuf[y*SCREEN_WIDTH + x] = color;
    vga_update_region(x,y,1,1);
}

static GR_PIXELVAL nxvga_getpixel(int x,int y) {
    if (x<0 || x>=SCREEN_WIDTH || y<0 || y>=SCREEN_HEIGHT) return bg_color;
    return backbuf[y*SCREEN_WIDTH + x];
}

/* Flush a rectangle (Nano-X calls this to update regions) */
static void nxvga_flush(GR_RECT *prect) {
    int x = prect->x;
    int y = prect->y;
    int w = prect->w;
    int h = prect->h;

    /* Clip to screen */
    if (x<0) { w+=x; x=0; }
    if (y<0) { h+=y; y=0; }
    if (x+w>SCREEN_WIDTH) w=SCREEN_WIDTH-x;
    if (y+h>SCREEN_HEIGHT) h=SCREEN_HEIGHT-y;
    if (w<=0 || h<=0) return;

    vga_update_region(x,y,w,h);
}

/* Set background color */
static void nxvga_setbg(GR_PIXELVAL color) {
    bg_color = color;
}

/* ------------------- GR_DRIVER Structure ------------------- */

GR_DRIVER NXVGA_Driver = {
    "nxvga_x",
    nxvga_open,
    nxvga_close,
    nxvga_setpixel,
    nxvga_getpixel,
    nxvga_flush,
    nxvga_setbg,
    NULL,  /* line draw (optional) */
    NULL,  /* rect fill (optional) */
    NULL   /* blit (optional) */
};

