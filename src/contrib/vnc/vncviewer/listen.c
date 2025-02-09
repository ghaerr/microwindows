/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * listen.c - listen for incoming connections
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <vncviewer.h>

#define FLASHWIDTH 50	/* pixels */
#define FLASHDELAY 1	/* seconds */

static Font flashFont;

static void getFlashFont(Display *d);
static void flashDisplay(Display *d, char *user);

void
listenForIncomingConnections()
{
    Display *d;
    XEvent ev;
    int listenSocket, flashSocket, sock;
    fd_set fds;
    char flashUser[256];
    int n;

    if (!(d = XOpenDisplay(displayname))) {
	fprintf(stderr,"%s: unable to open display %s\n",
		programName, XDisplayName(displayname));
	exit(1);
    }

    getFlashFont(d);

    listenSocket = ListenAtTcpPort(listenPort);
    flashSocket = ListenAtTcpPort(flashPort);

    if ((listenSocket < 0) || (flashSocket < 0)) exit(1);

    fprintf(stderr,"%s: Listening on port %d (flash port %d)\n",
	    programName,listenPort,flashPort);

    while (True) {

	/* reap any zombies */
	int status, pid;
	while ((pid= wait3(&status, WNOHANG, (struct rusage *)0))>0);

	/* discard any X events */
	while (XCheckIfEvent(d, &ev, AllXEventsPredicate, NULL))
	    ;

	FD_ZERO(&fds); 

	FD_SET(flashSocket, &fds);
	FD_SET(listenSocket, &fds);
	FD_SET(ConnectionNumber(d), &fds);

	select(FD_SETSIZE, &fds, NULL, NULL, NULL);

	if (FD_ISSET(flashSocket, &fds)) {

	    sock = AcceptTcpConnection(flashSocket);
	    if (sock < 0) exit(1);
	    n = read(sock, flashUser, 255);
	    if (n > 0) {
		flashUser[n] = 0;
		flashDisplay(d, flashUser);
	    } else {
		flashDisplay(d, NULL);
	    }
	    close(sock);
	}

	if (FD_ISSET(listenSocket, &fds)) {
	    rfbsock = AcceptTcpConnection(listenSocket);
	    if (rfbsock < 0) exit(1);

	    XCloseDisplay(d);

	    /* Now fork off a new process to deal with it... */

	    switch (fork()) {

	    case -1: 
		perror("fork"); 
		exit(1);

	    case 0:
		/* child - return to caller */
		close(listenSocket);
		close(flashSocket);
		return;

	    default:
		/* parent - go round and listen again */
		close(rfbsock); 
		if (!(d = XOpenDisplay(displayname))) {
		    fprintf(stderr,"%s: unable to open display \"%s\"\r\n",
			    programName, XDisplayName (displayname));
		    exit(1);
		}
		getFlashFont(d);
		break;
	    }
	}
    }
}


/*
 * getFlashFont
 */

static void
getFlashFont(Display *d)
{
    char fontName[256];
    char **fontNames;
    int nFontNames;

    sprintf(fontName,"-*-courier-bold-r-*-*-%d-*-*-*-*-*-iso8859-1",
	    FLASHWIDTH);
    fontNames = XListFonts(d, fontName, 1, &nFontNames);
    if (nFontNames == 1) {
	XFreeFontNames(fontNames);
    } else {
	sprintf(fontName,"fixed");
    }
    flashFont = XLoadFont(d, fontName);
}


/*
 * flashDisplay
 */

static void
flashDisplay(Display *d, char *user)
{
    Window w1, w2, w3, w4;
    XSetWindowAttributes attr;

    XBell(d, 100);

    XForceScreenSaver(d, ScreenSaverReset);

    attr.background_pixel = BlackPixel(d, DefaultScreen(d));
    attr.override_redirect = 1;
    attr.save_under = True;

    w1 = XCreateWindow(d, DefaultRootWindow(d), 0, 0,
		       WidthOfScreen(DefaultScreenOfDisplay(d)), 
		       FLASHWIDTH, 0, 
		       CopyFromParent, CopyFromParent, CopyFromParent, 
		       CWBackPixel|CWOverrideRedirect|CWSaveUnder,
		       &attr);
  
    w2 = XCreateWindow(d, DefaultRootWindow(d), 0, 0, FLASHWIDTH,
		       HeightOfScreen(DefaultScreenOfDisplay(d)), 0,
		       CopyFromParent, CopyFromParent, CopyFromParent, 
		       CWBackPixel|CWOverrideRedirect|CWSaveUnder,
		       &attr);

    w3 = XCreateWindow(d, DefaultRootWindow(d), 
		       WidthOfScreen(DefaultScreenOfDisplay(d))-FLASHWIDTH, 
		       0, FLASHWIDTH, 
		       HeightOfScreen(DefaultScreenOfDisplay(d)), 0, 
		       CopyFromParent, CopyFromParent, CopyFromParent, 
		       CWBackPixel|CWOverrideRedirect|CWSaveUnder,
		       &attr);

    w4 = XCreateWindow(d, DefaultRootWindow(d), 0,
		       HeightOfScreen(DefaultScreenOfDisplay(d))-FLASHWIDTH, 
		       WidthOfScreen(DefaultScreenOfDisplay(d)), 
		       FLASHWIDTH, 0, 
		       CopyFromParent, CopyFromParent, CopyFromParent, 
		       CWBackPixel|CWOverrideRedirect|CWSaveUnder,
		       &attr);

    XMapWindow(d, w1);
    XMapWindow(d, w2);
    XMapWindow(d, w3);
    XMapWindow(d, w4);

    if (user) {
	GC gc;
	XGCValues gcv;

	gcv.foreground = WhitePixel(d, DefaultScreen(d));
	gcv.font = flashFont;
	gc = XCreateGC(d, w1, GCForeground|GCFont, &gcv);
	XDrawString(d, w1, gc,
		    WidthOfScreen(DefaultScreenOfDisplay(d)) / 2 - FLASHWIDTH,
		    (FLASHWIDTH * 3 / 4), user, strlen(user));
    }
    XFlush(d);

    sleep(FLASHDELAY);

    XDestroyWindow(d, w1);
    XDestroyWindow(d, w2);
    XDestroyWindow(d, w3);
    XDestroyWindow(d, w4);
    XFlush(d);
}
