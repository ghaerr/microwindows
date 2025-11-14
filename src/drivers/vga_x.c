/* nxvga_x.c - Nano-X Mode X VGA driver for ELKS
 *
 * VGA Mode X unchained 320x200x256, plane-aware VRAM writes.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"
#include <arch/io.h>

typedef unsigned char  byte;
typedef unsigned short word;

/* VGA 256-color mode 13h */
#define VGA_256_COLOR_MODE  0x13
#define TEXT_MODE           0x03
#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       200
#define NUM_COLORS          256

/* bytes per scanline seen by Mode X planar layout */
#define BYTES_PER_LINE      80   /* 320 / 4 */

static byte __far *VGA = (byte __far *)0xA0000000L;

/* Set video mode using BIOS, used for open and close */
void set_mode(byte mode)
{
    __asm__(
        "push %%si;"
        "push %%di;"
        "push %%bp;"
        "push %%es;"
        "cli;"
        "mov %%ah,0;"
        "mov %%al,%0;"
        "int $0x10;"
        "sti;"
        "pop %%es;"
        "pop %%bp;"
        "pop %%di;"
        "pop %%si;"
        :
        : "r" (mode)
        : "ax"
    );
}

/* Configure VGA X (unchained planar 256-color) registers */
static void vga_x_configure(void)
{
    /* =============== Disable Chain-4 (GC register 0x06) =============== */
    outb(0x06, 0x3CE);     /* Graphics Controller: Mode register */
    outb(0x05, 0x3CF);     /* Disable chain-4 (bit 3=0), odd/even off */

    /* =============== Enable all planes in Sequencer =============== */
    outb(0x02, 0x3C4);     
    outb(0x0F, 0x3C5);     /* allow writes to all 4 planes */

    /* =============== GC write mode 0, read mode 0 =============== */
    outb(0x05, 0x3CE);     
    outb(0x00, 0x3CF);
}

/* Helper: calculate planar byte offset */
static inline int nxvga_offset(int x, int y)
{
    return (y * BYTES_PER_LINE) + (x >> 2);  /* each byte holds 4 pixels */
}

/* Helper: select the write plane by writing Sequencer index/data */
static inline void nxvga_select_plane(int plane)
{
    /* Sequencer Map Mask:
     * outb(index, port_index);
     * outb(data,  port_data);
     */
    outb(0x02, 0x3C4);           /* index = Map Mask */
    outb((byte)(1 << plane), 0x3C5); /* data = plane mask */
}

/* Plot pixel in Mode X (correct plane selection) */
static void nxvga_setpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    int plane = x & 3;                        /* which plane */
    int offset = nxvga_offset(x, y);          /* byte offset in plane */
    nxvga_select_plane(plane);                /* set map mask properly */
    VGA[offset] = (byte)c;                    /* write color byte to that plane */
}

/* Read pixel in Mode X (correct plane selection) */
static MWPIXELVAL nxvga_getpixel(PSD psd, MWCOORD x, MWCOORD y)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return 0;
    int plane = x & 3;
    int offset = nxvga_offset(x, y);
    nxvga_select_plane(plane);
    return VGA[offset];
}

/* Fill rectangle (unoptimized) */
static void nxvga_fillrect(PSD psd, MWCOORD x, MWCOORD y,
                           MWCOORD w, MWCOORD h, MWPIXELVAL c)
{
    if (w <= 0 || h <= 0) return;

    /* clip to screen */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH)  w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            nxvga_setpixel(psd, x + i, y + j, c);
        }
    }
}

/* Subdriver struct */
static SUBDRIVER nxvga_subdriver = {
    nxvga_setpixel,
    nxvga_getpixel,
    NULL,           
    NULL,          
    nxvga_fillrect,
    NULL,          
    NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL
};

/* Expose subdriver array */
PSUBDRIVER nxvga_drivers[] = {
    &nxvga_subdriver
};

/* Open driver: set Mode X */
int nxvga_open(PSD psd)
{
    /* fill psd fields so higher layers can use them */
    psd->xres = SCREEN_WIDTH;
    psd->yres = SCREEN_HEIGHT;
    psd->planes = 1;
    psd->bpp = 8;
    psd->ncolors = NUM_COLORS;
    psd->flags = PSF_SCREEN;

    psd->pitch = 80; /* pitch in pixels = 80 bytes * 4 pixels/byte = 320 */

    set_mode(VGA_256_COLOR_MODE);   /* BIOS mode 0x13 */
    vga_x_configure();              /* X mode register setup */

    return 1;
}

/* Close driver: reset to text mode */
int nxvga_close(PSD psd)
{
    set_mode(TEXT_MODE);
    return 1;
}

