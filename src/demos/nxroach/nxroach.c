/*
    Xroach - A game of skill.  Try to find the roaches under your windows.
    Ported to Nano-X by Greg Haerr
    
    Copyright 1991 by J.T. Anderson
    jta@locus.com
    
    This program may be freely distributed provided that all
    copyright notices are retained.

    Dedicated to Greg McFarlane.   (gregm@otc.otca.oz.au)
*/
char Copyright[] = "nxroach\nCopyright 1991 J.T. Anderson";

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

#define None	0
#define Pixmap	GR_WINDOW_ID
#include "roachmap.h"

#define SCAMPER_EVENT	99

typedef struct Roach {
    RoachMap *rp;
    int index;
    float x;
    float y;
    int intX;
    int intY;
    int hidden;
    int turnLeft;
    int steps;
} Roach;

GR_COORD display_width, display_height;
GR_GC_ID gc;
GR_COLOR roachColor = BLACK;
GR_REGION_ID rootVisible = 0;
GR_BOOL done = GR_FALSE;
GR_BOOL eventBlock = GR_FALSE;

Roach *roaches;
int maxRoaches = 10;
int curRoaches = 0;
float roachSpeed = 20.0;

void Usage(void);
void SigHandler(int sig);
void AddRoach(void);
void MoveRoach(int Rx);
void DrawRoaches(void);
void CoverRoot(void);
int CalcRootVisible(void);
int MarkHiddenRoaches(void);

int
main(int ac, char **av)
{
    int ax;
    char *arg;
    RoachMap *rp;
    int rx;
    float angle;
    GR_EVENT ev;
    int nVis;
    int needCalc;
    GR_SCREEN_INFO sinfo;
    
    /*
       Process command line options.
    */
    for (ax=1; ax<ac; ax++) {
	arg = av[ax];
	if (strcmp(arg, "-rc") == 0) {
	    roachColor = atoi(av[++ax]);
	}
	else if (strcmp(arg, "-speed") == 0) {
	    roachSpeed = atof(av[++ax]);
	}
	else if (strcmp(arg, "-roaches") == 0) {
	    maxRoaches = strtol(av[++ax], (char **)NULL, 0);
	}
	else {
	    Usage();
	}
    }

    srand((int)time((long *)NULL));
    
    /*
       Catch some signals so we can erase any visible roaches.
    */
    signal(SIGKILL, SigHandler);
    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);
    signal(SIGHUP, SigHandler);

    if (GrOpen() < 0) {
	fprintf(stderr, "can't open graphics\n");
	exit(1);
    }

    GrGetScreenInfo(&sinfo);
    display_width = sinfo.cols;
    display_height = sinfo.rows;
    
    /*
       Create roach pixmaps at several orientations.
    */
    for (ax=0; ax<360; ax+=ROACH_ANGLE) {
	rx = ax / ROACH_ANGLE;
	angle = rx * 0.261799387799;
	rp = &roachPix[rx];
	rp->pixmap = GrNewPixmapFromData(rp->width, rp->height, WHITE, BLACK,
		rp->roachBits, GR_BMDATA_BYTEREVERSE|GR_BMDATA_BYTESWAP);
	rp->sine = sin(angle);
	rp->cosine = cos(angle);
    }

    roaches = (Roach *)malloc(sizeof(Roach) * maxRoaches);

    gc = GrNewGC();
    
    while (curRoaches < maxRoaches)
	AddRoach();
    
    GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CHLD_UPDATE);
    
    needCalc = 1;
    while (!done) {
	if (GrPeekEvent(&ev))
	    GrGetNextEvent(&ev);
	else {
	    if (needCalc) {
		needCalc = CalcRootVisible();
	    }
	    nVis = MarkHiddenRoaches();
	    if (nVis) {
		ev.type = SCAMPER_EVENT;
	    }
	    else {
		DrawRoaches();
		eventBlock = GR_TRUE;
		GrGetNextEvent(&ev);
		eventBlock = GR_FALSE;
	    }
	}
	
	switch (ev.type) {
	    case SCAMPER_EVENT:
		for (rx=0; rx<curRoaches; rx++) {
		    if (!roaches[rx].hidden)
			MoveRoach(rx);
		}
		DrawRoaches();
		GrDelay(100);
		break;
		
	    case GR_EVENT_TYPE_EXPOSURE:
	    case GR_EVENT_TYPE_CHLD_UPDATE:
		needCalc = 1;
		break;
		
	}
    }
    
    CoverRoot();
    GrClose();
    return 0;
}

void
Usage(void)
{
    fprintf(stderr, "Usage: nxroach [options]\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "       -rc      roachcolor\n");
    fprintf(stderr, "       -roaches numroaches\n");
    fprintf(stderr, "       -speed   roachspeed\n");
    
    exit(1);
}

void
SigHandler(int sig)
{
    /*
       If we are blocked, no roaches are visible and we can just bail
       out.  If we are not blocked, then let the main procedure clean
       up the root window.
    */
    if (eventBlock) {
	GrClose();
	exit(0);
    }
    else {
	done = GR_TRUE;
    }
}

/*
   Generate random integer between 0 and maxVal-1.
*/
int
RandInt(int maxVal)
{
	return rand() % maxVal;
}

/*
   Check for roach completely in specified rectangle.
*/
int
RoachInRect(Roach *roach, int rx, int ry, int x, int y,
	unsigned int width, unsigned int height)
{
    if (rx < x) return 0;
    if ((rx + roach->rp->width) > (x + width)) return 0;
    if (ry < y) return 0;
    if ((ry + roach->rp->height) > (y + height)) return 0;
    
    return 1;
}

/*
   Check for roach overlapping specified rectangle.
*/
int
RoachOverRect(Roach *roach, int rx, int ry, int x, int y,
	unsigned int width, unsigned int height)
{
    if (rx >= (x + width)) return 0;
    if ((rx + roach->rp->width) <= x) return 0;
    if (ry >= (y + height)) return 0;
    if ((ry + roach->rp->height) <= y) return 0;
    
    return 1;
}

/*
   Give birth to a roach.
*/
void
AddRoach(void)
{
    Roach *r;
    
    if (curRoaches < maxRoaches) {
	r = &roaches[curRoaches++];
	r->index = RandInt(ROACH_HEADINGS);
	r->rp = &roachPix[r->index];
	r->x = RandInt(display_width - r->rp->width);
	r->y = RandInt(display_height - r->rp->height);
	r->intX = -1;
	r->intY = -1;
	r->hidden = 0;
	r->steps = RandInt(200);
	r->turnLeft = RandInt(100) >= 50;
    }
}

/*
   Turn a roach.
*/
void
TurnRoach(Roach *roach)
{
    if (roach->index != (roach->rp - roachPix)) return;

    if (roach->turnLeft) {
	roach->index += (RandInt(30) / 10) + 1;
	if (roach->index >= ROACH_HEADINGS)
	    roach->index -= ROACH_HEADINGS;
    }
    else {
	roach->index -= (RandInt(30) / 10) + 1;
	if (roach->index < 0)
	    roach->index += ROACH_HEADINGS;
    }
}

/*
   Move a roach.
*/
void
MoveRoach(int rx)
{
    Roach *roach;
    Roach *r2;
    float newX;
    float newY;
    int ii;
    
    roach = &roaches[rx];
    newX = roach->x + (roachSpeed * roach->rp->cosine);
    newY = roach->y - (roachSpeed * roach->rp->sine);
    
    if (RoachInRect(roach, (int)newX, (int)newY, 
			    0, 0, display_width, display_height)) {
	
	roach->x = newX;
	roach->y = newY;

	if (roach->steps-- <= 0) {
	    TurnRoach(roach);
	    roach->steps = RandInt(200);
	}

	for (ii=rx+1; ii<curRoaches; ii++) {
	    r2 = &roaches[ii];
	    if (RoachOverRect(roach, (int)newX, (int)newY,
		r2->intX, r2->intY, r2->rp->width, r2->rp->height)) {
	
		TurnRoach(roach);
	    }
	}
    }
    else {
	TurnRoach(roach);
    }
}
    
/*
   Draw all roaches.
*/
void
DrawRoaches(void)
{
    Roach *roach;
    int rx;
    
    for (rx=0; rx<curRoaches; rx++) {
	roach = &roaches[rx];
	
	if (roach->intX >= 0) {
	    GrClearArea(GR_ROOT_WINDOW_ID, roach->intX, roach->intY,
		roach->rp->width, roach->rp->height, GR_FALSE);
	}
    }
    
    for (rx=0; rx<curRoaches; rx++) {
	roach = &roaches[rx];
	
	if (!roach->hidden) {
	    int size = roach->rp->width * roach->rp->height;
	    GR_PIXELVAL roachbuf[size];
	    GR_PIXELVAL screenbuf[size];
	    int i;

	    roach->intX = roach->x;
	    roach->intY = roach->y;
	    roach->rp = &roachPix[roach->index];

	    /*
    	    //XSetForeground(display, gc, AllocNamedColor(roachColor, black));
    	    //XSetFillStyle(display, gc, FillStippled);
	    //XSetStipple(display, gc, roach->rp->pixmap);
	    //XSetTSOrigin(display, gc, roach->intX, roach->intY);
	    //XFillRectangle(display, rootWin, gc,
		//roach->intX, roach->intY, roach->rp->width, roach->rp->height);
	    */

	    /* read roach bitmap*/
	    GrReadArea(roach->rp->pixmap, 0, 0,
		roach->rp->width, roach->rp->height, roachbuf);

	    /* read root window*/
	    GrReadArea(GR_ROOT_WINDOW_ID, roach->intX, roach->intY,
		roach->rp->width, roach->rp->height, screenbuf);

	    /* convert fg roach bitmap bits to roach color on root window bits*/
	    for (i=0; i<size; ++i)
		    if (roachbuf[i] != BLACK)
			    screenbuf[i] = roachColor;

	    /* write root window*/
	    GrArea(GR_ROOT_WINDOW_ID, gc, roach->intX, roach->intY,
		roach->rp->width, roach->rp->height, screenbuf, MWPF_PIXELVAL);
	}
	else {
	    roach->intX = -1;
	}
    }
    GrFlush();
}

/*
   Cover root window to erase roaches.
*/
void
CoverRoot(void)
{
    GR_WINDOW_ID roachWin;
    
    roachWin = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, display_width, display_height,
		    0, CYAN, BLACK);
    GrLowerWindow(roachWin);
    GrMapWindow(roachWin);
    GrFlush();
}    

/*
   Calculate Visible region of root window.
*/
int
CalcRootVisible(void)
{
    GR_REGION_ID covered;
    GR_REGION_ID visible;
    GR_WINDOW_ID parent;
    GR_WINDOW_ID *children;
    GR_COUNT nChildren;
    GR_COUNT wx;
    GR_RECT rect;
    GR_WINDOW_INFO info;
    
    /*
       Get children of root.
    */
    GrQueryTree(GR_ROOT_WINDOW_ID, &parent, &children, &nChildren);
    
    /*
       For each mapped child, add the window rectangle to the covered
       region.
    */
    covered = GrNewRegion();
    for (wx=0; wx<nChildren; wx++) {
	GrGetWindowInfo(children[wx], &info);
	if (info.unmapcount == 0) {
	    rect.x = info.x;
	    rect.y = info.y;
	    rect.width = info.width;
	    rect.height = info.height;
	    GrUnionRectWithRegion(covered, &rect);
	}
    }
    free(children);

    /*
       Subtract the covered region from the root window region.
    */
    visible = GrNewRegion();
    rect.x = 0;
    rect.y = 0;
    rect.width = display_width;
    rect.height = display_height;
    GrUnionRectWithRegion(visible, &rect);
    GrSubtractRegion(visible, visible, covered);
    GrDestroyRegion(covered);
    
    /*
       Save visible region globally.
    */
    if (rootVisible)
	GrDestroyRegion(rootVisible);
    rootVisible = visible;

    /*
       Mark all roaches visible.
    */
    for (wx=0; wx<curRoaches; wx++) 
	roaches[wx].hidden = 0;

    return 0;
}

/*
   Mark hidden roaches.
*/
int
MarkHiddenRoaches(void)
{
    int rx;
    Roach *r;
    int nVisible;
    
    nVisible = 0;
    for (rx=0; rx<curRoaches; rx++) {
	r = &roaches[rx];
	
	if (!r->hidden) {
	    if (r->intX > 0 && GrRectInRegion(rootVisible, r->intX, r->intY,
			    r->rp->width, r->rp->height) == MWRECT_OUT) {
		r->hidden = 1;
	    }
	    else {
		nVisible++;
	    }
	}
    }
    
    return nVisible;
}
