/* Copyright (c) 2000 Simon Wood <simon@mungewell.uklinux.net>
 *
 * This program is licensed under the same terms that Microwindows
 * and Nano-X are licensed under.  See the file LICENSE accompanying
 * this distribution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MWINCLUDECOLORS
#include "nano-X.h"

/* set up size of the grid */
#define WIDTH_IN_TILES	4
#define HEIGHT_IN_TILES	4
#define MAX_TILES	(WIDTH_IN_TILES * HEIGHT_IN_TILES)
#define USE_IMAGE	1

static	int	value[WIDTH_IN_TILES][HEIGHT_IN_TILES];
static	int	calc_width, calc_height;
static	int	tile_width = 40;
static	int	tile_height = 40;

#if USE_IMAGE
static	void *	image_addr;
static	int	using_image = 1;
static	GR_WINDOW_ID	image;		/* storage area for image */
#endif

static	GR_WINDOW_ID	master;		/* id for whole window */
static	GR_WINDOW_ID	buttons;	/* id for buttons */
static	GR_WINDOW_ID	tiles;		/* id for play area */
static	GR_GC_ID	gc1;		/* graphics context for text */

static  int	value[WIDTH_IN_TILES][HEIGHT_IN_TILES];

/* function prototypes */
static	void	HandleEvents();
static	void	RefreshWindow();
static	void	RandomiseTiles();
static	void	MoveTile();
static	void	DrawTile();

int
main(int argc,char **argv)
{
	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}
		
	gc1 = GrNewGC();

#if USE_IMAGE
	image = GrNewWindow(GR_ROOT_WINDOW_ID, 300, 0, (WIDTH_IN_TILES * tile_width),
		(HEIGHT_IN_TILES * tile_height), 4, BLACK, WHITE);

	if(argc != 2)
		/* No image specified, use numered tiles */
		using_image = 0;
	else {
		/* need to find out image size.... */
 		image_addr = malloc(4 * (WIDTH_IN_TILES * tile_width) *
			(HEIGHT_IN_TILES * tile_height) );

 		image = GrNewPixmap((WIDTH_IN_TILES * tile_width),
 			(HEIGHT_IN_TILES * tile_height), image_addr);
 
		GrDrawImageFromFile(image, gc1, 0, 0,
			GR_IMAGE_MAX_SIZE, GR_IMAGE_MAX_SIZE, argv[1], 0);
	}
#endif
	
	/* calculate size of tile area */
 	calc_width = 10 + (WIDTH_IN_TILES * tile_width);
 	calc_height = 15 + 35 + (HEIGHT_IN_TILES * tile_height);
#if 0
	/* enforce minimum size */
	if (calc_width < 240) calc_width=240;
	if (calc_height < 320) calc_height=320;
#endif
	master = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, calc_width, calc_height, 1, RED, WHITE);
 	buttons = GrNewWindow((GR_WINDOW_ID) master, 5, 5, (calc_width - 5), 35, 1, RED, RED);

	tiles = GrNewWindow((GR_WINDOW_ID) master, (calc_width/2) - (WIDTH_IN_TILES * tile_width /2),
 	 	45 + ((calc_height - 50)/2) - (HEIGHT_IN_TILES * tile_height /2),
		(WIDTH_IN_TILES * tile_width), (HEIGHT_IN_TILES * tile_height), 1, RED, RED);

	GrMapWindow(master);
	GrMapWindow(buttons);
	GrMapWindow(tiles);

 	/* set random seed */
 	srandom((int) getpid());

	RandomiseTiles();
	
	GrSelectEvents(master, GR_EVENT_MASK_EXPOSURE|GR_EVENT_MASK_CLOSE_REQ);
	GrSelectEvents(buttons, GR_EVENT_MASK_BUTTON_DOWN); 
	GrSelectEvents(tiles, GR_EVENT_MASK_BUTTON_DOWN);

	RefreshWindow();

	while (GR_TRUE) {
		GR_EVENT event;

		GrGetNextEvent(&event);
		HandleEvents(&event);
	}
}


/*
 * Read the next event and handle it.
 */
void
HandleEvents(GR_EVENT *ep)
{
	switch (ep->type) {
		case GR_EVENT_TYPE_BUTTON_DOWN:
			if (ep->button.wid == buttons) {
 				if (ep->button.x < (calc_width/2)) {
					/* 'Again' */
					RandomiseTiles();
					RefreshWindow();
 				} else {
					/* 'Quit' */
					GrClose();
#if USE_IMAGE
					if (using_image)
						free(image_addr);
#endif
					exit(0);
				}
			}

			if (ep->button.wid == tiles) {
				/* Try to move selected tile */
				MoveTile( (int)(ep->button.x / tile_width),
					(int)(ep->button.y / tile_height) );
			}
			break;

		case GR_EVENT_TYPE_EXPOSURE:
			RefreshWindow();
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
	}

}

void
RefreshWindow()
{
	int xpos, ypos;

	GrSetGCForeground(gc1, WHITE);
	GrSetGCBackground(gc1, RED);

	/* draw the buttons */
	GrRect(buttons, gc1, 0, 0, (calc_width - 12)/2, 34);
	GrRect(buttons, gc1, (calc_width - 8)/2, 0, (calc_width - 12)/2, 34);

#if 0	/* for when center align text works */
	GrText(buttons, gc1, (calc_width - 10)/4, 22, "Again", 5, 0);
	GrText(buttons, gc1, (calc_width - 10)*3/4, 22, "Quit", 4, 0);
#else
	GrText(buttons, gc1, 5, 22, "Again", 5, 0);
	GrText(buttons, gc1, (calc_width / 2) + 5, 22, "Quit", 4, 0);
#endif
	
	/* draw the tiles */
	for (ypos=0; ypos< HEIGHT_IN_TILES; ypos++){
		for (xpos=0; xpos< WIDTH_IN_TILES; xpos++){
			DrawTile(xpos, ypos);
		}
	}
}

void
RandomiseTiles()
{
	int count, xpos, ypos;

	/* allocate all the numbers in order  1..MAX_TILES */
	for (ypos=0; ypos< HEIGHT_IN_TILES; ypos++){
		for (xpos=0; xpos< WIDTH_IN_TILES; xpos++){
			value[xpos][ypos] = 1 + xpos + (WIDTH_IN_TILES * ypos);
		}
	}

	/* position of 'hole' */
	xpos = WIDTH_IN_TILES - 1;
	ypos = HEIGHT_IN_TILES - 1;

	/* randomly slide them around, ALL games can therefore solved - so no excusses!! */
	for (count=0; count< MAX_TILES * 1000; count++){
		switch(random() % 4) {
			case 0:
				if (ypos < HEIGHT_IN_TILES - 1) {
					value[xpos][ypos] = value[xpos][ypos+1];
					ypos++;
					value[xpos][ypos] = MAX_TILES;
				}
				break;
			case 1:
				if (xpos > 0) {
					value[xpos][ypos] = value[xpos - 1][ypos];
					xpos--;
					value[xpos][ypos] = MAX_TILES;
				}
				break;
			case 2:
				if (ypos > 0) {
					value[xpos][ypos] = value[xpos][ypos - 1];
					ypos--;
					value[xpos][ypos] = MAX_TILES;
				}
				break;
			case 3:
				if (xpos < WIDTH_IN_TILES - 1) {
					value[xpos][ypos] = value[xpos + 1][ypos];
					xpos++;
					value[xpos][ypos] = MAX_TILES;
				}
				break;
		}
	}
}

void
MoveTile(xpos, ypos)
	int xpos, ypos;
{
	/* check all possible moves to see if there is the blank (N,E,S,W) */
	if (ypos > 0 && value[xpos][ypos - 1] == MAX_TILES) {
		value[xpos][ypos - 1] = value[xpos][ypos];
		value[xpos][ypos] = MAX_TILES;
		DrawTile(xpos, ypos - 1);
		DrawTile(xpos, ypos);
	}

	if (xpos < (WIDTH_IN_TILES - 1) && value[xpos + 1][ypos] == MAX_TILES) {
		value[xpos + 1][ypos] = value[xpos][ypos];
		value[xpos][ypos] = MAX_TILES;
		DrawTile(xpos + 1, ypos);
		DrawTile(xpos, ypos);
	}

	if (ypos < (HEIGHT_IN_TILES - 1) && value[xpos][ypos + 1] == MAX_TILES) {
		value[xpos][ypos + 1] = value[xpos][ypos];
		value[xpos][ypos] = MAX_TILES;
		DrawTile(xpos, ypos + 1);
		DrawTile(xpos, ypos);
	}

	if (xpos > 0 && value[xpos - 1][ypos] == MAX_TILES) {
		value[xpos - 1][ypos] = value[xpos][ypos];
		value[xpos][ypos] = MAX_TILES;
		DrawTile(xpos - 1, ypos);
		DrawTile(xpos, ypos);
	}

	/* check for a winner */
	if (value[WIDTH_IN_TILES - 1][HEIGHT_IN_TILES - 1] == MAX_TILES) {
		int winner = 0;
		for (ypos=0; ypos< HEIGHT_IN_TILES; ypos++){
			for (xpos=0; xpos< WIDTH_IN_TILES; xpos++){
				if (value[xpos][ypos] == winner + 1)
					winner++;
				else 
					winner=0;
			}
		}
		if (winner == MAX_TILES) {
			/* Do winning screen */
			int loop = MAX_TILES;
			for(loop=0; loop < MAX_TILES; loop++) {
				for(winner=0; winner < (MAX_TILES - loop) ; winner++) {

					/* move tiles around */
					xpos = winner % WIDTH_IN_TILES;
					ypos = (int)(winner/WIDTH_IN_TILES);
					value[xpos][ypos] = loop + winner + 1; 
					DrawTile(winner % WIDTH_IN_TILES, (int)(winner/WIDTH_IN_TILES));
				}
				GrFlush();
				for(winner=0; winner < 10000000 ; winner++);
					/* delay loop */
			}
			/* Print message */
			GrSetGCForeground(gc1, WHITE);
			GrSetGCBackground(gc1, RED);
			GrText(tiles, gc1, ((WIDTH_IN_TILES * tile_width)/2) - 40, (HEIGHT_IN_TILES * tile_height)/2, "Well Done!!", -1, 0);
		}
				
	}
}


void
DrawTile(xpos, ypos)
	int xpos, ypos;
{
	char text[]="00";

	/* blank out old tile */
	GrSetGCForeground(gc1, RED);
	GrFillRect(tiles, gc1, (xpos* tile_width), (ypos*tile_height), tile_width, tile_height);

	if (value[xpos][ypos] != MAX_TILES ) {
		/* re-draw tile and number */
		GrSetGCForeground(gc1, WHITE);
		GrSetGCBackground(gc1, RED);
		GrRect(tiles, gc1, (xpos*tile_width), (ypos*tile_height), tile_width, tile_height);
		
#if USE_IMAGE
		if (using_image) {
			/* copy from image window */
			GrCopyArea(tiles, gc1, 1 + (xpos*tile_width), 1 + (ypos*tile_height), 
				tile_width - 2, tile_height - 2, image,
				1 + (((value[xpos][ypos] - 1) % WIDTH_IN_TILES) * tile_width), 
				1 + (((int)(value[xpos][ypos] - 1) / WIDTH_IN_TILES) * tile_height), 0);
		} else {
#endif
			/* label the tile with a number */
			if (value[xpos][ypos] > 9)
				text[0] = 48 + (int)(value[xpos][ypos]/10);
			else
				text[0] = 32;
	
			text[1] = 48 + value[xpos][ypos] % 10;
			
			GrText(tiles, gc1, (xpos*tile_width) + (tile_width /2) - 5, (ypos*tile_height) + (tile_height/2) + 5, &text, -1, 0);
#if USE_IMAGE
		}
#endif
	}
}
