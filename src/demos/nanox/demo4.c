/*
 * Demonstration program for Nano-X blitting
 */
#include <stdio.h>
#include <stdlib.h>
#include "nano-X.h"
#include "device.h"

static	GR_GC_ID	gc1;		/* graphics context for text */
static	GR_SCREEN_INFO	si;		/* information about screen */

#define GRAY14		MWRGB( 17, 17, 17 )
#define GRAY13		MWRGB( 34, 34, 34 )
#define GRAY12		MWRGB( 51, 51, 51 )
#define GRAY11		MWRGB( 68, 68, 68 )
#define GRAY10		MWRGB( 85, 85, 85 )
#define GRAY9		MWRGB( 102, 102, 102 )
#define GRAY8		MWRGB( 119, 119, 119 )
#define GRAY7		MWRGB( 136, 136, 136 )
#define GRAY6		MWRGB( 153, 153, 153 )
#define GRAY5		MWRGB( 170, 170, 170 )
#define GRAY4		MWRGB( 187, 187, 187 )
#define GRAY3		MWRGB( 204, 204, 204 )
#define GRAY2		MWRGB( 221, 221, 221 )
#define GRAY1		MWRGB( 238, 238, 238 )

int
main(int argc,char **argv)
{
	GR_EVENT	event;		/* current event */

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}
	
	GrGetScreenInfo(&si);

	gc1 = GrNewGC();

	GrSetGCForeground(gc1, WHITE);
	/*GrFillRect(GR_ROOT_WINDOW_ID, gc, 0, 0, 240, 320);*/

	GrLine(GR_ROOT_WINDOW_ID, gc1, 4, 4, 634, 4);
	GrLine(GR_ROOT_WINDOW_ID, gc1, 4, 4, 4, 474);
	GrLine(GR_ROOT_WINDOW_ID, gc1, 634, 4, 634, 474);
	GrLine(GR_ROOT_WINDOW_ID, gc1, 4, 474, 634, 474);

	/*GrJPEG(GR_ROOT_WINDOW_ID, gc, 0, 0, 400, 400, "/home/mart/nov9_pic1.jpg");*/

/* Bitblit function */
{
	PSD	mempsd;
	int	linelen, size;
	void *	pixels;
	
	mempsd = scrdev.AllocateMemGC(&scrdev);
	GdCalcMemGCAlloc(mempsd, 50, 100, 0, 0, &size, &linelen);
	pixels = malloc(size);
	mempsd->flags |= PSF_ADDRMALLOC;
	mempsd->MapMemGC(mempsd, 50, 100, scrdev.planes, scrdev.bpp,
		linelen, size, pixels);

	/* Draw some stuff on offscreen */
	GdSetForeground(GdFindColor(GREEN));
	GdFillRect(mempsd, 0, 0, 50, 100);
	GdSetForeground(GdFindColor(RED));
	GdFillRect(mempsd, 5, 20, 40, 60);

	/* blit */
	GdBlit(&scrdev, 10, 10, 50, 100, mempsd, 0, 0, 0);
}
/*	GrSetGCForeground(gc, BLACK);
	GrPoint(GR_ROOT_WINDOW_ID, gc, 0, 0);

	GrSetGCForeground(gc, GRAY10);
	GrFillRect(GR_ROOT_WINDOW_ID, gc, 1, 1, 478, 638);
*/
	while (1) {
		GrGetNextEvent(&event);
		if(event.type == GR_EVENT_TYPE_CLOSE_REQ) break;
	}

	GrClose();

	return 0;
}
