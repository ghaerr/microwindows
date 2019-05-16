/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Nuklear demo for Nano-X
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_NANOX_IMPLEMENTATION
/*#define NK_NANOX_INCLUDE_STB_IMAGE*/
#include "nano-X.h"
#include "nuklear.h"
#include "nuklear_nxlib.h"

typedef struct NXWindow NXWindow;
struct NXWindow {
    GR_WINDOW_ID win;
    NXFont *font;
};

/* ===============================================================
 *
 *                          EXAMPLES
 *
 * ===============================================================*/
#ifdef INCLUDE_STYLE
  #include "style.c"
#endif
#ifdef INCLUDE_DEMO
  #include "demo.c"
#endif
#ifdef INCLUDE_CALCULATOR
  #include "calculator.c"
#endif
#ifdef INCLUDE_OVERVIEW
  #include "overview.c"
#endif
#ifdef INCLUDE_NODE_EDITOR
  #include "node_editor.c"
#endif

/* ===============================================================
 *
 *                          MAIN
 *
 * ===============================================================*/
int
main(void)
{
    int running = 1;
    NXWindow nxw;
    struct nk_context *ctx;

    /* Nano-X*/
    memset(&nxw, 0, sizeof nxw);
	if (GrOpen() < 0) {
    	fputs("Unable to open graphics", stderr);
		exit(EXIT_FAILURE);
	}

	/* note window width/height is less 1 because Nuklear sizing is off by 1*/
	nxw.win = GrNewBufferedWindow(GR_WM_PROPS_APPWINDOW, "Nuklear on Microwindows", GR_ROOT_WINDOW_ID,
		0, 0, WINDOW_WIDTH-1, WINDOW_HEIGHT-1, GR_RGB(30,30,30));
	GrSelectEvents(nxw.win, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_UPDATE |
		GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP |
		GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_MOUSE_POSITION);
	GrMapWindow(nxw.win);

    /* GUI */
    nxw.font = nk_nxfont_create(GR_FONT_SYSTEM_VAR);
    ctx = nk_nxlib_init(nxw.font, nxw.win, WINDOW_WIDTH, WINDOW_HEIGHT);

    #ifdef INCLUDE_STYLE
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/
    #endif

    while (running)
    {
        /* Input */
        GR_EVENT evt;
        nk_input_begin(ctx);
		do {
			GrGetNextEvent(&evt);
            if (evt.type == GR_EVENT_TYPE_CLOSE_REQ)
				running = 0;
            nk_nxlib_handle_event(&evt);
        }
        while (running && GrPeekEvent(&evt));
        nk_input_end(ctx);

		if (!running)
			break;

        /* GUI */
        /* -------------- EXAMPLES ---------------- */
        #ifdef INCLUDE_DEMO
          demo(ctx);
        #endif
        #ifdef INCLUDE_CALCULATOR
          calculator(ctx);
        #endif
        #ifdef INCLUDE_OVERVIEW
          overview(ctx);
        #endif
        #ifdef INCLUDE_NODE_EDITOR
          node_editor(ctx);
        #endif
        /* ----------------------------------------- */

        /* Draw */
        nk_nxlib_render(nxw.win, nk_rgb(30,30,30));
		GrFlushWindow(nxw.win);
        GrFlush();
    }

    nk_nxfont_del(nxw.font);
    nk_nxlib_shutdown();
	GrDestroyWindow(nxw.win);
	GrClose();
    return 0;
}
