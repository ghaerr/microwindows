/*
 * A simple utility to set the portrait mode of the display.
 * Copyright (c) Alex Holden <alex@alexholden.net> 2002.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>

static void usage(void)
{
	fprintf(stderr, "Usage: setportrait <none|left|right|down>\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	int portrait = -1;

	if(argc != 2) usage();

	if(!strcmp("none", argv[1])) portrait = MWPORTRAIT_NONE;
	else if(!strcmp("left", argv[1])) portrait = MWPORTRAIT_LEFT;
	else if(!strcmp("right", argv[1])) portrait = MWPORTRAIT_RIGHT;
	else if(!strcmp("down", argv[1])) portrait = MWPORTRAIT_DOWN;
	else usage();

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return -1;
	}

	GrSetPortraitMode(portrait);

	GrClose();

	return 0;
}
