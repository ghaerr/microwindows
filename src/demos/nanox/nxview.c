/*
 * Copyright (c) 2000, 2001 Greg Haerr <greg@censoft.com>
 *
 * nxview - Nano-X image viewer
 *
 * Autorecognizes and displays BMP, GIF, JPEG, PNG and XPM files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

int
main(int argc,char **argv)
{
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
		printf("Usage: nxview <image file> [stretch]\n");
		exit(1);
	}

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}
	
	if (!(image_id = GrLoadImageFromFile(argv[1], 0))) {
		fprintf(stderr, "Can't load image file: %s\n", argv[1]);
		exit(1);
	}

	if(argc > 2) {
		/* stretch to half screen size*/
		GrGetScreenInfo(&sinfo);
		w = sinfo.cols/2;
		h = sinfo.rows/2;
	} else {
		GrGetImageInfo(image_id, &info);
		w = info.width;
		h = info.height;
	}

	sprintf(title, "nxview %s", argv[1]);
	window_id = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, title,
		GR_ROOT_WINDOW_ID, 0, 0, w, h, BLACK);

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
			exit(0);
			/* no return*/
		case GR_EVENT_TYPE_EXPOSURE:
			GrDrawImageToFit(window_id, gc_id, 0,0, w,h, image_id);
			break;
		}
	}

	return 0;
}
