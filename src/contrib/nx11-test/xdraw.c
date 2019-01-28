#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xutil.h>
#include <math.h>
#include <unistd.h>

int main (argc, argv)
int argc;
char **argv;
{
Display			*myDisplay;
int			myScreen;
int 			myDepth;
XSetWindowAttributes	myWindowAttributes;
unsigned long		myWindowMask;
Window			myWindow;
XSizeHints		theSizeHints;
GC			myGC;
unsigned long		myWhitePixel;
unsigned long		myBlackPixel;
XGCValues		myGCValues;
unsigned long		myValueMask;
int			i;
double			twopi= 8.0*atan(1.0);

int                     x 	= 200;		/* x top left corner of window */
int                     y 	= 150;		/* y top left corner of window */
unsigned int            width 	= 850;		/* width of the window */
unsigned int            height 	= 700;		/* height of the window */
int 			border_width = 20;	/* border width of the window */
/*--------------- Pre-assigned points for XDrawLines call below---------------*/
static XPoint	points1[]=
	{
	 499,214,489,224,484,229,479,239,474,254,479,274,489,279,499,279,
	 509,274,519,264
	};
static XPoint	points2[]=
	{
	499,284,504,289,509,289,514,289,519,284
	};
static XPoint	points3[]=
	{
	514,214,509,214,514,219,519,219,514,214
	};
static XPoint	points4[]=
	{
	 484,209,479,214,479,214,484,214,489,209,484,209
	};
static XPoint	points5[]=
	{
	 514,199,529,214,539,224,539,244,539,259,539,264,534,279,534,274
	};
static XPoint	points6[]=
	{
	 519,194,529,204,544,219,544,239
	};
static XPoint	points7[]=
	{
	 484,194,494,194,504,199,509,199
	};
static XPoint	points8[]=
	{
	 489,189,494,189,509,189
	};
static XPoint	points9[]=
	{
	 494,179,509,179,514,184,514,189
	};
static XPoint	points10[]=
	{
	 524,189,534,199,549,209,554,229,549,249,549,259,549,279,549,284
	};
/*--------End of  Pre-assigned points for XDrawLines call below---------------*/

	myDisplay = XOpenDisplay ("");
	if (myDisplay == NULL)
	{
		fprintf (stderr, 
			"ERROR: Could not open a connection to X on display %s\n",
		XDisplayName (NULL));
		exit (0);
	}

	myScreen = DefaultScreen (myDisplay);

	myDepth = DefaultDepth (myDisplay, myScreen);

	myWhitePixel = WhitePixel (myDisplay, myScreen);
	myBlackPixel = BlackPixel (myDisplay, myScreen);

	/* border colour */
	myWindowAttributes.border_pixel = BlackPixel (myDisplay, myScreen);

	/* background colour */
	myWindowAttributes.background_pixel = WhitePixel (myDisplay, myScreen);

	/* if window manager intervenes or not */
	myWindowAttributes.override_redirect = True;
	
	/* create mask for attributes */
	myWindowMask = CWBackPixel | CWBorderPixel | CWOverrideRedirect;

	myWindow = XCreateWindow (myDisplay, 
				RootWindow (myDisplay, myScreen),
				x, y, width, height, border_width,
				myDepth, InputOutput, CopyFromParent,
				myWindowMask, &myWindowAttributes);

	theSizeHints.flags      = PPosition | PSize;    /* set mask for the hints */
	theSizeHints.x          = x;                    /* x position */
	theSizeHints.y          = y;                    /* y position */
	theSizeHints.width      = width;                /* width of the window */
	theSizeHints.height     = height;               /* height of the window */

	XSetNormalHints (myDisplay, myWindow, &theSizeHints);

	myGC = XCreateGC (myDisplay,
                           myWindow,
                           (unsigned long) 0,
                           &myGCValues);

	/* error... cannot create gc */
	if (myGC == 0)
	{
		XDestroyWindow(myDisplay, myScreen);
		exit (0);
	}

	/* set forground and background defaults */
	else
	{
		XSetForeground (myDisplay, myGC, myBlackPixel);
		XSetBackground (myDisplay, myGC, myWhitePixel);
	}

	XMapWindow (myDisplay, myWindow);
	
	XDrawRectangle (myDisplay, myWindow, myGC,
		100, 100, 50, 50 );

	XFillRectangle (myDisplay, myWindow, myGC,
		200, 100, 50, 50 );

	/* draws a circle */
	XDrawArc (myDisplay, myWindow, myGC,
		300, 100, 50, 50 , 0, 360*64);

	XFillArc (myDisplay, myWindow, myGC,
		400, 100, 50, 50 , 0, 360*64);

	/* draws an oval */
	XDrawArc (myDisplay, myWindow, myGC,
		500,100, 60, 40 , 0, 360*64);

	XFillArc (myDisplay, myWindow, myGC,
		600,100, 60, 40 , 0, 360*64);

	/* draws an arc */
	XDrawArc (myDisplay, myWindow, myGC,
                100, 300, 60, 40 , 90*64, 180*64);

	XFillArc (myDisplay, myWindow, myGC,
                200, 300, 60, 40 , 90*64, 180*64);

#define RAYS 10
	for(i=0;i<RAYS/2;i++)
		XDrawLine(myDisplay, myWindow, myGC,
				325+(int)25*cos(twopi/RAYS*i),
				325+(int)25*sin(twopi/RAYS*i),
				325+(int)25*cos(twopi/2.0+twopi/RAYS*i),
				325+(int)25*sin(twopi/2.0+twopi/RAYS*i));

	XDrawLines(myDisplay, myWindow, myGC,
			points1,sizeof(points1)/sizeof(points1[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points2,sizeof(points2)/sizeof(points2[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points3,sizeof(points3)/sizeof(points3[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points4,sizeof(points4)/sizeof(points4[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points5,sizeof(points5)/sizeof(points5[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points6,sizeof(points6)/sizeof(points6[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points7,sizeof(points7)/sizeof(points7[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points8,sizeof(points8)/sizeof(points8[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points9,sizeof(points9)/sizeof(points9[0]),CoordModeOrigin);
	XDrawLines(myDisplay, myWindow, myGC,
			points10,sizeof(points10)/sizeof(points10[0]),CoordModeOrigin);

	XFlush (myDisplay);

/*-----------------------------------------------------------------------------
	SLEEP FOR 5 SECONDS
-----------------------------------------------------------------------------*/

#if EMSCRIPTEN
	extern void		GrDelay(uint32_t msecs);
	GrDelay(5000);		// required to draw XFlush'd graphics, can only call GrDelay from main()
#else
	sleep(5);
#endif

/*-----------------------------------------------------------------------------
	DESTROY ALL WINDOWS
-----------------------------------------------------------------------------*/

	XDestroyWindow (myDisplay, myWindow);
	XCloseDisplay (myDisplay);

	return 0;
}
