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
 * args.c - argument processing.
 */

#include <sys/utsname.h>
#include <vncviewer.h>

#define FLASHPORT  (5400)    /* Offset to listen for `flash' commands */
#define CLIENTPORT (5500)    /* Offset to listen for reverse connections */
#define SERVERPORT (5900)    /* Offset to server for regular connections */

char *programName;

char hostname[256];
int port;

Bool listenSpecified = False;
int listenPort = 0, flashPort = 0;

char *displayname = NULL;

Bool shareDesktop = False;
Bool viewOnly = False;

CARD32 explicitEncodings[MAX_ENCODINGS];
int nExplicitEncodings = 0;
Bool addCopyRect = True;
Bool addRRE = True;
Bool addCoRRE = True;
Bool addHextile = True;

Bool useBGR233 = False;
Bool forceOwnCmap = False;
Bool forceTruecolour = False;
int requestedDepth = 0;

char *geometry = NULL;

int wmDecorationWidth = 4;
int wmDecorationHeight = 24;

char *passwdFile = NULL;

int updateRequestPeriodms = 0;

int updateRequestX = 0;
int updateRequestY = 0;
int updateRequestW = 0;
int updateRequestH = 0;

int rawDelay = 0;
int copyRectDelay = 0;

Bool debug = False;


void
usage()
{
    fprintf(stderr,"\n"
	    "usage: %s [<options>] <host>:<display#>\n"
	    "       %s [<options>] -listen [<display#>]\n"
	    "\n"
	    "<options> are:\n"
	    "              [-display <display>] [-shared] [-viewonly]\n"
	    "              [-raw] [-copyrect] [-rre] [-corre] [-hextile]\n"
	    "              [-nocopyrect] [-norre] [-nocorre] [-nohextile]\n"
	    "              [-bgr233] [-owncmap] [-truecolour] [-depth <d>]\n"
	    "              [-geometry <geom>]\n"
	    "              [-wmdecoration <width>x<height>]\n"
	    "              [-passwd <passwd-file>]\n"
	    "              [-period <ms>]\n"
	    "              [-region <x> <y> <width> <height>]\n"
	    "              [-rawdelay <ms>] [-copyrectdelay <ms>] [-debug]\n\n"
	    ,programName,programName);
    exit(1);
}


void
processArgs(int argc, char **argv)
{
    int i;
    Bool argumentSpecified = False;

    programName = argv[0];

    for (i = 1; i < argc; i++) {

	if (strcmp(argv[i],"-display") == 0) {

	    if (++i >= argc) usage();
	    displayname = argv[i];

	} else if (strcmp(argv[i],"-shared") == 0) {

	    shareDesktop = True;

	} else if (strcmp(argv[i],"-viewonly") == 0) {

	    viewOnly = True;

	} else if (strcmp(argv[i],"-rre") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingRRE;
	    addRRE = False;

	} else if (strcmp(argv[i],"-corre") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingCoRRE;
	    addCoRRE = False;

	} else if (strcmp(argv[i],"-hextile") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingHextile;
	    addHextile = False;

	} else if (strcmp(argv[i],"-copyrect") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingCopyRect;
	    addCopyRect = False;

	} else if (strcmp(argv[i],"-raw") == 0) {

	    explicitEncodings[nExplicitEncodings++] = rfbEncodingRaw;

	} else if (strcmp(argv[i],"-norre") == 0) {

	    addRRE = False;

	} else if (strcmp(argv[i],"-nocorre") == 0) {

	    addCoRRE = False;

	} else if (strcmp(argv[i],"-nohextile") == 0) {

	    addHextile = False;

	} else if (strcmp(argv[i],"-nocopyrect") == 0) {

	    addCopyRect = False;

	} else if (strcmp(argv[i],"-bgr233") == 0) {

	    useBGR233 = True;

	} else if (strcmp(argv[i],"-owncmap") == 0) {

	    forceOwnCmap = True;

	} else if (strcmp(argv[i],"-truecolour") == 0) {

	    forceTruecolour = True;

	} else if (strcmp(argv[i],"-depth") == 0) {

	    if (++i >= argc) usage();
	    requestedDepth = atoi(argv[i]);

	} else if (strcmp(argv[i],"-geometry") == 0) {

	    if (++i >= argc) usage();
	    geometry = argv[i];

	} else if (strcmp(argv[i],"-wmdecoration") == 0) {

	    if (++i >= argc) usage();
	    if (sscanf(argv[i], "%dx%d",
		       &wmDecorationWidth, &wmDecorationHeight) != 2) usage();

	} else if (strcmp(argv[i],"-passwd") == 0) {

	    if (++i >= argc) usage();
	    passwdFile = argv[i];

	} else if (strcmp(argv[i],"-period") == 0) {

	    if (++i >= argc) usage();
	    updateRequestPeriodms = atoi(argv[i]);

	} else if (strcmp(argv[i],"-region") == 0) {

	    if ((i+4) >= argc) usage();
	    updateRequestX = atoi(argv[i+1]);
	    updateRequestY = atoi(argv[i+2]);
	    updateRequestW = atoi(argv[i+3]);
	    updateRequestH = atoi(argv[i+4]);
	    if ((updateRequestX < 0) || (updateRequestY < 0) ||
		(updateRequestW < 0) || (updateRequestH < 0))
		usage();
	    i += 4;

	} else if (strcmp(argv[i],"-rawdelay") == 0) {

	    if (++i >= argc) usage();
	    rawDelay = atoi(argv[i]);

	} else if (strcmp(argv[i],"-copyrectdelay") == 0) {

	    if (++i >= argc) usage();
	    copyRectDelay = atoi(argv[i]);

	} else if (strcmp(argv[i],"-debug") == 0) {

	    debug = True;

	} else if (strcmp(argv[i],"-listen") == 0) {

	    if (argumentSpecified) usage();

	    listenSpecified = True;
	    if (++i < argc) {
		listenPort = CLIENTPORT+atoi(argv[i]);
		flashPort = FLASHPORT+atoi(argv[i]);
	    }

	} else if (argv[i][0] != '-') {

	    if (argumentSpecified || listenSpecified) usage();

	    argumentSpecified = True;

	    if (sscanf(argv[i], "%[^:]:%d", hostname, &port) != 2) usage();

	    if (port < 100)
		port += SERVERPORT;

	} else {

	    usage();

	}
    }

    if (listenSpecified) {
	if (listenPort == 0) {
	    char *display;
	    char *colonPos;
	    struct utsname hostinfo;

	    display = XDisplayName(displayname);
	    colonPos = strchr(display, ':');

	    uname(&hostinfo);

	    if (colonPos && ((colonPos == display) ||
			     (strncmp(hostinfo.nodename, display,
				      strlen(hostinfo.nodename)) == 0))) {

		listenPort = CLIENTPORT+atoi(colonPos+1);
		flashPort = FLASHPORT+atoi(colonPos+1);

	    } else {
		fprintf(stderr,"%s: cannot work out which display number to "
			"listen on.\n", programName);
		fprintf(stderr,
			"Please specify explicitly with -listen <num>\n");
		exit(1);
	    }
	}

    } else if (!argumentSpecified) {

	usage();

    }
}
