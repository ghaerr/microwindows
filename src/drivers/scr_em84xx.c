/*
 * Copyright (c) 2002 Peter Hartshorn <peter@signal3.com>
 *
 * Experimental em8400 screen driver for Microwindows
 *
 * 07/07/2002 Peter Hartshorn
 * EM8400 screen driver, because I cannot use the VGA overlay cable.
 * I figure, if I can't have harware mpeg on my desktop, I will have
 * a desktop on my mpeg hardware :)
 *
 */

/* USE_FAST_UPDATE
 * define this to use the faster updating.
 * FIXME the faster method is broken. Y works for all x,y,w,h
 * but UV broken for x,w,y,h != 0,0,WIDTH,HEIGHT
 */

/* #define USE_FAST_UPDATE */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "device.h"
#include "genfont.h"
#include "genmem.h"

#define _DEFINE_FMP_TYPES_
#include "fmp.h"

#define WIDTH 720
#define HEIGHT 480
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char* PBYTE;

#define IID_IOSD 0x00000000

typedef struct tagOSD_WND
{
        LONG x;
        LONG y;
        LONG w;
        LONG h;
} OSD_WND;

typedef struct tagIOsd
{
        struct tagIOsdVtbl *lpVtbl;
} IOsd;

typedef struct tagIOsdVtbl
{
        ULONG (*On)(IOsd *This);
        ULONG (*Off)(IOsd *This);
        ULONG (*Flush)(IOsd *This);
        ULONG (*Write)(IOsd *This, BYTE *pBuf, ULONG BufLen);
        ULONG (*SetDest)(IOsd *This, OSD_WND *pWnd);
        ULONG (*SetHli)(IOsd *This, OSD_WND *pWnd);
        ULONG (*ShowSplash)(IOsd *This, PBYTE buf, DWORD width, DWORD height);
        ULONG (*SetFrameBuffer)(IOsd *This, DWORD PhysicalAddress, DWORD Length);
        ULONG (*ShowSplashEx)(IOsd *This, PBYTE buf, DWORD width, DWORD height, int x, int y);
} IOsdVtbl;

typedef unsigned char *            ADDR8;

static IOsd *pIOsd = NULL;
static unsigned char yuv_data[WIDTH*HEIGHT*3/2];
static unsigned char rgb_data[WIDTH*HEIGHT*3];

/* EM8400 driver entry points*/
static PSD  EM8400_open(PSD psd);
static void EM8400_close(PSD psd);
static void EM8400_getscreeninfo(PSD psd,PMWSCREENINFO psi);;
static void EM8400_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void EM8400_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
static MWPIXELVAL EM8400_readpixel(PSD psd,MWCOORD x, MWCOORD y);
static void EM8400_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void EM8400_drawvline(PSD psd,MWCOORD x,MWCOORD y1,MWCOORD y2,MWPIXELVAL c);
static void EM8400_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
		MWPIXELVAL c);
static void EM8400_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
		PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op);
static MWBOOL EM8400_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
                int linelen,int size,void *addr);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	EM8400_open,
	EM8400_close,
	EM8400_getscreeninfo,
	EM8400_setpalette,
	EM8400_drawpixel,
	EM8400_readpixel,
	EM8400_drawhline,
	EM8400_drawvline,
	EM8400_fillrect,
	gen_fonts,
	EM8400_blit,
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
	EM8400_mapmemgc,		/* MapMemGC*/
	gen_freememgc		/* FreeMemGC*/
};

#define printd(_a)

#define DRAWON draw_lock++;if(debug)fprintf(stderr, "ENTER %s\n", __PRETTY_FUNCTION__);
#define DRAWOFF draw_lock--;if(debug)fprintf(stderr, "LEAVE %s\n", __PRETTY_FUNCTION__);

#ifndef USE_FAST_UPDATE
static pthread_t display_thread;
static pthread_cond_t display_update_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t display_update_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static unsigned char quit = 0;
static int debug = 0;
static int draw_lock = 0;

static void show_splash(int ux, int uy, int uw, int uh)
{
  int x, y;
  unsigned char *iptr, *optry, *optruv;
  unsigned long r1, g1, b1, y1, y2=0, u=0, v=0, u0=0, u1, u2, v0=0, v1, v2;
  unsigned long r2, g2, b2;

  if (pIOsd == NULL) return;

#ifdef USE_FAST_UPDATE
  if (draw_lock != 1) return;
#endif

  uw+=1; uh+=1;
  if (uw + ux > WIDTH) uw = WIDTH-ux;
  if (uh + uy > HEIGHT) uh = HEIGHT-uy;
  optry = yuv_data;
  optruv = yuv_data + uw*uh;
  iptr = rgb_data + (ux+uy*WIDTH)*3;
  
  for (y = uy; y < uy+uh; y++) {
    for (x = ux; x < ux+uw; x+=2) {
      r1 = *iptr++;
      g1 = *iptr++;
      b1 = *iptr++;
      r2 = *iptr++;
      g2 = *iptr++;
      b2 = *iptr++;

      y1 = 16829 * r1 + 33039 * g1 + 6416 * b1 + (0xffff & y2);
      y2 = 16829 * r2 + 33039 * g2 + 6416 * b2 + (0xffff & y1);

      *optry++ = (y1 >> 16) + 16;
      *optry++ = (y2 >> 16) + 16;

      if (!(y % 2)) {
        u1 = -4853 * r1 - 9530 * g1 + 14383 * b1;
        v1 = 14386 * r1 - 12046 * g1 - 2340 * b1;
        u2 = -2426 * r2 - 4765 * g2 + 7191 * b2;
        v2 = 7193 * r2 - 6023 * g2 - 1170 * b2;
        u = u0 + u1 + u2 + (0xffff & u);
        v = v0 + v1 + v2 + (0xffff & v);
        u0 = u2;
        v0 = v2;


        *optruv++ = (u >> 16) + 128;
        *optruv++ = (v >> 16) + 128;
      }
    }
    iptr += (WIDTH-uw)*6;
  }
  pIOsd->lpVtbl->ShowSplashEx(pIOsd, yuv_data, uw, uh, ux, uy);
}

void *thread_loop(void *data)
{
  while (!quit) {
	pthread_cond_wait(&display_update_cond, &display_update_mutex);
	show_splash(0,0,WIDTH-1,HEIGHT-1);
	usleep(10);
  }
  return 0;
}

static PSD
EM8400_open(PSD psd)
{
	DWORD tv_setting, old_setting;

	/* init driver variables depending on ega/vga mode*/
	psd->xres = psd->xvirtres = WIDTH;
	psd->yres = psd->yvirtres = HEIGHT;
	psd->planes = 1;
	psd->bpp = 24;
	psd->ncolors = 1<<24;
	psd->pixtype = MWPF_TRUECOLOR888;
	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
	psd->linelen = WIDTH;
	psd->addr = rgb_data;

	memset(rgb_data, 0x00, WIDTH*HEIGHT*3);
	memset(yuv_data, 0x00, WIDTH*HEIGHT*3/2);
	MPEGDriverEntry(NO_DRIVE);

	/* For some stupid reason, we cannot set the output to PAL
	 * unless we actually open the player.
	 */
	FMPOpen(FMPF_SYSTEM, 32767, 8, NULL, 0);

	old_setting = FMPGet(FMPI_VIDEOOUT);
	tv_setting = old_setting&(~FMPV_VIDEOOUT_STANDARDTV_MASK);
	tv_setting |= FMPV_VIDEOOUT_PAL;
	FMPSet(FMPI_VIDEOOUT,tv_setting);
	FMPClose();

	if (FMPQueryInterface(IID_IOSD, (void**)&pIOsd)) {
	  fprintf(stderr, "not open\n");
	  pIOsd = NULL;
  	}

#ifdef USE_FAST_UPDATE
	show_splash(0,0,WIDTH,HEIGHT);
#else
	pthread_create(&display_thread, NULL, thread_loop, NULL);
	pthread_cond_signal(&display_update_cond);
#endif
	return psd;
}

static void
EM8400_close(PSD psd)
{
	quit = 1;
#ifndef USE_FAST_UPDATE
	pthread_cond_signal(&display_update_cond);
	pthread_join(display_thread, NULL);
#endif
	MPEGDriverUnload();
	printd("EM8400_close()\n");
}

static void
EM8400_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = 1;

	/* EM8400 640xHEIGHT*/
	psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	printd("EM8400_getscreeninfo()\n");
}

static void
EM8400_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	printd("EM8400_setpalette()\n");
}

static void
EM8400_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8 addr = psd->addr;
	MWUCHAR r, g, b;

	DRAWON;
	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);

	addr += (x+y*psd->linelen) * 3;

	*addr++ = r;
	*addr++ = g;
	*addr++ = b;
#ifdef USE_FAST_UPDATE
	show_splash(x, y, 1, 1);
#else
	pthread_cond_signal(&display_update_cond);
#endif
	DRAWOFF;
}

static MWPIXELVAL
EM8400_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	ADDR8 addr = psd->addr;

	addr += (x+y*psd->linelen) * 3;
	return RGB2PIXEL(addr[0], addr[1], addr[2]);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
EM8400_drawhline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8 addr = psd->addr;
	MWUCHAR r, g, b;
#ifdef USE_FAST_UPDATE
	int x = x1;
#endif

	DRAWON;
	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);

	addr += (x1+y*psd->linelen)*3;

	while(x1++ <= x2) {
		*addr++ = r;
		*addr++ = g;
		*addr++ = b;
	}
#ifdef USE_FAST_UPDATE
	show_splash(x, y, x2 - x, 1);
#else
	pthread_cond_signal(&display_update_cond);
#endif
	DRAWOFF;
}


/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
EM8400_drawvline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8 addr = psd->addr;
	int linelen = psd->linelen * 3;
	MWUCHAR r, g, b;
#ifdef USE_FAST_UPDATE
	int y = y1;
#endif

	DRAWON;
	r = PIXEL888RED(c);
	g = PIXEL888GREEN(c);
	b = PIXEL888BLUE(c);

	addr += (x+y1*psd->linelen)*3;

	while (y1++ <= y2) {
		addr[0] = r;
		addr[1] = g;
		addr[2] = b;
		addr += linelen;
	}
#ifdef USE_FAST_UPDATE
	show_splash(x, y, 1, y2 - y);
#else
	pthread_cond_signal(&display_update_cond);
#endif
	DRAWOFF;
}

static void
EM8400_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
#ifdef USE_FAST_UPDATE
	int y = y1;
#endif
	DRAWON;
        while(y1 <= y2)
                psd->DrawHorzLine(psd, x1, x2, y1++, c);
#ifdef USE_FAST_UPDATE
	show_splash(x1, y, x2-x1, y2-y);
#else
	pthread_cond_signal(&display_update_cond);
#endif
	DRAWOFF;

}

void
EM8400_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR8 dst = dstpsd->addr;
	ADDR8 src = srcpsd->addr;
	int dlinelen = dstpsd->linelen * 3;
	int slinelen = srcpsd->linelen * 3;

	DRAWON;
	dst += (dstx+dsty*dstpsd->linelen)*3;
	src += (srcx+srcy*srcpsd->linelen)*3;

	if (srcy < dsty) {
		src += (h - 1) * slinelen;
		dst += (h - 1) * dlinelen;
		slinelen *= -1;
		dlinelen *= -1;
	}

	while(--h >=0) {
		memmove(dst, src, w*3);
		dst += dlinelen;
		src += slinelen;
	}
#ifdef USE_FAST_UPDATE
	show_splash(dstx, dsty, w, h);
#else
	pthread_cond_signal(&display_update_cond);
#endif
	DRAWOFF;
}

static MWBOOL
EM8400_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
	int linelen,int size,void *addr)
{
	extern SUBDRIVER fblinear24;

        initmemgc(mempsd, w, h, planes, bpp, linelen, size, addr);

        /* set and initialize subdriver into mem screen driver*/
        if (!set_subdriver(mempsd, &fblinear24, TRUE))
                return 0;

        return 1;
}

