#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef TEST_FOR_X
#include <X11/Xlib.h>
#else
#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#endif

#define NUM_POINTS	80

time_t start_time, end_time;

void start_timer()
{
	time(&start_time);
}

void end_timer()
{
	time(&end_time);
	printf("start=%lu, end=%lu, time=%lu\n", 
		start_time, end_time, end_time-start_time);
}
	
int main()
{
#ifdef TEST_FOR_X
	Display		*display;
	Window		window;
	GC		gc;
	XGCValues	gcValues;
	Colormap	colormap;
	Pixmap		src_pixmap;
	unsigned long	fgColor, bgColor;
	int		screenNum;
	XPoint		points[NUM_POINTS];
#else
	GR_WINDOW_ID 	window;
	GR_WINDOW_ID	src_pixmap;
	unsigned char*	src_pixmap_buf[320*240*2];
	GR_GC_ID        gc;
	GR_POINT	points[NUM_POINTS];
#endif
	
	int		c, c1,  count=4500;
	int		x, y, x1, y1, x2, y2;

#ifdef TEST_FOR_X
	if(!(display=XOpenDisplay(""))) {
		printf("Cannot connect to X.\n");
	}
	screenNum = DefaultScreen(display);
	colormap = DefaultColormap(display, screenNum);

	bgColor = BlackPixel(display, screenNum);
	fgColor = WhitePixel(display, screenNum);
	window = XCreateSimpleWindow(display, RootWindow(display, screenNum),
		0, 0 , 639, 479, 0,
		fgColor, bgColor);
	src_pixmap = XCreatePixmap(display, window, 320, 240, 16);
	XMapRaised(display, window);
	gcValues.background = bgColor;
	gcValues.foreground = fgColor;
	gcValues.line_width = 1;
	gcValues.line_style = LineSolid;
	gcValues.fill_style = FillSolid;
	gcValues.fill_rule = WindingRule;
	gcValues.arc_mode = ArcPieSlice;
	gc = XCreateGC(display, window,
		GCForeground  | GCBackground | GCLineWidth | GCLineStyle |
		GCFillStyle,
		&gcValues);

#else	
	GrOpen();
	window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, 639, 479, 0, BLACK, BLUE);
	src_pixmap = GrNewPixmap(640, 480, src_pixmap_buf);
	GrMapWindow(window);
	gc = GrNewGC();
        GrSetGCForeground(gc, WHITE);
	GrSetGCBackground(gc, BLACK);
	GrSetGCMode(gc, GR_MODE_COPY);
#endif




	
	// Horizontal Line
	////////////////////////////////////////////////
	printf("Horizontal Line(XDrawLine)\n");
	start_timer();
	for(c=0; c<count*20; c++)
	{
		y1=random()%480;
#ifdef TEST_FOR_X
		XDrawLine(display, window, gc, 0, y1, 639, y1);
		XFlush(display);
#else
		GrLine(window, gc, 0, y1, 639, y1);
		GrFlush();
#endif
	}
	end_timer();
#ifdef TEST_FOR_X
	XClearWindow(display, window);
#else
	GrClearWindow(window, GR_TRUE);
#endif
	
	// Vertical Line
	/////////////////////////////////////////////////
	printf("Vertical Line(XDrawLine)\n");
	start_timer();
	for(c=0; c<count*19; c++)
	{
		x1=random()%640;
#ifdef TEST_FOR_X
		XDrawLine(display, window, gc, x1, 0, x1, 479);
		XFlush(display);
#else
		GrLine(window, gc, x1, 0, x1, 479);
		GrFlush();
#endif
	}
	end_timer();
#ifdef TEST_FOR_X
	XClearWindow(display, window);
#else
	GrClearWindow(window, GR_TRUE);
#endif

	// General Line
	/////////////////////////////////////////////////
	printf("General Line(XDrawLine)\n");
	start_timer();
	for(c=0; c<count*22; c++)
	{
		x1 = random()%640;
		x2 = random()%640;
		y1 = random()%480;
		y2 = random()%480;
#ifdef TEST_FOR_X
		XDrawLine(display, window, gc, x1, y1, x2, y2);
		XFlush(display);
#else
		GrLine(window, gc, x1, y1, x2, y2);
		GrFlush();
#endif
	}
	end_timer();
#ifdef TEST_FOR_X
	XClearWindow(display, window);
#else
	GrClearWindow(window, GR_TRUE);
#endif

	// Point
	//////////////////////////////////////////////////
	printf("XPoint\n");
	start_timer();
	for(c=0; c<count*25; c++)
	{
		x1 = random()%640;
		y1 = random()%480;
#ifdef TEST_FOR_X
		XDrawPoint(display, window, gc, x1, y1);
		XFlush(display);
#else
		GrPoint(window, gc, x1, y1);
		GrFlush();
#endif
	}
	end_timer();
#ifdef TEST_FOR_X
	XClearWindow(display, window);
#else
	GrClearWindow(window, GR_TRUE);
#endif

	// Rectangle
	//////////////////////////////////////////////////
	printf("XRectangle\n");
	start_timer();
	for(c=0; c<count*20; c++)
	{
		x1=random()%639;
		y1=random()%479;
		x2=random()%(639-x1)+1;
		y2=random()%(479-y1)+1;
#ifdef TEST_FOR_X
		XDrawRectangle(display, window, gc, x1, y1, x2, y2);
		XFlush(display);
#else
		GrRect(window, gc, x1, y1, x2, y2);
		GrFlush();
#endif
	}
	end_timer();
#ifdef TEST_FOR_X
	XClearWindow(display, window);
#else
	GrClearWindow(window, GR_TRUE);
#endif

	// FillRectangle
	//////////////////////////////////////////////////
	printf("XFillRectangle\n");
	start_timer();
	for(c=0; c<count*18; c++)
	{
		x1=random()%639;
		y1=random()%479;
		x2=random()%(639-x1)+1;
		y2=random()%(479-y1)+1;	
#ifdef TEST_FOR_X
		XFillRectangle(display, window, gc, x1, y1, x2, y2);
		XFlush(display);
#else
		GrFillRect(window, gc, x1, y1, x2, y2);
		GrFlush();
#endif
	}
	end_timer();
#ifdef TEST_FOR_X

	XClearWindow(display, window);
#else
	GrClearWindow(window, GR_TRUE);
#endif

	// FillPolygon
	//////////////////////////////////////////////////
	printf("XFillPolygon\n");
	start_timer();
	for(c=0; c<count; c++)
	{
		for(c1=0; c1<NUM_POINTS; c1++)
		{
			points[c1].x = random()%640;
			points[c1].y = random()%480;
		}	
#ifdef TEST_FOR_X
		XFillPolygon(display, window, gc, points, NUM_POINTS,
			0, 0);
		XFlush(display);
#else
		GrFillPoly(window, gc, NUM_POINTS, points);
		GrFlush();
#endif
	}
	end_timer();
#ifdef TEST_FOR_X
	XClearWindow(display, window);
#else
	GrClearWindow(window, GR_TRUE);
#endif


	// CopyArea
	/////////////////////////////////////////////////
	printf("XCopyArea\n");
	start_timer();
	for(c=0; c<count*5; c++)
	{
		x1=random()%320;
		y1=random()%240;
		x2=random()%319+1;
		y2=random()%239+1;
		
#ifdef TEST_FOR_X
		XCopyArea(display, src_pixmap, window, gc,
			0, 0, x2, y2, x1, y1);
		XFlush(display);
#else
		GrCopyArea(window, gc, x1, y1, x2 ,y2, src_pixmap,
			0, 0, 0);
		GrFlush();
#endif
	}
	end_timer();	

#ifdef TEST_FOR_X
	XDestroyWindow(display, window);
#else
	GrClose();
#endif
}

