#include <stdlib.h>
#include <stdio.h>
#include "X11/X.h"
#include "X11/Xlib.h"
/*
 * NXLIB XListFonts demonstration
 *
 * calls XListFonts and prints the font names returned
 */

/* This are the fonts FLTK can use (in fl_font_x.cxx) */

//#define FONT "-*-helvetica-medium-r-normal--*"
//#define FONT "-*-helvetica-bold-r-normal--*" 
//#define FONT "-*-helvetica-medium-o-normal--*" 
//#define FONT "-*-helvetica-bold-o-normal--*" 
//#define FONT "-*-courier-medium-r-normal--*" 
//#define FONT "-*-courier-bold-r-normal--*" 
//#define FONT "-*-courier-medium-o-normal--*" 
#define FONT "-*-courier-bold-o-normal--*" 
//#define FONT "-*-times-medium-r-normal--*" 
//#define FONT "-*-times-bold-r-normal--*" 
//#define FONT "-*-times-medium-i-normal--*" 
//#define FONT "-*-times-bold-i-normal--*" 
//#define FONT "-*-symbol-*" 
//#define FONT "-*-lucidatypewriter-medium-r-normal-sans-*" 
//#define FONT "-*-lucidatypewriter-bold-r-normal-sans-*" 
//#define FONT "-*-*zapf dingbats-*"
//#define FONT "*helv*"
//#define FONT "*Dejavu*"
//#define FONT "-misc-cantarell-bold-r-normal--0-0-0-0-p-0-iso8859-1"

int
main(int ac, char **av)
{
	Display *d;
	int i;
	int num_fonts;               /* number of fonts X server found */
	char **fontlist;             /* array of strings (available font names) */

	d = XOpenDisplay(NULL);
	if (!d)
		exit(1);
	
	/* Get the names of all fonts that match FONT */
	fontlist = XListFonts (d, FONT, 1000, &num_fonts);
	for (i=0;i<num_fonts;i++) printf("Font number: %d | XLFD Font name: %s\n",i,fontlist[i]);

	XFreeFontNames (fontlist);
	
	return 0;
}
