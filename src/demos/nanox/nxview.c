/*
 * nxview - Nano-X image viewer
 *
 * Autorecognizes and displays BMP, GIF, JPEG, PNG and XPM files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

#define CX	16		/* pattern width*/
#define CY	16		/* pattern height*/

int pflag = 0;		/* -p for pattern background to test alpha/transparent draws*/
int sflag = 0;		/* -s for window half screen size*/

int
main(int argc,char **argv)
{
	int			t;
	GR_IMAGE_ID	image_id;
	GR_WINDOW_ID	window_id;
	GR_GC_ID	gc_id;
	GR_SIZE		w = -1;
	GR_SIZE		h = -1;
	GR_EVENT	event;
	GR_SCREEN_INFO	sinfo;
	GR_IMAGE_INFO	info;
	char		title[256];

	if (argc < 2) {
		GrError("Usage: nxview [-p] [-s] <image file>\n");
		return 1;
	}

	t = 1;
	while ( t < argc ) {
		if ( !strcmp("-p",argv[t])) {
			pflag = 1;
			++t;
			continue;
		}
		if ( !strcmp("-s",argv[t])) {
			sflag = 1;
			++t;
			continue;
		}
		break;
	}

	if (GrOpen() < 0) {
		GrError("cannot open graphics\n");
		return 1;
	}
	
	if (!(image_id = GrLoadImageFromFile(argv[t], 0))) {
		GrError("Can't load image file: %s\n", argv[t]);
		return 1;
	}

	if(sflag) {
		/* stretch to half screen size*/
		GrGetScreenInfo(&sinfo);
		w = sinfo.cols/2;
		h = sinfo.rows/2;
	} else {
		GrGetImageInfo(image_id, &info);
		w = info.width;
		h = info.height;
	}

	sprintf(title, "nxview %s", argv[t]);
	window_id = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, title, GR_ROOT_WINDOW_ID, 0, 0, w, h, GRAY);

	GrSelectEvents(window_id,
		GR_EVENT_MASK_CLOSE_REQ|GR_EVENT_MASK_EXPOSURE);

	GrMapWindow(window_id);

	gc_id = GrNewGC();

	while (1) {
		GrGetNextEvent(&event);
		switch(event.type) {
		case GR_EVENT_TYPE_CLOSE_REQ:
			GrDestroyWindow(window_id);
			GrDestroyGC(gc_id);
			GrFreeImage(image_id);
			GrClose();
			return 0;
			/* no return*/
		case GR_EVENT_TYPE_EXPOSURE:
			if (pflag) {
				int x, y;
				GR_WINDOW_INFO wi;

				GrGetWindowInfo(window_id, &wi);
				for (x=0; x<wi.width; x+=CX*2)
					for (y=0; y<wi.height; y+=CY) {
						if (y & CY)
							GrFillRect(window_id, gc_id, x, y, CX, CY);
						else GrFillRect(window_id, gc_id, x+CX, y, CX, CY); 
					}
			}
			GrDrawImageToFit(window_id, gc_id, 0,0, w,h, image_id);
			break;
		}
	}

	return 0;
}
