/*
 * scr_prsm.c
 *
 * Microwindows screen driver for the Isicad Prisma diskless workstation
 * by George Harvey.
 *
 * This is mostly 'glue' code, the real work is done by assembler routines
 * in asm_prsm.s.
 *
 * The Prisma has a custom graphics controller with a fixed resolution
 * 1280 x 1024, 256 color display. This would be great except that it
 * was desgined for CAD work, not graphical desktops. The only interface
 * to the the video display is through a set of control registers. These
 * support line drawing in hardware (any angle, any length, any colour)
 * but there is no way to access the video memory as a frame buffer.
 * There is a way to read and write individual pixels but it is VERY
 * slow so blitting is bad news.
 *
 * 26/02/00	first tests with Microwindows 0.87
 * 24/03/00	added blitter code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#ifdef _MINIX
#include <sys/types.h>
#endif
#include "genfont.h"
#include "genmem.h"

#define	MY_PSD_ADDR	0x01000000	/* dummy video memory addr */

/* VB driver entry points*/
static PSD  VB_open(PSD psd);
static void VB_close(PSD psd);
static void VB_getscreeninfo(PSD psd,PMWSCREENINFO psi);;
static void VB_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void VB_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL VB_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void VB_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y,
	 MWPIXELVAL c);
static void VB_drawvline(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2,MWPIXELVAL c);
static void VB_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
	MWPIXELVAL c);
static void VB_blit(PSD destpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
	PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);

/* low-level routines in asm_prsm.s */
void init_scr(void);
void c_pcmap(int cnt, int *map);
void c_ldraw4(int x1, int y1, int x2, int y2, int inten, int sel);
/* optimised block drawing routines */
void c_ldraw5y(int x1, int y1, int x2, int y2, int inten);
void c_ldraw5x(int x1, int y1, int x2, int y2, int inten);
int  r1pix3(int x, int y, int selmask);
void rd_rect2(int x1, int x2, int y1, int y2, unsigned char *buf);
void w1pix3(int x, int y, int selmask, int pixdata);
void wr_rect2(int x1, int x2, int y1, int y2, unsigned char *buf);

/* dummy routines for now */
static MWBOOL VB_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
	int linelen,int size,void *addr);


SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	VB_open,
	VB_close,
	VB_getscreeninfo,
	VB_setpalette,
	VB_drawpixel,
	VB_readpixel,
	VB_drawhline,
	VB_drawvline,
	VB_fillrect,
	gen_fonts,
	VB_blit,
	NULL,			/* preSelect */
	NULL,			/* DrawArea */
	NULL,			/* SetIOPermissions */
	gen_allocatememgc,
	VB_mapmemgc,
	gen_freememgc
};

static int *pr_cmap;		/* Prisma colour map */

static PSD
VB_open(PSD psd)
{
	/* allocate space for the colour map */
	if ((pr_cmap = (int *)malloc((size_t)(256 * sizeof(int)))) == NULL) {
		perror("malloc cmap");
		return(0);
	}
	memset(pr_cmap, 0, (256 * sizeof(int)));	/* clear colour map */

	/* init driver variables */
	psd->xres = psd->xvirtres = 1280;
	psd->yres = psd->yvirtres = 1024;
	psd->planes = 1;
	psd->bpp = 8;
	psd->ncolors = 256;
	psd->pixtype = MWPF_PALETTE;
#if HAVEBLIT
	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
#else
	psd->flags = PSF_SCREEN;
#endif
	psd->addr = (void *)MY_PSD_ADDR;	/* dummy addr. */
	psd->linelen = 1280;
	init_scr();				/* init VB and clear screen */
	printf("VB_open: finished open\n");	/* DEBUG */
	return psd;
}

static void
VB_close(PSD psd)
{
	/* should probably restore the default palette here */
	/* empty for now */
}

static void
VB_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = 1;

	/* 1280 x 1024 on a 16in monitor */
	psi->xdpcm = 40;	/* assumes screen width of 32 cm*/
	psi->ydpcm = 43;	/* assumes screen height of 24 cm*/
}

static void
VB_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	int i, ind;
	MWPALENTRY *p;

	ind = first;
	for (i = 0; i < count; i++) {
		p = &pal[i];
		if (ind > 255) {
			fprintf(stderr, "VB_setpalette: index out of range: %d\n", ind);
			return;
		}
		*(pr_cmap + ind) = (ind << 24) | (p->r << 16) | (p->g << 8) |
			(p->b);
		ind++;
	}
	/* to try and debug palette code in VNC... */
/*	if (count == 1) {
		printf("VB_setpalette: setting %d to %08x\n", first, \
			*(pr_cmap + first));
	}
 */
	/* write a maximum of 16 entries at a time, any more doesn't work
	 * reliably (no idea why not)
	 */
	ind = first;
	while (count) {
		if (count >= 16) {
			c_pcmap(16, (pr_cmap + ind));
			count -= 16;
			ind += 16;
		} else {
			c_pcmap(count, (pr_cmap + ind));
			count = 0;
		}
	}

}

static void
VB_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	w1pix3(x, y, 255, (int)c);
}

static MWPIXELVAL
VB_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	return((MWPIXELVAL)r1pix3(x, y, 255));
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
VB_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
/*	++x2;		/* draw final point*/
	y = 1023 - y;	/* flip y axis */
	c_ldraw4(x1, y, x2, y, (int)c, 255);
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
VB_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
/*	++y2;		/* draw final point*/
	y1 = 1023 - y1;
	y2 = 1023 - y2;
	c_ldraw4(x, y1, x, y2, (int)c, 255);
}

static void
VB_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	register int w, h;

	w = ((x2 - x1) >= 0) ? (x2 - x1) : (x1 - x2);	/* abs width */
	h = ((y2 - y1) >= 0) ? (y2 - y1) : (y1 - y2);	/* abs height */
	/* sanity check */
	if ((w > 1279) || (h > 1023))
		return;
	y1 = 1023 - y1;			/* flip origin to top corner */
	y2 = 1023 - y2;
	if (w >= h) {
		/* width is greater, draw horizontal lines */
#if 0
		while(y2 <= y1) {
			c_ldraw4(x1, y2, x2, y2, (int)c, 255);
			y2++;
		}
#endif
		c_ldraw5y(x1, y2, x2, y1, (int)c);
	} else {
		/* height is greater, draw vertical lines */
#if 0
		while(x1 <= x2) {
			c_ldraw4(x1, y2, x1, y1, (int)c, 255);
			x1++;
		}
#endif
		c_ldraw5x(x1, y2, x2, y1, (int)c);
	}
}

/*
 * Blitting is going to be very slow on the Prisma!
 */
static void
VB_blit(PSD destpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
        PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op)
{
	int x1s, x1e, x2s, x2e, y1s, y1e, y2s, y2e;
	unsigned char *rect;

/*	printf("VB_blit\n");			/* DEBUG */
	if ((rect = (unsigned char *)malloc((size_t)(w * h * sizeof(char))))
			== NULL) {
		perror("malloc rect");
		return;
        }
	/* always copy in +ve direction */
	x1s = (w >= 0) ? srcx : (srcx + w);
	x2s = (w >= 0) ? destx : (destx + w);
	x1e = (w >= 0) ? (srcx + w) : srcx;
	x2e = (w >= 0) ? (destx + w) : destx;
	y1s = (h >= 0) ? srcy : (srcy + h);
	y2s = (h >= 0) ? desty : (desty + h);
	y1e = (h >= 0) ? (srcy + h) : srcy;
	y2e = (h >= 0) ? (desty + h) : desty;

	/* determine which PSD is the screen */
	if ((srcpsd->addr == MY_PSD_ADDR) && (destpsd->addr == MY_PSD_ADDR)) {
		/* screen to screen copy */
		/* copy rectangle into buffer */
		rd_rect2(x1s, y1s, x1e, y1e, rect);
		/* copy rectangle out of buffer */
		wr_rect2(x2s, y2s, x2e, y2e, rect);
	} else if (srcpsd->addr == MY_PSD_ADDR) {
		/* screen to off-screen */
	} else if (destpsd->addr == MY_PSD_ADDR) {
		/* off-screen to screen */
	} else {
		/* error ! */
		printf("VB_blit with no screen!\n");
	}
	free(rect);
}

static MWBOOL
VB_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
	int size,void *addr)
{
	return 0;
}

