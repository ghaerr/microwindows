/*
 * Microwindows direct client-side framebuffer mapping routines
 *
 * Copyright (c) 2001, 2002, 2003 by Greg Haerr <greg@censoft.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef LINUX
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#endif
#include "nano-X.h"
#include "lock.h"

/* For definition of PAGE_SIZE */
#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)

#define CG6_RAM    	0x70016000	/* for Sun systems*/

LOCK_EXTERN(nxGlobalLock);	/* global lock for threads safety*/

/* globals: assumes use of non-shared libnano-X.a for now*/
static unsigned char *	physpixels  = NULL;	/* start address of pixels*/
static GR_SCREEN_INFO	sinfo;
static GR_BOOL		sinfo_valid = GR_FALSE;	/* True if sinfo is initialized. */
#ifdef LINUX
static int 		frame_fd    = -1;	/* client side framebuffer fd*/
static unsigned char *	frame_map   = NULL;	/* client side framebuffer mmap'd addr*/
static int 		frame_len   = 0;	/* client side framebuffer length*/
#endif

/**
 * Map framebuffer address into client memory.
 *
 * @return Pointer to start of framebuffer,
 * or NULL if framebuffer not directly accessible by client.
 */
unsigned char *
GrOpenClientFramebuffer(void)
{
#ifdef LINUX
	int 	frame_offset;
	char *	fbdev;
	struct fb_fix_screeninfo finfo;

	LOCK(&nxGlobalLock);

	/* if already open, return fb address*/
	if (physpixels) {
		UNLOCK(&nxGlobalLock);
		return physpixels;
	}

	/*
	 * For now, we'll just check whether or not Microwindows
	 * is running its framebuffer driver to determine whether
	 * to allow direct client-side framebuffer mapping.  In
	 * the future, we could allow direct mapping for Microwindows
	 * running on top of X, and finding the address of the 
	 * window within the Microwindows X window.
	 */
	GrGetScreenInfo(&sinfo);
	sinfo_valid = GR_TRUE;
	if (!sinfo.fbdriver) {
		UNLOCK(&nxGlobalLock);
		return NULL;
	}

	/*
	 * Try to open the framebuffer directly.
	 */
	if (!(fbdev = getenv("FRAMEBUFFER")))
		fbdev = "/dev/fb0";
	frame_fd = open(fbdev, O_RDWR);
	if (frame_fd < 0) {
		printf("Can't open framebuffer device\n");
		UNLOCK(&nxGlobalLock);
		return NULL;
	}

	/* Get the type of video hardware */
	if (ioctl(frame_fd, FBIOGET_FSCREENINFO, &finfo) < 0 ) {
		printf("Couldn't get fb hardware info\n");
		goto err;
	}

	/* FIXME remove when mwin returns fb or X */
	switch (finfo.visual) {
		case FB_VISUAL_TRUECOLOR:
		case FB_VISUAL_PSEUDOCOLOR:
		case FB_VISUAL_STATIC_PSEUDOCOLOR:
		case FB_VISUAL_DIRECTCOLOR:
			break;
		default:
			printf("Unsupported fb color map\n");
			goto err;
	}

	/* Memory map the device, compensating for buggy PPC mmap() */
	frame_offset = (((long)finfo.smem_start) -
		(((long)finfo.smem_start)&~(PAGE_SIZE-1)));
	frame_len = finfo.smem_len + frame_offset;
	frame_map = (unsigned char *)mmap(NULL, frame_len, PROT_READ|PROT_WRITE,
		MAP_SHARED, frame_fd,
#ifdef ARCH_LINUX_SPARC
		CG6_RAM);
#else
		0);
#endif
	if (frame_map == (unsigned char *)-1) {
		printf("Unable to memory map the video hardware\n");
		frame_map = NULL;
		goto err;
	}
	physpixels = frame_map + frame_offset;
	UNLOCK(&nxGlobalLock);
	return physpixels;

err:
	close(frame_fd);
	UNLOCK(&nxGlobalLock);
#endif /* LINUX */
	return NULL;
}

/**
 * Unmap framebuffer, if mapped.
 */
void
GrCloseClientFramebuffer(void)
{
#ifdef LINUX
	LOCK(&nxGlobalLock);
	if (frame_fd >= 0) {
		if (frame_map) {
			munmap(frame_map, frame_len);
			frame_map = NULL;
			physpixels = NULL;
		}
		close(frame_fd);
		frame_fd = -1;

		/* reset sinfo struct*/
		sinfo_valid = GR_FALSE;
	}
	UNLOCK(&nxGlobalLock);
#endif
}

/**
 * Return client-side mapped framebuffer info for
 * passed window.  If not running framebuffer, the
 * physpixel and winpixel members will be NULL, and
 * everything else correct.
 *
 * @param wid    Window to query
 * @param fbinfo Structure to store results.
 */
void
GrGetWindowFBInfo(GR_WINDOW_ID wid, GR_WINDOW_FB_INFO *fbinfo)
{
	int			physoffset;
	int			x, y;
	GR_WINDOW_INFO		info;
	static int		last_portrait = -1;

	LOCK(&nxGlobalLock);

	/* re-get screen info on auto-portrait switch*/
	if (!sinfo_valid || (last_portrait != sinfo.portrait)) {
		GrGetScreenInfo(&sinfo);
		sinfo_valid = GR_TRUE;
	}
	last_portrait = sinfo.portrait;

	/* must get window position anew each time*/
	GrGetWindowInfo(wid, &info);

	fbinfo->bpp = sinfo.bpp;
	fbinfo->bytespp = (sinfo.bpp+7)/8;
	fbinfo->pixtype = sinfo.pixtype;
	fbinfo->portrait_mode = sinfo.portrait;

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
	fbinfo->physpixels = physpixels;

	/* winpixels only valid for non-portrait modes*/
	physoffset = fbinfo->y*fbinfo->pitch + fbinfo->x*fbinfo->bytespp;
	fbinfo->winpixels = physpixels? (physpixels + physoffset): NULL;

	UNLOCK(&nxGlobalLock);
}
