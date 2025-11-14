/* nxvga_x.c - Nano-X Mode X VGA driver for ELKS
 *
 * 16-bit, ELKS compatible. Uses Mode X unchained 320x200x256, plane-aware VRAM writes.
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <arch/io.h>
#include <nano-X.h>
#include <device.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200
#define SCREEN_PIXELS (SCREEN_WIDTH*SCREEN_HEIGHT)

typedef unsigned char byte;

static byte *VGA = (byte*)0xA0000L;   /* Mode X VRAM */
static byte *backbuf = NULL;          /* Back buffer */
static byte bg_color = 0;             /* Desktop background color */

/* ---------------- Hardware I/O ---------------- */

static void set_mode(byte mode)
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

/* ---------------- VGA Mode X ---------------- */

static int vga_set_modeX(void)
{
    set_mode(0x13); /* standard 320x200x256 */

    /* Unchain mode (disable chain4) */
    outb(0x04, 0x03C4);       /* Memory Mode Index */
    outb(0x06, 0x03C5);       /* Disable chain4 */

    /* Clear all planes */
    for (int plane = 0; plane < 4; plane++) {
        outb(0x02, 0x03C4);   /* Map mask */
        outb(1 << plane, 0x03C5);
        memset(VGA, 0, 0x10000);
    }

    return 0;
}

static void vga_update_region(int x, int y, int w, int h)
{
    for (int plane = 0; plane < 4; plane++) {
        outb(0x02, 0x03C4);       /* Map mask */
        outb(1 << plane, 0x03C5);

        for (int py = 0; py < h; py++) {
            for (int px = 0; px < w; px++) {
                int vx = x + px;
                int vy = y + py;
                if ((vx & 3) == plane) {
                    VGA[(vy << 6) + (vy << 4) + (vx >> 2)] =
                        backbuf[vy * SCREEN_WIDTH + vx];
                }
            }
        }
    }
}

/* ---------------- Subdriver Function Prototypes ---------------- */

static void nxvga_setpixel(int x,int y,byte color);
static byte nxvga_getpixel(int x,int y);
static void nxvga_drawhorzline(int x1,int x2,int y,byte color);
static void nxvga_drawvertline(int x,int y1,int y2,byte color);
static void nxvga_fillrect(int x,int y,int w,int h,byte color);
static void nxvga_blit(int dx,int dy,int w,int h,const byte *src);

/* ---------------- Subdriver Functions ---------------- */

static void nxvga_setpixel(int x,int y,byte color)
{
    if (x<0 || x>=SCREEN_WIDTH || y<0 || y>=SCREEN_HEIGHT) return;
    backbuf[y*SCREEN_WIDTH + x] = color;
    vga_update_region(x,y,1,1);
}

static byte nxvga_getpixel(int x,int y)
{
    if (x<0 || x>=SCREEN_WIDTH || y<0 || y>=SCREEN_HEIGHT) return bg_color;
    return backbuf[y*SCREEN_WIDTH + x];
}

static void nxvga_drawhorzline(int x1,int x2,int y,byte color)
{
    if(y<0 || y>=SCREEN_HEIGHT) return;
    if(x1<0) x1=0;
    if(x2>=SCREEN_WIDTH) x2=SCREEN_WIDTH-1;
    for(int x=x1;x<=x2;x++)
        nxvga_setpixel(x,y,color);
}

static void nxvga_drawvertline(int x,int y1,int y2,byte color)
{
    if(x<0 || x>=SCREEN_WIDTH) return;
    if(y1<0) y1=0;
    if(y2>=SCREEN_HEIGHT) y2=SCREEN_HEIGHT-1;
    for(int y=y1;y<=y2;y++)
        nxvga_setpixel(x,y,color);
}

static void nxvga_fillrect(int x,int y,int w,int h,byte color)
{
    if(x<0){ w+=x; x=0; }
    if(y<0){ h+=y; y=0; }
    if(x+w>SCREEN_WIDTH) w=SCREEN_WIDTH-x;
    if(y+h>SCREEN_HEIGHT) h=SCREEN_HEIGHT-y;
    if(w<=0 || h<=0) return;
    for(int py=0;py<h;py++)
        for(int px=0;px<w;px++)
            nxvga_setpixel(x+px,y+py,color);
}

static void nxvga_blit(int dx,int dy,int w,int h,const byte *src)
{
    for(int py=0;py<h;py++)
        for(int px=0;px<w;px++)
            nxvga_setpixel(dx+px,dy+py,src[py*w+px]);
}

/* ---------------- Subdriver Definition ---------------- */

static SUBDRIVER nxvga_subdriver = {
    nxvga_setpixel,
    nxvga_getpixel,
    nxvga_drawhorzline,
    nxvga_drawvertline,
    nxvga_fillrect,
    nxvga_blit,
    NULL,
    NULL,
    NULL
};

/* ---------------- Subdriver Array ---------------- */

SUBDRIVER *nxvga_drivers[] = {
    &nxvga_subdriver
};

/* ---------------- Open / Close ---------------- */

static int nxvga_open(void)
{
    if (vga_set_modeX() < 0) return 0;

    backbuf = malloc(SCREEN_PIXELS);
    if (!backbuf) return 0;

    bg_color = 0;
    memset(backbuf,bg_color,SCREEN_PIXELS);

    vga_update_region(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
    return 1;
}

static void nxvga_close(void)
{
    if(backbuf) free(backbuf);
    backbuf=NULL;

    /* restore 80x25 text mode */
    set_mode(0x03);
}
