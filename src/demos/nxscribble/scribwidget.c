/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2000 Century Software <embedded.centurysoftware.com>
 * Scribble Handwriting Recognition for Nano-X!
 * Scribble object routines
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

#include <stdio.h>
#include <string.h>
#include "scrib.h"

static ScribbleRec	scrib;		/* static object instance*/
static char *curmsg = NULL;

static char *cl_name[3] = {DEFAULT_LETTERS_FILE, 
			     DEFAULT_DIGITS_FILE,
			     DEFAULT_PUNC_FILE};

static int graffiti_load_recognizers(struct graffiti *pg);
static void Recognize (ScribbleWidget w);
static void ShowMode (ScribbleWidget w);
    
static void
ResetStroke (ScribbleWidget w)
{
    w->ps.ps_npts = 0;
    w->ps.ps_nstate = 0;
    w->ps.ps_trans = 0;
    w->ps.ps_state = 0;
    w->lastchar = 0;
    curmsg = NULL;
    ShowMode(w);
}

static void
DisplayStroke (ScribbleWidget	w)
{
    GrDrawLines(w->win, w->gc, w->pt, w->ps.ps_npts);
}

static void
DisplayLast (ScribbleWidget w)
{
    int	    npt;
    
    npt = w->ps.ps_npts;
    if (npt > 2)
	npt = 2;
    GrDrawLines(w->win, w->gc, w->pt + (w->ps.ps_npts - npt), npt);
}

static void
AddPoint (ScribbleWidget w, int x, int y)
{
    pen_point	*ppa;
    GR_POINT	*pt;
    int		ppasize;
    
    if (w->ps.ps_npts == w->ppasize)
    {
	ppasize = w->ppasize + 100;
	ppa = malloc ((sizeof (pen_point) + sizeof (GR_POINT)) * ppasize);
	if (!ppa)
	    return;
	pt = (GR_POINT *) (ppa + ppasize);
	memcpy (ppa, w->ps.ps_pts, w->ppasize * sizeof (pen_point));
	memcpy (pt, w->pt, w->ppasize * sizeof (GR_POINT));
	free (w->ps.ps_pts);
	w->ps.ps_pts = ppa;
	w->pt = pt;
	w->ppasize = ppasize;
    }
    ppa = &w->ps.ps_pts[w->ps.ps_npts];
    ppa->x = x;
    ppa->y = y;
    
    pt = &w->pt[w->ps.ps_npts];
    pt->x = x;
    pt->y = y;
    
    w->ps.ps_npts++;
    
    DisplayLast (w);
}

ScribbleWidget
create_scribble(void)
{
    ScribbleWidget new = (ScribbleWidget)&scrib;
    GR_WM_PROPERTIES props;

    new->capsLock = 0;
    new->puncShift = 0;
    new->tmpShift = 0;
    new->ctrlShift = 0;
    new->curCharSet = CS_LETTERS;
    new->lastchar = 0;
    new->down = GR_FALSE;
    /*new->lastfocusid = 0;*/
    
    graffiti_load_recognizers (&new->graf);
    
    new->ppasize = 0;
    new->ps.ps_pts = 0;
    new->pt = 0;
    
    new->win = GrNewWindow(GR_ROOT_WINDOW_ID,
		    350, 20, 200, 150,
		    0, GrGetSysColor(GR_COLOR_APPWINDOW), BLACK);
    /* set title, disallow focus on input window*/
    props.flags = GR_WM_FLAGS_TITLE | GR_WM_FLAGS_PROPS;
    props.props = GR_WM_PROPS_NOFOCUS | GR_WM_PROPS_BORDER |
	    GR_WM_PROPS_CAPTION | GR_WM_PROPS_CLOSEBOX;
    props.title = "nxScribble";
    GrSetWMProperties(new->win, &props);

    GrSelectEvents(new->win, GR_EVENT_MASK_BUTTON_DOWN |
		GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_MOUSE_MOTION |
		GR_EVENT_MASK_KEY_DOWN | /*GR_EVENT_MASK_FOCUS_IN |*/
		GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(new->win);

    new->gc = GrNewGC();
    GrSetGCForeground(new->gc, GrGetSysColor(GR_COLOR_APPTEXT));
    GrSetGCBackground(new->gc, GrGetSysColor(GR_COLOR_APPWINDOW));
    GrSetGCFont(new->gc, GrCreateFont(GR_FONT_OEM_FIXED, 0, NULL));

    ResetStroke (new);
    return new;
}

void
destroy_scribble(ScribbleWidget w)
{
    GrDestroyWindow(w->win);
    GrDestroyGC(w->gc);
    free (w->ps.ps_pts);
}

void
Redisplay (ScribbleWidget w)
{
    /*DisplayStroke (w);*/
    ShowMode(w);
}

void
ActionStart (ScribbleWidget w, int x, int y)
{
    GrRaiseWindow(w->win);
    ResetStroke (w);
    w->down = GR_TRUE;
    AddPoint (w, x, y);
}

void
ActionMove (ScribbleWidget w, int x, int y)
{
    if (w->down)
    	AddPoint (w, x, y);
}

void
ActionEnd (ScribbleWidget w, int x, int y)
{
    AddPoint (w, x, y);
    w->down = GR_FALSE;
    Recognize (w);
}


static void
SendKey(ScribbleWidget w, int ch)
{
	GR_WINDOW_ID	win = GrGetFocus();

	/* FIXME: modifiers are incorrect*/
	GrInjectKeyboardEvent(win, ch, 0, 0, 1);
	GrInjectKeyboardEvent(win, ch, 0, 0, 0);
}

/* This procedure is called to initialize pg by loading the three
   recognizers, loading the initial set of three classifiers, and
   loading & verifying the recognizer extension functions.  If the
   directory $HOME/.recognizers exists, the classifier files will be
   loaded from that directory.  If not, or if there is an error, the
   default files (directory specified in Makefile) will be loaded
   instead.  Returns non-zero on success, 0 on failure.  (Adapted from
   package tkgraf/src/GraffitiPkg.c. */

static int
graffiti_load_recognizers(struct graffiti *pg)
{
	bool usingDefault;
#if 0
	char* homedir;
#endif
	int i;
	rec_fn *fns;

	/* First, load the recognizers... */
	/* call recognizer_unload if an error ? */
	for (i = 0; i < NUM_RECS; i++) {
		/* Load the recognizer itself... */
		pg->rec[i] = recognizer_load(DEFAULT_REC_DIR, rec_name, NULL);
		if (pg->rec[i] == NULL) {
			fprintf(stderr,"Error loading recognizer from %s.", DEFAULT_REC_DIR);
			return 0;
		}
		if ((* (int *)(pg->rec[i])) != 0xfeed) {
			fprintf(stderr,"Error in recognizer_magic.");
			return 0;
		}
	}

	/* ...then figure out where the classifiers are... */
#if 0
	if ( (homedir = (char*)getenv("HOME")) == NULL ) {
#endif
		strcpy(pg->cldir, REC_DEFAULT_USER_DIR);
		usingDefault = true;
#if 0
	} else {
		strcpy(pg->cldir, homedir);
		strcat(pg->cldir, "/"); 
		strcat(pg->cldir, CLASSIFIER_DIR); 
		usingDefault = false;
	}
#endif

	/* ...then load the classifiers... */
	for (i = 0; i < NUM_RECS; i++) {
		int rec_return;
		char *s;

		rec_return = recognizer_load_state(pg->rec[i], pg->cldir, cl_name[i]);
		if ((rec_return == -1) && (usingDefault == false)) {
			fprintf(stderr,
				"Unable to load custom classifier file %s/%s.\nTrying default classifier file instead.\nOriginal error: %s\n ", 
				pg->cldir, cl_name[i], 
				(s = recognizer_error(pg->rec[i])) ? s : "(none)");
			rec_return = recognizer_load_state(pg->rec[i],
							   REC_DEFAULT_USER_DIR, cl_name[i]);
		}
		if (rec_return == -1) {
			fprintf(stderr, "Unable to load default classifier file %s.\nOriginal error: %s\n",
				cl_name[i], 
				(s = recognizer_error(pg->rec[i])) ? s : "(none)");
			return 0;
		}
	}

	/* We have recognizers and classifiers now.   */
	/* Get the vector of LIextension functions..     */
	fns = recognizer_get_extension_functions(pg->rec[CS_LETTERS]);
	if (fns == NULL) {
		fprintf(stderr, "LI Recognizer Training:No extension functions!");
		return 0;
	}
	
	/* ... and make sure the training & get-classes functions are okay. */
	if( (pg->rec_train = (li_recognizer_train)fns[LI_TRAIN]) == NULL ) {
		fprintf(stderr,
			"LI Recognizer Training:li_recognizer_train() not found!");
		if (fns != NULL) {
			free(fns);
		}
		return 0;
	}
  
	if( (pg->rec_getClasses = (li_recognizer_getClasses)fns[LI_GET_CLASSES]) == NULL ) {
		fprintf(stderr,
			"LI Recognizer Training:li_recognizer_getClasses() not found!");
		if (fns != NULL) {
			free(fns);
		}
		return 0;
	}
	free(fns);
	return 1;
}

static void
msg(char *str)
{
	curmsg = str;
}

static void
ShowMode (ScribbleWidget w)
{
    char *mode;
    char buf[32];

    if (w->ctrlShift)
	mode = "^C";
    else if (w->puncShift)
	mode = "#&^";
    else if (w->curCharSet == CS_DIGITS)
	mode = "123";
    else if (w->capsLock)
	mode = "ABC";
    else if (w->tmpShift)
	mode = "Abc";
    else
	mode = "abc";

    if (curmsg)
    	sprintf(buf, "%s  %s", mode, curmsg);
    else if (w->lastchar > ' ')
    	sprintf(buf, "%s    %c", mode, w->lastchar);
    else sprintf(buf, "%s", mode);
    GrClearWindow(w->win, GR_FALSE);
    GrText(w->win, w->gc, 70, 0, buf, strlen(buf), GR_TFTOP);
}

static char
do_recognize(struct graffiti *pg, pen_stroke *ps, int charset)
{
       int rec_char;
       int nret;
       rec_alternative *ret;

       rec_char = recognizer_translate(pg->rec[charset], 1, ps, false,
				       &nret, &ret);
       if (rec_char != -1) {
	       delete_rec_alternative_array(nret, ret, false);
       }
       return rec_char;
}

typedef int KeySym;

static void
Recognize (ScribbleWidget w)
{
    struct graffiti *graf = &w->graf;
    pen_stroke	    *ps = &w->ps;
    KeySym	    keysym;
    GR_BOOL	    control;
    char	    c;
    
    if (ps->ps_npts == 0)
	return;
    
    w->lastchar = 0;

    c = do_recognize(graf, ps, w->puncShift ? CS_PUNCTUATION : w->curCharSet);

    /*printf("class %c (%d)\n", c, c);*/

    switch (c) {
    case '\000':
msg("[Error]");
	w->tmpShift = 0;
	w->puncShift = 0;
	w->ctrlShift = 0;
	ShowMode (w);
	break;
    case 'L': /* caps lock */
msg("[Capslock]");
	w->capsLock = !w->capsLock;
	ShowMode (w);
	break;
    case 'N': /* numlock */
	if (w->curCharSet == CS_DIGITS) {
	    w->curCharSet = CS_LETTERS;
msg("[Letters]");
	} else {
	    w->curCharSet = CS_DIGITS;
msg("[Digits]");
	}
	w->tmpShift = 0;
	w->puncShift = 0;
	w->ctrlShift = 0;
	ShowMode (w);
	break;
    case 'P': /* usually puncshift, but we'll make it CTRL */
msg("[Ctrlshift]");
	w->ctrlShift = !w->ctrlShift;
	w->tmpShift = 0;
	w->puncShift = 0;
	ShowMode (w);
	break;
    case 'S': /* shift */
	w->tmpShift = !w->tmpShift;
if (w->tmpShift) msg("[Shift]"); else msg("[Unshift]");
	w->puncShift = 0;
	w->ctrlShift = 0;
	ShowMode (w);
	break;
    default:
	control = GR_FALSE;
	switch (c) {
	case 'A':
msg("[Space]");
	    keysym = ' ';
	    break;
	case 'B':
msg("[Backspace]");
	    keysym = '\b';
	    break;
	case 'R':
msg("[Return]");
	    keysym = '\r';
	    break;
	case '.':
	    if (! w->puncShift) {
msg("[Puncshift]");
		w->puncShift = 1;
		w->ctrlShift = 0;
		w->tmpShift = 0;
		ShowMode (w);
		return;
	    } else {
		w->puncShift = 0;
		ShowMode (w);
	    }		  	
	    keysym = '.';
	    break;
	default:
	    if ('A' <= c && c <= 'Z') {
msg("[Notimp]");
		ShowMode (w);
		return;
	    }
	    keysym = (KeySym) c;
	    if (w->ctrlShift) 
	    {
		control = GR_TRUE;
		w->ctrlShift = 0;
		if (c < 'a' || 'z' < c)
		{
		    ShowMode (w);
		    return;
		}
	    } 
	    else if ((w->capsLock && !w->tmpShift) || 
		     (!w->capsLock && w->tmpShift)) 
	    {
		keysym = keysym-'a'+'A';
	    } 
	    w->tmpShift = 0;
	    w->puncShift = 0;
	    ShowMode(w);
	}
	
	if (control)
		keysym &= 0x1f;
	w->lastchar = keysym;
	ShowMode(w);
	SendKey(w, keysym);
	break;
    }
}
