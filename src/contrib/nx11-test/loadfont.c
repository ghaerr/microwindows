#include <stdlib.h>
#include <stdio.h>
#include "X11/X.h"
#include "X11/Xlib.h"
/*
 * NXLIB XLoadFont demonstration
 *
 * Loads a scaleable and non-scaleable font to test X11_FONT_DIRs
 * and parsing pixel height for scaleable fonts
 */

#define FONT1 "6x10" /*6x10.pcf.gz*/
#define FONT2 "-misc-fixed-medium-r-normal--10-100-75-75-c-60-iso10646-1" /*6x10.pcf.gz*/
#define FONT3 "-b&h-Luxi Mono-bold-i-normal--24-0-0-0-m-0-iso8859-1"	/*luximbi.ttf*/
#define FONT4 "-adobe-helvetica-bold-r-normal--14-100-100-100-p-82-iso8859-1" /* helvB10*/
#define FONT5 "-adobe-helvetica-bold-r-normal--14-0-0-0-p-0-iso8859-1" /* arialb*/

int
main(int ac, char **av)
{
	Display *d;
	Font 	f;

	d = XOpenDisplay(NULL);
	if (!d)
		exit(1);

	f = XLoadFont(d, FONT1);	/* 6x10 prefix*/
	printf("Font1 number:%ld is %s\n",f,FONT1);
	XUnloadFont(d, f);
	f = XLoadFont(d, FONT2);	/* non-scaled*/
	printf("Font2 number:%ld is %s\n",f,FONT2);
	XUnloadFont(d, f);
	f = XLoadFont(d, FONT3);	/* scaled, pixelsize 24*/
	printf("Font3 number:%ld is %s\n",f,FONT3);
	XUnloadFont(d, f);
	f = XLoadFont(d, FONT4);	/* FLTK helvB10 non-scaled*/
	printf("Font4 number:%ld is %s\n",f,FONT4);
	XUnloadFont(d, f);
	f = XLoadFont(d, FONT5);	/* sample fonts.dir adobe TTF*/
	printf("Font5 number:%ld is %s\n",f,FONT5);
	XUnloadFont(d, f);

	return 0;
}
