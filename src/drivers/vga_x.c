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

/* Far pointer to VGA memory */
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
    /* Disable chain-4 (Graphics Controller Index 4 = 0x04) */
    outb(0x04, 0x3CE); // Graphics Controller Index
    outb(0x06, 0x3CF); // Disable chain-4

    /* Enable all planes in Sequencer Map Mask */
    outb(0x02, 0x3C4);
    outb(0x0F, 0x3C5);

    /* Set Graphics Controller: write mode 0, read mode 0 */
    outb(0x05, 0x3CE);
    outb(0x00, 0x3CF);
}

/* Plot pixel in Mode X */
static void nxvga_setpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
    int plane = x & 3;                        // select plane
    int offset = (y * 80) + (x >> 2);        // each plane stores every 4th pixel
    outb(1 << plane, 0x3C4 + 2);             // Sequencer map mask
    VGA[offset] = c;
}

/* Read pixel in Mode X */
static MWPIXELVAL nxvga_getpixel(PSD psd, MWCOORD x, MWCOORD y)
{
    int plane = x & 3;
    int offset = (y * 80) + (x >> 2);
    outb(1 << plane, 0x3C4 + 2);
    return VGA[offset];
}

/* Fill rectangle (naive) */
static void nxvga_fillrect(PSD psd, MWCOORD x, MWCOORD y,
                           MWCOORD w, MWCOORD h, MWPIXELVAL c)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            nxvga_setpixel(psd, x + i, y + j, c);
}


/* Set background color (fill entire screen) */
static void nxvga_setbg(PSD psd, MWPIXELVAL c)
{
    nxvga_fillrect(psd, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, c);
}

/* Subdriver struct */
static SUBDRIVER nxvga_subdriver = {
    nxvga_setpixel,
    nxvga_getpixel,
    NULL,           /* drawhorzline optional */
    NULL,           /* drawvertline optional */
    nxvga_fillrect,
    NULL,           /* blit optional */
    NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL
};

/* Expose subdriver array */
PSUBDRIVER nxvga_drivers[] = {
    &nxvga_subdriver
};

/* Open driver: set Mode X */
int nxvga_open(PSD psd)
{
    psd->xres = SCREEN_WIDTH;
    psd->yres = SCREEN_HEIGHT;
    psd->planes = 1;
    psd->bpp = 8;
    psd->ncolors = NUM_COLORS;
    psd->flags = PSF_SCREEN;

    set_mode(VGA_256_COLOR_MODE);   // BIOS mode 0x13
    vga_x_configure();              // X mode register setup

    return 1;
}

/* Close driver: reset to text mode */
int nxvga_close(PSD psd)
{
    set_mode(TEXT_MODE);
    return 1;
}

