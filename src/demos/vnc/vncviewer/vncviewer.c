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
 * vncviewer.c - VNC viewer for nano-X.
 */

#include <vncviewer.h>

static void HandleEvents(GR_EVENT *ev);

int
main(int argc, char **argv)
{
#ifndef NANOX
    fd_set fds;
    struct timeval tv, *tvp;
    int msWait;
#endif
    processArgs(argc, argv);

    if (listenSpecified) {

#ifndef NANOX
	listenForIncomingConnections();
	/* returns only with a succesful connection */
#endif

    } else {
	if (!ConnectToRFBServer(hostname, port)) exit(1);
    }

    if (!InitialiseRFBConnection(rfbsock)) exit(1);

    if (!CreateXWindow()) exit(1);

    if (!SetFormatAndEncodings()) {
	ShutdownX();
	exit(1);
    }

    if (!SendFramebufferUpdateRequest(updateRequestX, updateRequestY,
				      updateRequestW, updateRequestH, False)) {
	ShutdownX();
	exit(1);
    }

    printf("nanox fd = %d, rfbsock = %d\n", ConnectionNumber(dpy), rfbsock);
#ifdef NANOX
    /* register the RFB socket */
    GrRegisterInput(rfbsock);
    /* call the nanox main loop to wait for all events */
    while (True) {
	GrMainLoop(HandleEvents);
    }

#else
    while (True) {
	/*
	 * Always handle all X events before doing select.  This is the
	 * simplest way of ensuring that we don't block in select while
	 * Xlib has some events on its queue.
	 */

	if (!HandleXEvents()) {
	    ShutdownX();
	    exit(1);
	}

	tvp = NULL;

	if (sendUpdateRequest) {
	    gettimeofday(&tv, NULL);

	    msWait = (updateRequestPeriodms +
		      ((updateRequestTime.tv_sec - tv.tv_sec) * 1000) +
		      ((updateRequestTime.tv_usec - tv.tv_usec) / 1000));

	    if (msWait > 0) {
		tv.tv_sec = msWait / 1000;
		tv.tv_usec = (msWait % 1000) * 1000;

		tvp = &tv;
	    } else {
		if (!SendIncrementalFramebufferUpdateRequest()) {
		    ShutdownX();
		    exit(1);
		}
	    }
	}

	FD_ZERO(&fds);
	FD_SET(ConnectionNumber(dpy),&fds);
	FD_SET(rfbsock,&fds);

	if (select(FD_SETSIZE, &fds, NULL, NULL, tvp) < 0) {
	    perror("select");
	    ShutdownX();
	    exit(1);
	}

	if (FD_ISSET(rfbsock, &fds)) {
	    if (!HandleRFBServerMessage()) {
		ShutdownX();
		exit(1);
	    }
	}
    }
#endif	/* NANOX */

    return 0;
}

#ifdef NANOX
/*
 * call-back routine to handle all events
 */
void
HandleEvents(GR_EVENT *ev)
{
	struct timeval tv, *tvp;
	int msWait;

/*	printf("%2d ", ev->type);
	fflush(stdout);
 */
	if (ev->type == GR_EVENT_TYPE_FDINPUT) {
		if (!HandleRFBServerMessage()) {
			ShutdownX();
			exit(1);
		}
	} else {
		if (!HandleXEvents(ev)) {
		    ShutdownX();
		    exit(1);
		}
	}
	if (sendUpdateRequest) {
	    gettimeofday(&tv, NULL);

	    msWait = (updateRequestPeriodms +
		      ((updateRequestTime.tv_sec - tv.tv_sec) * 1000) +
		      ((updateRequestTime.tv_usec - tv.tv_usec) / 1000));

	    if (msWait > 0) {
		tv.tv_sec = msWait / 1000;
		tv.tv_usec = (msWait % 1000) * 1000;

		tvp = &tv;
	    } else {
		if (!SendIncrementalFramebufferUpdateRequest()) {
		    ShutdownX();
		    exit(1);
		}
	    }
	}
}
#endif	/* NANOX */

