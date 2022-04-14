/*
 * Microwindows direct client-side framebuffer mapping routines
 *
 * Copyright (c) 2001, 2002, 2003, 2019 by Greg Haerr <greg@censoft.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include "uni_std.h"
#include "nano-X.h"
#include "device.h"		/* for MW_PATHs*/
#include "lock.h"
#if HAVE_MMAP
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif

LOCK_EXTERN(nxGlobalLock);	/* global lock for threads safety*/

typedef struct gr_clientfb GR_CLIENTFB;
struct gr_clientfb {
	GR_WINDOW_ID wid;
	void *mapaddr;
	int maplen;
	GR_CLIENTFB *next;
};

static GR_CLIENTFB *fbhead;

/*
 * Return a pointer to the clientfb structure with the specified window id.
 */
static GR_CLIENTFB *
GsFindClientFB(GR_WINDOW_ID wid)
{
	GR_CLIENTFB	*fp;

	for (fp = fbhead; fp; fp = fp->next) {
		if (fp->wid == wid)
			return fp;
	}
	return NULL;
}

/**
 * Map framebuffer or window buffer address into client memory.
 * @wid Window ID with GR_WM_PROPS_BUFFER_MMAP set, or 0 for entire framebuffer.
 *
 * @return Pointer to start of memory mapped area,
 * or NULL if framebuffer not directly accessible by client.
 */
unsigned char *
GrOpenClientFramebuffer(GR_WINDOW_ID wid)
{
#if HAVE_MMAP
	GR_CLIENTFB *fp;
	int fd, maplen;
	void *mapaddr;
	char *env;
	GR_WINDOW_INFO info;
	GR_SCREEN_INFO sinfo;
	char path[256];

	LOCK(&nxGlobalLock);

	/* if already open, return fb address*/
	fp = GsFindClientFB(wid);
	if (fp) {
		UNLOCK(&nxGlobalLock);
		return fp->mapaddr;
	}

	if (wid == 0) {
		/* Map entire framebuffer, get size of screen memory*/
		GrGetScreenInfo(&sinfo);
		if (sinfo.size == 0) {
			UNLOCK(&nxGlobalLock);
			return NULL;
		}
		maplen = sinfo.size;

		/* Open framebuffer or FBE device*/
		if((env = getenv("FRAMEBUFFER")) != NULL)
			fd = open(env, O_RDWR);
		else {
			/* try /dev/fb0 then FBE /tmp/fb0*/
			fd = open(MW_PATH_FRAMEBUFFER, O_RDWR);
			if (fd < 0)
				fd = open(MW_PATH_FBE_FRAMEBUFFER, O_RDWR);
		}
		if (fd < 0) {
			EPRINTF("GrOpenClientFramebuffer: Can't open framebuffer device\n");
			UNLOCK(&nxGlobalLock);
			return NULL;
		}
	} else {
		/* Map window buffer*/
		GrGetWindowInfo(wid, &info);
		if (info.bufsize == 0) {
			UNLOCK(&nxGlobalLock);
			return NULL;
		}
		maplen = info.bufsize;

		/* Open /tmp/.nano-fb%d mmaped buffer using window ID*/
		sprintf(path, MW_PATH_BUFFER_MMAP, wid);
		fd = open(path, O_RDWR);
		if (fd < 0) {
			EPRINTF("GrOpenClientFramebuffer: Can't open mmap window buffer %s\n", path);
			UNLOCK(&nxGlobalLock);
			return NULL;
		}
	}

	/* Memory map the device*/
	mapaddr = (unsigned char *)mmap(NULL, maplen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (mapaddr == (unsigned char *)-1) {
		EPRINTF("GrOpenClientFramebuffer: Can't mmap frame or window buffer\n");
		UNLOCK(&nxGlobalLock);
		return NULL;
	}

	fp = (GR_CLIENTFB *)malloc(sizeof(GR_CLIENTFB));
	if (fp == NULL) {
		munmap(mapaddr, maplen);
		return NULL;
	}

	fp->wid = wid;
	fp->mapaddr = mapaddr;
	fp->maplen = maplen;
	fp->next = fbhead;
	fbhead = fp;
	UNLOCK(&nxGlobalLock);
	return fp->mapaddr;

#else /* !HAVE_MMAP*/
	return NULL;
#endif
}

/**
 * Unmap client framebuffer, if mapped.
 */
void
GrCloseClientFramebuffer(GR_WINDOW_ID wid)
{
#if HAVE_MMAP
	GR_CLIENTFB *fp, *prevfp;

	LOCK(&nxGlobalLock);
	fp = GsFindClientFB(wid);
	if (fp == NULL) {
		UNLOCK(&nxGlobalLock);
		return;
	}

	munmap(fp->mapaddr, fp->maplen);

	/* unlink from clientfb list*/
	prevfp = fbhead;
	if (prevfp == fp)
		fbhead = fp->next;
	else {
		while (prevfp->next != fp)
			prevfp = prevfp->next;
		prevfp->next = fp->next;
	}

	free(fp);
	UNLOCK(&nxGlobalLock);
#endif
}

/**
 * Return client-side mapped mmap address info for framebuffer or
 * window buffer. If not running framebuffer, the
 * physpixel and winpixel members will be NULL.
 *
 * @param wid    Window to query or 0 for entire framebuffer.
 * @param fbinfo Structure to store results.
 */
void
GrGetWindowFBInfo(GR_WINDOW_ID wid, GR_WINDOW_FB_INFO *fbinfo)
{
	int			x, y;
	GR_CLIENTFB *fp;
	GR_BOOL		entire_framebuffer = 0;
	GR_WINDOW_INFO info;
	GR_SCREEN_INFO sinfo;

	LOCK(&nxGlobalLock);
	if (wid == 0)			/* window 0 would fail on GrGetWindowInfo below*/
		wid = GR_ROOT_WINDOW_ID;

	/* first check for GR_WM_PROPS_BUFFER_MMAP mmaped window*/
	fp = GsFindClientFB(wid);
	if (!fp) {
		/* then check for entire framebuffer mapping, stored as window 0*/
		fp = GsFindClientFB(0);
		if (fp)
			entire_framebuffer = 1;
	}

	GrGetScreenInfo(&sinfo);
	GrGetWindowInfo(wid, &info);

	fbinfo->bpp = sinfo.bpp;
	fbinfo->bytespp = (sinfo.bpp+7)/8;
	fbinfo->pixtype = sinfo.pixtype;
	fbinfo->portrait_mode = sinfo.portrait;

	if (entire_framebuffer) {
		switch (fbinfo->portrait_mode) {
		case MWPORTRAIT_RIGHT:
		case MWPORTRAIT_LEFT:
			/*
			 * We reverse coords since Microwindows reports
			 * back the virtual xres/yres, and we want
			 * the physical xres/yres.
			 */
			/* FIXME return xres and xvirtres in SCREENINFO? */
			fbinfo->xres = sinfo.rows;	/* reverse coords*/
			fbinfo->yres = sinfo.cols;
			break;
		default:
			fbinfo->xres = sinfo.cols;
			fbinfo->yres = sinfo.rows;
			break;
		}
		fbinfo->xvirtres = sinfo.cols;
		fbinfo->yvirtres = sinfo.rows;
	} else {
		fbinfo->xres = fbinfo->xvirtres = info.width;
		fbinfo->yres = fbinfo->yvirtres = info.height;
	}
	fbinfo->pitch = fbinfo->xres * fbinfo->bytespp;

	/* calc absolute window coords*/
	x = info.x;
	y = info.y;
	while (info.parent != 0) {
		GrGetWindowInfo(info.parent, &info);
		x += info.x;
		y += info.y;
	}
	fbinfo->x = x;
	fbinfo->y = y;

	/* fill in memory mapped addresses*/
	fbinfo->physpixels = fp? fp->mapaddr: NULL;

	if (entire_framebuffer) {
		/* Winpixels is offset into framebuffer for current window position,
		 * and only valid for non-portrait modes.
		 */
		int physoffset = fbinfo->y*fbinfo->pitch + fbinfo->x*fbinfo->bytespp;
		fbinfo->winpixels = fp? ((unsigned char *)fp->mapaddr + physoffset): NULL;
	} else {
		/* No offset for window buffer, both addresses are buffer address*/
		fbinfo->winpixels = fbinfo->physpixels;
	}

	UNLOCK(&nxGlobalLock);
}
