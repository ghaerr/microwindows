/*
 * snap_ppm - screen snapshot for Nano-X, ppm format
 */
/*
 * A simple Nano-X screenshot program using GrReadArea().
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
 * The Original Code is Snap.
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <nano-X.h>

struct snap_state {
	char *outname;
	GR_PIXELVAL *pixels;
	GR_SCREEN_INFO sinfo;
};
typedef struct snap_state snapstate;

static void oom(void)
{
	fprintf(stderr, "Out of memory\n");
}

static snapstate *init(int argc, char *argv[])
{
	snapstate *state;

	if(argc != 2) {
		fprintf(stderr, "Usage: snap output.ppm\n");
		return NULL;
	}

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return NULL;
	}

	if(!(state = malloc(sizeof(snapstate)))) {
		oom();
		return NULL;
	}

	state->outname = argv[1];

	GrGetScreenInfo(&state->sinfo);

	if(!(state->pixels = malloc(sizeof(GR_PIXELVAL) * state->sinfo.rows
					* state->sinfo.cols))) {
		free(state);
		oom();
		return NULL;
	}

	return(state);
}

static int writeout(snapstate *state)
{
	FILE *fp;
	int x, y;
	unsigned char rgb[3], *pp;
	GR_PALETTE *palette = NULL;

	if(!(fp = fopen(state->outname, "w"))) {
		fprintf(stderr, "Couldn't open output file \"%s\": %s\n",
				state->outname, strerror(errno));
		return 1;
	}

	if(fprintf(fp, "P6\n%d %d\n255\n", state->sinfo.cols, state->sinfo.rows)
									< 0)
		goto badwrite;

	if(state->sinfo.pixtype == MWPF_PALETTE) {
		if(!(palette = malloc(sizeof(GR_PALETTE)))) {
			oom();
			return 1;
		}
		GrGetSystemPalette(palette);
	}

	for(y = 0; y < state->sinfo.rows; y++) {
		for(x = 0; x < state->sinfo.cols; x++) {

			pp = (unsigned char *)state->pixels +
				((x + (y * state->sinfo.cols)) *
				 sizeof(GR_PIXELVAL));

			switch(state->sinfo.pixtype) {
			/* FIXME: These may need modifying on big endian. */
				case MWPF_TRUECOLOR0888:
				case MWPF_TRUECOLOR888:
					rgb[0] = pp[2];
					rgb[1] = pp[1];
					rgb[2] = pp[0];
					break;
				case MWPF_PALETTE:
					rgb[0] = palette->palette[pp[0]].r;
					rgb[1] = palette->palette[pp[0]].g;
					rgb[2] = palette->palette[pp[0]].b;
					break;
				case MWPF_TRUECOLOR565:
					rgb[0] = pp[1] & 0xf8;
					rgb[1] = ((pp[1] & 0x07) << 5) |
						((pp[0] & 0xe0) >> 3);
					rgb[2] = (pp[0] & 0x1f) << 3;
					break;
				case MWPF_TRUECOLOR555:
					rgb[0] = (pp[1] & 0x7c) << 1;
					rgb[1] = ((pp[1] & 0x03) << 6) |
						((pp[0] & 0xe0) >> 2);
					rgb[2] = (pp[0] & 0x1f) << 3;
					break;
				case MWPF_TRUECOLOR332:
					rgb[0] = pp[0] & 0xe0;
					rgb[1] = (pp[0] & 0x1c) << 3;
					rgb[2] = (pp[0] & 0x03) << 6;
					break;
				default:
					fprintf(stderr, "Unsupported pixel "
							"format\n");
					fclose(fp);
					return 1;
			}

			if(fwrite(rgb, 3, 1, fp) < 0) goto badwrite;
		}
	}
	
	fclose(fp);
	if(palette) free(palette);
	return 0;

badwrite:
	fprintf(stderr, "Error writing to output file: %s\n", strerror(errno));
	fclose(fp);
	if(palette) free(palette);
	return 1;
}

static void cleanup(snapstate *state)
{
	if(state) {
		if(state->pixels) free(state->pixels);
		free(state);
	}
	GrClose();
}

int main(int argc, char *argv[])
{
	int ret = 0;
	snapstate *state = NULL;

	if((state = init(argc, argv))) {
		GrReadArea(GR_ROOT_WINDOW_ID, 0, 0, state->sinfo.cols,
			state->sinfo.rows, state->pixels);
		if(writeout(state)) ret = 2;
	} else ret = 1;

	cleanup(state);
	return ret;
}
