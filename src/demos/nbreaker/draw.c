/*
 * NanoBreaker, a Nano-X Breakout clone by Alex Holden.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is NanoBreaker.
 * 
 * The Initial Developer of the Original Code is Alex Holden.
 * Portions created by Alex Holden are Copyright (C) 2002
 * Alex Holden <alex@alexholden.net>. All Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public license (the  "[GNU] License"), in which case the
 * provisions of [GNU] License are applicable instead of those
 * above.  If you wish to allow use of your version of this file only
 * under the terms of the [GNU] License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting  the provisions above and replace  them with the notice and
 * other provisions required by the [GNU] License.  If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the [GNU] License.
 */

/* draw.c contains functions for drawing the various game objects. */

#include <stdio.h>
#include <stdlib.h>

#include <nano-X.h>
#include <nxcolors.h>
#include "nbreaker.h"

/* Tiles the background image onto the canvas over the rectangle specified by
 * x, y, w, and h. This could probably be done more efficiently, but it works
 * and doesn't seem to use a lot of CPU time. */
static void draw_tiled_background(nbstate *state, int x, int y, int w, int h)
{
	int tilex = 0, tiley = 0, fromx, fromy, cx, cy;
	int destwidth, destheight, srcwidth, srcheight, cwidth, cheight;
	int dwidth = state->canvaswidth;
	int dheight = state->canvasheight;
	int swidth = state->background->w;
	int sheight = state->background->h;

	/* Clip the width of the background image to the size of the canvas
	 * if necessary. */
	if(swidth > dwidth) srcwidth = dwidth;
	else srcwidth = swidth;
	if(sheight > dheight) srcheight = dheight;
	else srcheight = sheight;

	/* For each row of tiles: */
	for(;tiley < dheight; tiley += srcheight, tilex = 0) {
		/* If we've gone past the rectangle, return. */
		if(tiley > (y + h)) return;
		/* If we haven't reached the rectangle, go to the next row. */
		if(y > (tiley + srcheight)) continue;
		/* Clip the output to the bottom of the output window. */
		if((tiley + srcheight) > dheight) destheight = dheight - tiley;
		else destheight = srcheight;
		/* For each tile in the row: */
		for(;tilex < dwidth; tilex += srcwidth) {
			/* If we've gone past the rectangle, skip to the next
			 * row. */
			if(tilex > (x + w)) break;
			/* If we haven't reached the rectangle, skip to the
			 * next tile in the row. */
			if(x > (tilex + srcwidth)) continue;
			/* Clip the output size to the right side of the
			 * output window. */
			if((tilex + srcwidth) > dwidth)
				destwidth = dwidth - tilex;
			else destwidth = srcwidth;

			/* If the tile X dimension is completely within the
			 * output rectangle, set the clipping to the full
			 * size of the background image. */
			if((tilex >= x) && ((tilex + destwidth) <= (x + w))) {
				fromx = 0;
				cx = tilex;
				cwidth = destwidth;
			} else { /* Not completely in the output rectangle. */
				/* If necessary, clip off the left part of
				 * the rectangle. */
				if(x > tilex) {
					fromx = x - tilex;
					cwidth = destwidth - fromx;
				} else {
					/* Otherwise clip off the right part
					 * of the rectangle. */
					fromx = 0;
					cwidth = x + w - tilex;
				}
				/* Clip it to the size of the output area. */
				if(cwidth > w) cwidth = w;
				/* Clip it to the size of the output window? */
				if(cwidth > destwidth) cwidth = destwidth;
				/* Calculate the start of the area to copy
				 * from. */
				cx = tilex + fromx;
			}

			/* As above but for the Y dimension. */
			if((tiley >= y) && ((tiley + destheight) <= (y + h))) {
				fromy = 0;
				cy = tiley;
				cheight = destheight;
			} else {
				if(y > tiley) {
					fromy = y - tiley;
					cheight = destheight - fromy;
				} else {
					fromy = 0;
					cheight = y + h - tiley;
				}
				if(cheight > h) cheight = h;
				if(cheight > destheight) cheight = destheight;
				cy = tiley + fromy;
			}
			/* If the output size is non-zero after clipping, copy
			 * it to the canvas. */
			if((cwidth > 0) && (cheight > 0)) {
				GrCopyArea(state->canvas, state->gc, cx, cy,
						cwidth, cheight,
						state->background->p,
						fromx, fromy, 0);
			}
		}
	}
}

/* Fill in the intersection of the two specified rectangles: */
static void fill_in_rectangle(nbstate *state, int x, int y, int w, int h,
		int xx, int yy, int ww, int hh)
{
	int xxx, yyy;

	/* Calculate the bottom right extents of the second rectangle: */
	xxx = xx + ww;
	yyy = yy + hh;

	/* Intersect with the requested area: */
	if(x > xx) xx = x;
	if(y > yy) yy = y;
	if(x + w < xxx) xxx = x + w;
	if(y + h < yyy) yyy = y + h;

	/* Calculate the size of the resulting area: */
	ww = xxx - xx;
	hh = yyy - yy;

	/* If the result is greater than zero in size, draw it: */
	if(ww > 0 && hh > 0)
		GrFillRect(state->canvas, state->gc, xx, yy, ww, hh);

}

/* Draw the background image in the middle of the canvas over the rectangle
 * specified by x, y, w, and h. */
static void draw_centred_background(nbstate *state, int x, int y, int w, int h)
{
	int bgx, bgy, xx, yy, ww, hh;

	/* Work out where to place the background image: */
	if(state->background->w >= state->canvaswidth) bgx = 0;
	else bgx = (state->canvaswidth / 2) - (state->background->w / 2);
	if(state->background->h >= state->canvasheight) bgy = 0;
	else bgy = (state->canvasheight / 2) - (state->background->h / 2);

	/* If the area to copy intersects the area of the image: */
	if(x < bgx + state->background->w && y < bgx + state->background->h &&
						x + w > bgx && y + h > bgy) {
		/* Clip the start of the region: */
		xx = (x > bgx) ? x : bgx;
		yy = (y > bgy) ? y : bgy;
		/* Clip the end of the region: */
		ww = (x + w < bgx + state->background->w) ? x + w - xx :
		       bgx + state->background->w - xx;	
		hh = (y + h < bgy + state->background->h) ? y + h - yy :
		       bgy + state->background->h - yy;	
		/* Copy the background image. */
		if(ww > 0 && hh > 0)
			GrCopyArea(state->canvas, state->gc, xx, yy, ww, hh,
				state->background->p, xx - bgx, yy - bgy, 0);
	}
	
	/* We fill in the rest of the canvas area black: */
	GrSetGCForeground(state->gc, GR_COLOR_BLACK);

	/* Fill in the top rectangle: */
	fill_in_rectangle(state, x, y, w, h, 0, 0, state->canvaswidth, bgy);
	/* Fill in the left rectangle: */
	fill_in_rectangle(state, x, y, w, h, 0, bgy, bgx, state->background->h);
	/* Fill in the right rectangle: */
	fill_in_rectangle(state, x, y, w, h, bgx + state->background->w, bgy,
				state->canvaswidth - bgx + state->background->w,
							state->background->h);
	/* Fill in the bottom rectangle: */
	fill_in_rectangle(state, x, y, w, h, 0, bgy + state->background->h,
				state->canvaswidth, state->canvasheight - bgy +
							state->background->h);
}

/* Draw the background image onto the canvas over the rectangle specified by
 * x, y, w, and h. */
void draw_background(nbstate *state, int x, int y, int w, int h)
{
	if(state->backgroundtiled) draw_tiled_background(state, x, y, w, h);
	else draw_centred_background(state, x, y, w, h);
}

/* Draw the splash sprite (if there is one) in the centre of the canvas. */
void draw_splash(nbstate *state)
{
	if(!state->splash) return;

	GrCopyAreaAlpha(state->canvas, state->gc, state->splash->a,
			(state->canvaswidth / 2) - (state->splash->w / 2),
			(state->canvasheight / 2) - (state->splash->h / 2),
			state->splash->w, state->splash->h, state->splash->p,
			0, 0, GR_ALPHA_BLEND);
}

/* Draw the specified portion of the specified brick onto the canvas, reducing
 * the opacity of the alpha channel if it is a multiple-hit brick which has
 * been hit at least once already. */
void draw_brick(nbstate *state, grid *g, int x, int y, int w, int h)
{
	GR_ALPHA_ID a;
	int alpha = 0;

	/* If there is no brick at this grid location, return to caller. */
	if(!g->b) return;

	/* If it's a 2 hit brick and it's been hit once already, set the level
	 * to about 60%. If it's a 3 hit brick and it's been hit once, set it
	 * to about 75%. If it's a 3 hit brick and it's been hit twice, set it
	 * to just over 50% (not below because then the transparency only
	 * alpha channel drivers would draw it as completely transparent): */
	if(g->b->flags & BRICK_FLAG_2_HITS && g->hits == 1) alpha = 155;
	else if(g->b->flags & BRICK_FLAG_3_HITS) {
		if(g->hits == 1) alpha = 190;
		else if(g->hits == 2) alpha = 128;
	}

	/* If a reduced opacity level was chosen, draw a lighter version of
	 * the sprites alpha channel and use that. */
	if(alpha) {
		/* Start with black, */
		GrSetGCForeground(state->gc, GR_COLOR_BLACK);
		GrFillRect(state->brickalpha, state->gc, 0, 0,
				state->brickwidth, state->brickheight);
		/* and add the specified percentage of the original alpha
		 * channel to it: */
		GrCopyArea(state->brickalpha, state->gc, 0, 0,
				state->brickwidth, state->brickheight,
				g->b->s->a, 0, 0, GR_CONST_ADD | alpha);
		a = state->brickalpha;
	/* Normal opacity, so just use the sprites alpha channel directly. */
	} else a = g->b->s->a;

	/* Copy the specified portion of the brick onto the canvas with the
	 * chosen alpha channel. */
	GrCopyAreaAlpha(state->canvas, state->gc, a, g->x + x, g->y + y,
			w, h, g->b->s->p, x, y, GR_ALPHA_BLEND);
}

/* Draw the bricks which are in the specified area onto the canvas.
 * This is a modified version of draw_tiled_background(), so see that for the
 * comments on how it works. */
void draw_bricks(nbstate *state, int x, int y, int w, int h)
{
	int tilex = 0, tiley = 0, fromx, fromy;
	int destwidth, destheight, srcwidth, srcheight, cwidth, cheight;
	int offy = state->scores.h + state->ball.s->h + (BALLS_BORDER * 2);
	int dwidth = state->canvaswidth;
	int dheight = state->height * state->brickheight;
	int swidth = state->brickwidth;
	int sheight = state->brickheight;
	grid *g = state->grid - 1;

	y -= offy;
	if(y < 0) y = 0;

	if(swidth > dwidth) srcwidth = dwidth;
	else srcwidth = swidth;
	if(sheight > dheight) srcheight = dheight;
	else srcheight = sheight;

	for(;tiley < dheight; tiley += srcheight, tilex = 0) {
		if(tiley > (y + h)) return;
		if(y > (tiley + srcheight)) {
			g += state->width;
			continue;
		}
		if((tiley + srcheight) > dheight) destheight = dheight - tiley;
		else destheight = srcheight;
		for(;tilex < dwidth; tilex += srcwidth) {
			g++;
			if(tilex > (x + w)) continue;
			if(x > (tilex + srcwidth)) continue;
			if((tilex + srcwidth) > dwidth)
				destwidth = dwidth - tilex;
			else destwidth = srcwidth;

			if((tilex >= x) && ((tilex + destwidth) <= (x + w))) {
				fromx = 0;
				cwidth = destwidth;
			} else {
				if(x > tilex) {
					fromx = x - tilex;
					cwidth = destwidth - fromx;
				} else {
					fromx = 0;
					cwidth = x + w - tilex;
				}
				if(cwidth > w) cwidth = w;
				if(cwidth > destwidth) cwidth = destwidth;
			}

			if((tiley >= y)&&((tiley + destheight)<=(y + h))) {
				fromy = 0;
				cheight = destheight;
			} else {
				if(y > tiley) {
					fromy = y - tiley;
					cheight = destheight - fromy;
				} else {
					fromy = 0;
					cheight = y + h - tiley;
				}
				if(cheight > h) cheight = h;
				if(cheight > destheight) cheight = destheight;
			}
			if((cwidth > 0) && (cheight > 0))
				draw_brick(state, g, fromx, fromy, cwidth,
						cheight);
		}
	}
}

/* Draw the bat onto the canvas. */
void draw_bat(nbstate *state)
{
	GrCopyAreaAlpha(state->canvas, state->gc, state->bats[state->bat]->a,
			state->batx - (state->batwidths[state->bat] / 2),
			state->canvasheight - state->batheight,
			state->batwidths[state->bat], state->batheight,
			state->bats[state->bat]->p, 0, 0, GR_ALPHA_BLEND);
}

/* Draw the ball onto the canvas and remember the current position so when the
 * ball is moved, the old ball position is available to figure out where to
 * redraw the background in order to erase the old ball. */
void draw_ball(nbstate *state)
{
	GrCopyAreaAlpha(state->canvas, state->gc, state->ball.s->a,
			state->ball.x, state->ball.y, state->ball.s->w,
			state->ball.s->w, state->ball.s->p, 0, 0,
			GR_ALPHA_BLEND);
	state->ball.lx = (int)state->ball.x;
	state->ball.ly = (int)state->ball.y;
}

/* Draw the balls bar at the top of the screen below the scores strip. */
void draw_balls(nbstate *state)
{
	int i, s, x, y;

	/* Precalculate some values. */
	s = state->ball.s->w;
	x = BALLS_BORDER;
	y = state->scores.h + BALLS_BORDER;

	for(i = 0; i < state->numballs; i++) { /* For each ball. */
		/* If this ball will run off the edge of the canvas, don't
		 * draw it and return. */
		if(x + s + BALLS_BORDER > state->canvaswidth) return;
		/* Draw the ball. */
		GrCopyAreaAlpha(state->canvas, state->gc, state->ball.s->a,
				x, y, s, s, state->ball.s->p, 0, 0,
				GR_ALPHA_BLEND);
		/* Move onto the next ball position. */
		x += s + BALLS_BORDER;
	}
}

/* Draw the scores bar across the top of the screen. This is somewhat
 * complicated because we generate the alpha channel on the fly in order
 * to make the text itself have 100% opacity while the text background is
 * blended onto the canvas over the background image with partial
 * transparency. This makes the background of the text area darker and the
 * text easier to see, while still allowing you to see the background image
 * pattern through it. */
void draw_scores(nbstate *state)
{
	char buf[128];

	/* Generate the text that will be printed into the score area. */
	snprintf(buf, 128, "Score: %d   HiScore: %d   Level: %d",
			state->scores.s, state->scores.hi, state->level);

	/* Redraw the background that will be underneath the text. */
	draw_background(state, 0, 0, state->canvaswidth, state->scores.h);

	/* Fill the pixmap with the text background colour. */
	GrSetGCForeground(state->gc, SCORE_BGCOLOUR);
	GrFillRect(state->scores.p, state->gc, 0, 0, state->canvaswidth,
			state->scores.h);

	/* Draw the text onto the pixmap. */
	GrSetGCForeground(state->gc, SCORE_FGCOLOUR);
	GrSetGCBackground(state->gc, SCORE_BGCOLOUR);
	GrText(state->scores.p, state->gc, SCORE_BORDER, SCORE_BORDER, buf,
			-1, GR_TFTOP);

	/* Draw the text onto the pixmap using white (100% opacity) for the
	 * text and the alpha shade for the background. */
	GrSetGCForeground(state->gc, SCORE_ALPHACOLOUR);
	GrFillRect(state->scores.a, state->gc, 0, 0, state->canvaswidth,
			state->scores.h);
	GrSetGCForeground(state->gc, GR_COLOR_WHITE);
	GrSetGCBackground(state->gc, SCORE_ALPHACOLOUR);
	GrText(state->scores.a, state->gc, SCORE_BORDER, SCORE_BORDER, buf,
			-1, GR_TFTOP);

	/* Blend the scores pixmap onto the canvas over the clean background
	 * using the newly created alpha channel. */
	GrCopyAreaAlpha(state->canvas, state->gc, state->scores.a, 0, 0,
			state->canvaswidth, state->scores.h, state->scores.p,
			0, 0, GR_ALPHA_BLEND);
}

/* Draw the specified portion of the specified power onto the canvas. */
void draw_power(nbstate *state, power *p, int x, int y, int w, int h)
{
	sprite *s = state->powersprites[p->type];
	GrCopyAreaAlpha(state->canvas, state->gc, s->a, x,
			y, w, h, s->p, x - p->x, y - p->y,
			GR_ALPHA_BLEND);
}

/* Copy the specified area of the canvas onto the output window. */
void draw_canvas(nbstate *state, int x, int y, int w, int h)
{
	GrCopyArea(state->wid, state->gc, x, y, w, h, state->canvas, x, y, 0);
}
