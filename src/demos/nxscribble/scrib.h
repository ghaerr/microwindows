/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "hre_api.h"
#include "li_recognizer.h"

#define CS_LETTERS     0
#define CS_DIGITS      1
#define CS_PUNCTUATION 2

#define NUM_RECS    3
#define DEFAULT_REC_DIR         "classifiers"
#ifndef REC_DEFAULT_USER_DIR
#define REC_DEFAULT_USER_DIR    "bin"
#endif
/*#define REC_DEFAULT_USER_DIR    "/home/greg/net/microwin/src/apps/scribble"*/
/*#define CLASSIFIER_DIR          ".classifiers"*/
#define DEFAULT_LETTERS_FILE    "letters.cl"
#define DEFAULT_DIGITS_FILE     "digits.cl"
#define DEFAULT_PUNC_FILE       "punc.cl"
#define rec_name                "libli_recog.so"

struct graffiti {
	recognizer rec[3];     /* 3 recognizers, one each for letters, digits, 
				  and punctuation */
	char cldir[200];       /* directory in which the current classifier
				  files are found */
	li_recognizer_train rec_train; /* pointer to training function */
	li_recognizer_getClasses rec_getClasses; 
    	                       /* pointer to the function that lists
                           	  the characters in the classifier
                           	  file. */
};

typedef struct {
    /* private state */
    GR_WINDOW_ID    win;
    GR_GC_ID	    gc;
    GR_BOOL	    down;	    /* mouse is down*/
    /*GR_WINDOW_ID    lastfocusid;*/    /* last window with focus*/
    GR_POINT	    *pt;	    /* points */
    int		    ppasize;
    pen_stroke	    ps;
    struct graffiti graf;
    int		    capsLock;
    int		    puncShift;
    int		    tmpShift;
    int		    ctrlShift;
    int		    curCharSet;
    int		    lastchar;
} ScribbleRec, *ScribbleWidget;

ScribbleWidget	create_scribble(void);
void		destroy_scribble(ScribbleWidget w);
void		ActionStart(ScribbleWidget w, int x, int y);
void		ActionMove(ScribbleWidget w, int x, int y);
void		ActionEnd(ScribbleWidget w, int x, int y);
void		Redisplay (ScribbleWidget w);
