#ifndef _NXLIB_H_
#define _NXLIB_H_
/*
 * NX11 X11 to Nano-X conversion library internal header file
 */

/* Changeable options*/
#define HAVE_STATICFONTS 		0	/* set to include static buffered fonts when no filesystem*/
#define MALLOC_0_RETURNS_NULL	0	/* not yet needed*/

/* required settings*/
#define NeedFunctionPrototypes	1	/* ANSI C*/
#define XLIB_ILLEGAL_ACCESS	1	/* define real structures*/

/* deal with _Xconst differences in X11 header files*/
#ifndef XCONST
#define XCONST	_Xconst
#endif

/*
 * bet you never thought you'd see both of these in the same file ;-)
 */
#include "X11/Xlib.h"
#include "../include/nano-X.h"

#include "mwconfig.h"				/* for USE_ALLOC, DPRINTF, EPRINTF*/

/* malloc stuff*/
#if MALLOC_0_RETURNS_NULL
/* for machines that do not return a valid pointer for malloc(0)*/
# define Xmalloc(size) malloc(((size) == 0 ? 1 : (size)))
# define Xrealloc(ptr, size) realloc((ptr), ((size) == 0 ? 1 : (size)))
# define Xcalloc(nelem, elsize) calloc(((nelem) == 0 ? 1 : (nelem)), (elsize))
#else
# define Xmalloc(size) malloc((size))
# define Xrealloc(ptr, size) realloc((ptr), (size))
# define Xcalloc(nelem, elsize) calloc((nelem), (elsize))
#endif
#define Xfree(ptr) free((ptr))

/* defines for unmodified (Xrm) Xlib routines...*/
//#define bzero(mem, size)	memset(mem, 0, size)
#define LockDisplay(dpy)
#define UnlockDisplay(dpy)
#define _XLockMutex(lock)
#define _XUnlockMutex(lock)
#define _XCreateMutex(lock)
#define _XFreeMutex(lock)

/* Used internally for the colormap */
typedef struct  {
	GR_PIXELVAL	value;
	int		ref;
} nxColorval;

typedef struct _nxColormap {
	int			id;
	int			color_alloc;
	int			cur_color;
	nxColorval *		colorval;
	struct _nxColormap *	next;
} nxColormap;

/* Colormap.c */
nxColormap *_nxFindColormap(Colormap id);
Colormap _nxDefaultColormap(Display *dpy);

/* Colorname.c*/
GR_COLOR GrGetColorByName(char *colorname, int *retr, int *retg, int *retb);

/* AllocColor.c*/
void _nxPixel2RGB(Display * display, unsigned long color,
	   unsigned short *red, unsigned short *green, unsigned short *blue);

/* QueryColor.c*/
GR_COLOR _nxColorvalFromPixelval(Display *dpy, unsigned long pixelval);

/* font_find.c */
typedef struct {
  char *	file;
  char *	xlfd;
  unsigned char *data;
  int 		data_size;
} nxStaticFontList;

char **font_enumfonts(char *pattern, int maxnames, int *count_return, int chkalias);
void   font_freefontnames(char **fontlist);
char * font_findfont(char *name, int height, int width, int *return_height);
int	   font_findstaticfont(char *fontname, unsigned char** data, int* size);

/* ChProperty.c */
int _nxDelAllProperty(Window w);

/* SelInput.c*/
GR_EVENT_MASK _nxTranslateEventMask(unsigned long mask);

/* CrCursor.c*/
GR_CURSOR_ID _nxCreateCursor(GR_WINDOW_ID cursor, GR_RECT * cbb,
	GR_WINDOW_ID mask, GR_RECT * mbb, int hotx, int hoty,
	GR_COLOR fg, GR_COLOR bg);

/* OpenDisp.c*/
void _XFreeDisplayStructure(Display *dpy);
extern Font _nxCursorFont;

/* CrGC.c*/
int _nxConvertROP(int Xrop);

#endif /* _NXLIB_H_*/
