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

/* sprite.c contains sprite creation and destruction functions. NanoBreaker
 * sprites basically consist of a Nano-X pixmap and a Nano-X alpha channel,
 * with their dimensions and the name of the file they were loaded from.
 * A list is kept of all the sprites, and when a sprite is asked to be created
 * from an image which has already been loaded (with the same dimensions) the
 * existing sprite is returned instead and the sprites usage count incremented.
 * When a sprite is asked to be destroyed, the usage count is decremented and
 * the sprite is only actually destroyed when the count reaches 0. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nano-X.h>
#include "nbreaker.h"

/* Create a new sprite structure with the specified file name, dimensions,
 * and pixmap/alpha channel IDs, then set the usage count to 1 and link it
 * into the sprite list. Called by make_empty_sprite() and load_sprite(). */
static sprite *make_sprite(nbstate *state, char *fname, int w, int h,
		GR_PIXMAP_ID p, GR_ALPHA_ID a)
{
	sprite *s;

	/* Allocate the sprite structure: */
	if(!(s = malloc(sizeof(sprite)))) {
		oom();
		return NULL;
	}

	/* Copy the file name string: */
	if(!fname) s->fname = NULL;
	else {
		if(!(s->fname = strdup(fname))) {
			oom();
			free(s);
			return NULL;
		}
	}

	/* Set the various parameters: */
	s->w = w;
	s->h = h;
	s->p = p;
	s->a = a;
	s->usage = 1;

	/* Link it into the sprite list: */
	s->next = state->spritelist;
	if(s->next) s->next->prev = s;
	s->prev = NULL;
	state->spritelist = s;

	return s; /* Return the address of the new sprite structure. */
}

/* Make an empty sprite structure with the specified file name, width, and
 * height. It is expected that you will probably already have tried calling
 * load_sprite() first and it failed, so you want to create the sprite
 * manually. */
sprite *make_empty_sprite(nbstate *state, char *fname, int width, int height)
{
	sprite *s;
	GR_ALPHA_ID a;
	GR_PIXMAP_ID p;

	/* Allocate the alpha channel and pixmap with the specified
	 * dimensions (which must be valid): */
	if(!(a = GrNewAlpha(width, height)))
		return NULL;
	if(!(p = GrNewPixmap(width, height, NULL))) {
		GrDestroyWindow(a);
		return NULL;
	}

	/* Make the sprite itself and return it to the caller: */
	if(!(s = make_sprite(state, fname, width, height, p, a))) {
		GrDestroyWindow(a);
		GrDestroyWindow(p);
		return NULL;
	} else return s;
}

/* Create a new sprite from the specified image file. Returns the address of
 * the new sprite on success or NULL on failure. If width is -1, the real
 * dimensions of the image file will be used, otherwise the image will be
 * scaled up or down to the specified dimensions. */
sprite *load_sprite(nbstate *state, char *fname, int width, int height)
{
	sprite *s;
	char buf[256];
	GR_ALPHA_ID a;
	GR_PIXMAP_ID p;
	GR_IMAGE_ID img;
	GR_IMAGE_INFO ii;

	/* Make sure the file name has been specified: */
	if(!fname) return NULL;

	/* Try to find a sprite in the list with the specified filename and
	 * dimensions (any dimensions are OK if width is -1). If one is found,
	 * increment its usage count and return its address. */
	for(s = state->spritelist; s; s = s->next) {
		if((width == -1 || (width == s->w && height == s->h)) &&
				s->fname && !strcmp(fname, s->fname)) {
			s->usage++;
			return s;
		}
	}

	/* Make the full path to the filename because the Nano-X server
	 * probably isn't running from the game data directory: */
	if(snprintf(buf, 256, "%s/%s", state->gamedir, fname) >= 256){
		fprintf(stderr, "Warning: image path \"%s/%s\" is too long\n",
				state->gamedir, state->gamefile);
		return NULL;
	}

	/* Try to load the image file, and return NULL on failure: */
	if(!(img = GrLoadImageFromFile(buf, 0))) {
		fprintf(stderr, "Warning: failed to load image \"%s\"- make "
				"sure it is where the server can find it and "
				"that support for loading the relevant image "
				"type has been built into the server\n", buf);
		return NULL;
	}

	/* If a size wasn't specified, get the real image size from the server
	 * instead: */
	if(width == -1 || height == -1) {
		GrGetImageInfo(img, &ii);
		width = ii.width;
		height = ii.height;
	}

	/* Make the alpha channel and pixmap to store the image in: */
	if(!(a = GrNewAlpha(width, height))) {
		GrFreeImage(img);
		return NULL;
	}
	if(!(p = GrNewPixmap(width, height, NULL))) {
		GrFreeImage(img);
		GrDestroyWindow(a);
		return NULL;
	}

	/* Draw the image into the specified pixmap and alpha channel, scaling
	 * it up or down if necessary: */
	GrDrawImageToFit(p, state->gc, 0, 0, width, height, img, a);
	GrFreeImage(img); /* Destroy the server image object. */

	/* Make a new sprite and link it into the list, then return its
	 * address to the caller: */
	if(!(s = make_sprite(state, fname, width, height, p, a))) {
		GrDestroyWindow(a);
		GrDestroyWindow(p);
		return NULL;
	} else return s;
}

/* Destroy the specified sprite. Should be called whenever you don't need a
 * particular sprite any more, even though it doesn't actually destroy the
 * sprite until all users of the sprite have called destroy_sprite(). */
void destroy_sprite(nbstate *state, sprite *s)
{
	if(!s) return; /* Sanity check. */

	if(!--s->usage) { /* Decrement the usage count. */
		/* The usage count is zero, so destroy the sprite: */
		myfree(s->fname); /* Free the file name. */
		GrDestroyWindow(s->p); /* Destroy the pixmap. */
		GrDestroyWindow(s->a); /* Destroy the alpha channel. */
		/* Unlink it from the sprite list: */
		if(s == state->spritelist) state->spritelist = s->next;
		else s->prev->next = s->next;
		if(s->next) s->next->prev = s->prev;
		free(s); /* Free the structure itself. */
	}
}

/* Destroy every sprite in the sprite list in one go. More efficient than
 * calling destroy_sprite() for every use of every sprite. */
void destroy_all_sprites(nbstate *state)
{
	sprite *s, *snext = state->spritelist;

	while((s = snext)) { /* Iterate through the list. */
		myfree(s->fname); /* Free the file name. */
		GrDestroyWindow(s->p); /* Destroy the pixmap. */
		GrDestroyWindow(s->a); /* Destroy the alpha channel. */
		snext = s->next;
		free(s); /* Free the structure itself. */
	}
	state->spritelist = NULL; /* Set the list head empty. */
}
