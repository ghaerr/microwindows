/*
 * Demonstrates loading a binary PPM file and displaying it in a window
 * as a Pixmap.
 */

/* Comment this definition out if you don't want to use server side pixmaps */
/* (it will be slower but will work on device drivers without bitblt) */
#define USE_PIXMAPS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <nano-X.h>

GR_WINDOW_ID window;	/* ID for output window */
#ifdef USE_PIXMAPS
GR_WINDOW_ID pmap;	/* ID for pixmap */
#endif
GR_GC_ID gc;		/* Graphics context */
int width, height;	/* Size of image */
unsigned char *data;	/* Local copy of image data */

void do_exposure(GR_EVENT_EXPOSURE *event)
{
	/* The window has been exposed so redraw it */
#ifdef USE_PIXMAPS
	GrCopyArea(window, gc, 0, 0, width, height, pmap, 0, 0, MWROP_SRCCOPY);
#else
	GrArea(window, gc, 0, 0, width, height, data, MWPF_RGB);
#endif
}

void errorhandler(GR_EVENT *ep)
{
	printf("Error (%s) code %d id %d", ep->error.name, 
		ep->error.code, ep->error.id);
	exit(1);
}

int main(int argc, char *argv[])
{
	unsigned char line[256];
	GR_EVENT event;
	FILE *infile;
	int i, o;
	unsigned char *p;

	if(argc != 2) {
		printf("Usage: demo6 <filename.ppm>\n");
		exit(1);
	}

	if(!(infile = fopen(argv[1], "r"))) {
		printf("Couldn't open \"%s\" for reading: %s\n", argv[1],
						strerror(errno));
		exit(2);
	}

	/* Read magic number (P6 = colour, binary encoded PPM file) */
	if(!fgets(line, 256, infile)) goto truncated;
	if(line[0] != 'P' || line[1] != '6') {
		printf("Unsupported PPM type or not a PPM file.\n");
		printf("Please supply a valid P6 format file (colour, with "
			"binary encoding).\n");
	}

	/* Strip comments */
	do {
		if(!fgets(line, 256, infile)) goto truncated;
	} while(line[0] == '#');

	/* Read width and height */
	sscanf(line, "%i %i", &width, &height);

	/* Read the maximum colour value */
	if(!fgets(line, 256, infile)) goto truncated;
	sscanf(line, "%i", &i);
	if(i != 255) {
		printf("Truecolour mode only is supported\n");
		exit(4);
	}

	/* Calculate how many bytes of image data there is */
	i = width * height * 3;
	/* Calculate how many bytes of data there will be after unpacking */
	o = width * height * 4;

	/* Allocate the space to store the data whilst it's being loaded */
	if(!(data = malloc(o))) {
		printf("Not enough memory to load image\n");
		exit(5);
	}

	/* Read the data in and unpack it to RGBX format */
	/* The lower byte isn't used so we don't set it to anything */
	p = data;
	while(o) {
		if(fread(p, 1, 3, infile) != 3) goto truncated;
		p += 4;
		o -= 4;
	}

	/* We don't need the input file anymore so close it */
	fclose(infile);

	/* Register the error handler */
	GrSetErrorHandler(errorhandler);

	if(GrOpen() < 0) {
		printf("Couldn't connect to Nano-X server\n");
		exit(6);
	}

#ifdef USE_PIXMAPS
	/* Create the pixmap to store the picture in */
	pmap = GrNewPixmap(width, height, NULL);
#endif

	/* Create a graphics context */
	gc = GrNewGC();

#ifdef USE_PIXMAPS
	/* Copy the image data into the pixmap */
	GrArea(pmap, gc, 0, 0, width, height, data, MWPF_RGB);
	/* We can free the image data now because it's stored in the pixmap */
	free(data);
#endif

	/* Create a window to output the image to */
	window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, width, height, 0, 0, 0);

	/* Select expose events so we can redraw the image when necessary */
	GrSelectEvents(window, GR_EVENT_MASK_EXPOSURE |
				GR_EVENT_MASK_CLOSE_REQ);

	/* Make the window visible */
	GrMapWindow(window);

#ifdef USE_PIXMAPS
	/* Paint the pixmap onto it */
	GrCopyArea(window, gc, 0, 0, width, height, pmap, 0, 0,
							MWROP_SRCCOPY);
#else
	GrArea(window, gc, 0, 0, width, height, data, MWPF_RGB);
#endif

	while(1) {
		GrGetNextEvent(&event);
		switch(event.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				do_exposure(&event.exposure);
				break;
			case GR_EVENT_TYPE_CLOSE_REQ:
				GrClose();
				exit(0);
		}
	}

	return 0;

truncated:
	printf("Error: File appears to be truncated\n");
	exit(3);
}
