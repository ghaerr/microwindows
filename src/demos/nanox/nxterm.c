/*
 * nxterm - terminal emulator for Nano-X
 *
 * (C) 1994,95,96 by Torsten Scherer (TeSche)
 * itschere@techfak.uni-bielefeld.de
 *
 * - quite some changes for W1R1
 * - yet more changes for W1R2
 *
 * TeSche 01/96:
 * - supports W_ICON & W_CLOSE
 * - supports /etc/utmp logging for SunOS4 and Linux
 * - supports catching of console output for SunOS4
 * Phx 02-06/96:
 * - supports NetBSD-Amiga
 * Eero 11/97:
 * - unsetenv(DISPLAY), setenv(LINES, COLUMNS).
 * - Add new text modes (you need to use terminfo...).
 * Eero 2/98:
 * - Implemented fg/bgcolor setting.  With monochrome server it changes
 *   bgmode variable, which tells in which mode to draw to screen
 *   (M_CLEAR/M_DRAW) and affects F_REVERSE settings.
 * - Added a couple of checks.
 *
 * TODO:
 * - Allocate and set sensible window palette for fg/bg color setting.
 * - add scroll-region ('cs') command. Fairly many programs
 *   can take advantage of that.
 * - Add xterm like mouse event to terminfo key event conversion... :)
 * - check if current NetBSD really needs ifdefs below with
 *   current W server/library.
 */
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <utmp.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
#include "nxterm.h"

#define TITLE	"nxterm"

#define	SMALLBUFFER 80
#define	LARGEBUFFER 1024

/*
 * some pty helper functions
 */
#ifdef linux
#define NSIG _NSIG
#endif

#ifdef __FreeBSD__
#include <libutil.h>
#endif


/*
 * some global variables
 */

static GR_WINDOW_ID	w1;	/* id for window */
static GR_GC_ID		gc1;	/* graphics context */
static GR_FONT_ID       regFont;
/*static GR_FONT_ID       boldFont;*/

static GR_SCREEN_INFO	si;	/* screen info */
static GR_FONT_INFO     fi;     /* Font Info */
static GR_WINDOW_INFO   wi;
static GR_GC_INFO       gi;
static GR_BOOL		havefocus = GR_FALSE;

static short winw, winh, pid, console;
static int pipeh;
static short cblink = 0, visualbell = 0, debug = 0;
#ifdef __FreeBSD__
static char pty[SMALLBUFFER];
static struct winsize winsz;
#endif

#define fonh fi.height
#define fonw fi.maxwidth

int term_init();

/****************************************************************************/

/*
 *
 */

/* static int isIconified; */
static int isMaximized=0;

void maximize(void)
{
    static short x0, y0, w, h, w_max,h_max;

    if (!isMaximized) 
    {
      
	w_max=si.cols-wi.bordersize;
	h_max=si.rows-wi.bordersize;
	GrMoveWindow(w1,0,0);
	GrResizeWindow(w1,w_max, h_max);
	isMaximized=1;
    } 
    else 
    {
	GrResizeWindow(w1, w, h);
	GrMoveWindow(w1, x0, y0);
	isMaximized=0;
    }
}


/****************************************************************************/


/*
 * some common tool functions
 */

void sigpipe(int sig)
{
  /* this one musn't close the window */
/*  _write_utmp(pty, "", "", 0);  */
  kill(-pid, SIGHUP);
  _exit(sig);
}


void sigchld(int sig)
{
/*  _write_utmp(pty, "", "", 0);  */
  _exit(sig);
}


void sigquit(int sig)
{
  signal(sig, SIG_IGN);
  kill(-pid, SIGHUP);
}


/*
 * this is the wterm terminal code, almost like VT52
 */

short	bgmode, escstate, curx, cury, curon, curvis;
short	savx, savy, wrap, style;
short	col, row, colmask = 0x7f, rowmask = 0x7f;

/*
 * something to buffer plain text output
 */

short	sbufcnt = 0;
short	sbufx, sbufy;
char    lineBuffer[SMALLBUFFER+1];
char	*sbuf=lineBuffer;

void sflush(void)
{
    if (sbufcnt) 
    {
	GrText(w1,gc1, sbufx*fonw, sbufy*fonh, sbuf, sbufcnt, GR_TFTOP);
	sbufcnt = 0;
    }
}


void lineRedraw(void)
{
    GrSetGCForeground(gc1,gi.background);
    GrFillRect(w1, gc1, curx*fonw, cury*fonh, (col-curx)*fonw, fonh);
    GrSetGCForeground(gc1,gi.foreground);

    if (sbufcnt) 
    {
	sbuf[sbufcnt] = 0;
	GrText(w1,gc1, sbufx*fonw, sbufy*fonh, sbuf, sbufcnt, GR_TFTOP);
    }
}

void sadd (char c)
{
    if (sbufcnt == SMALLBUFFER)
    {
	sflush ();
    }

    if (!sbufcnt)
    { 
 	sbufx = curx; 
 	sbufy = cury; 
    } 

    sbuf[sbufcnt++] = c;
}

void show_cursor (void)
{
	GrSetGCMode(gc1,GR_MODE_XOR);
	GrSetGCForeground(gc1, WHITE);
	GrFillRect(w1, gc1, curx*fonw, cury*fonh+1, fonw, fonh-1);
	GrSetGCForeground(gc1, gi.foreground);
	GrSetGCMode(gc1,GR_MODE_COPY);
}


void draw_cursor (void)
{
    if (!curvis)
    {
	curvis = 1;
	show_cursor();
    }
}
void hide_cursor (void)
{
    if (curvis) 
    {
	curvis = 0;
	show_cursor();
    }
}


void vscroll(int lines)
{
    hide_cursor();
    GrCopyArea(w1,gc1,0, 0, winw, winh-(lines*fonh),
	       w1, 0, (lines*fonh), MWROP_SRCCOPY);
    
    GrSetGCForeground(gc1,gi.background);    
    GrFillRect(w1, gc1, 0, winh-(lines*fonh),
	       winw, (lines*fonh));
    GrSetGCForeground(gc1,gi.foreground);    
}

void esc5(unsigned char c)	/* setting background color */
{

    GrSetGCBackground(gc1, c);
    GrGetGCInfo(gc1,&gi);
    escstate = 0;
}

void esc4(unsigned char c)	/* setting foreground color */
{
    GrSetGCForeground(gc1,c);
    GrGetGCInfo(gc1,&gi);
    escstate = 0;
}

void esc3(unsigned char c)	/* cursor position x axis */
{
    curx = (c - 32) & colmask;
    if (curx >= col)
	curx = col - 1;
    else if (curx < 0)
	curx = 0;
    escstate = 0;
}


void esc2(unsigned char c)	/* cursor position y axis */
{
  cury = (c - 32) & rowmask;
  if (cury >= row)
    cury = row - 1;
  else if (cury < 0)
    cury = 0;
  escstate = 3;
}


void esc1(unsigned char c)	/* various control codes */
{
    static int ReverseMode=0;
    escstate = 0;

    switch(c) 
    {
    case 'A':/* cursor up */
	hide_cursor();
	if ((cury -= 1) < 0)
	    cury = 0;
	break;

    case 'B':/* cursor down */
	hide_cursor();
	if ((cury += 1) >= row)
	    cury = row - 1;
	break;

    case 'C':/* cursor right */
	hide_cursor();
	if ((curx += 1) >= col)
	    curx = col - 1;
	break;

    case 'D':/* cursor left */
	hide_cursor();
	if ((curx -= 1) < 0)
	    curx = 0;
	break;

    case 'E':/* clear screen & home */
	GrClearWindow(w1, 0);
	curx = 0;
	cury = 0;
	break;

    case 'H':/* cursor home */
	curx = 0;
	cury = 0;
	break;

    case 'I':/* reverse index */
	if ((cury -= 1) < 0) 
	{
	    cury = 0;
	    vscroll(1);
	}
	break;

    case 'J':/* erase to end of page */

 	if (cury < row-1) 
	{
	    GrSetGCForeground(gc1,gi.background);
	    GrFillRect(w1,gc1, 0,(cury+1)*fonh, winw, (row-1-cury)*fonh);
	    GrSetGCForeground(gc1,gi.foreground);
	}
	GrSetGCForeground(gc1,gi.background);
 	GrFillRect(w1, gc1, curx*fonw, cury*fonh, (col-curx)*fonw, fonh);
	GrSetGCForeground(gc1,gi.foreground);
	break;

    case 'K':/* erase to end of line */
	GrSetGCForeground(gc1,gi.background);
	GrFillRect(w1, gc1, curx*fonw, cury*fonh, (col-curx)*fonw, fonh);
	GrSetGCForeground(gc1,gi.foreground);
	break;

    case 'L':/* insert line */
 	if (cury < row-1) 
	{
	    vscroll(1);
	}
 	curx = 0;
	break;

    case 'M':/* delete line */
 	if (cury < row-1) 
	{ 
	    vscroll(1);
 	}
 	curx = 0;
	break;

    case 'Y':/* position cursor */
	escstate = 2;
	break;

    case 'b':/* set foreground color */
	escstate = 4;
	break;

    case 'c':/* set background color */
	escstate = 5;
	break;

    case 'd':/* erase beginning of display */
/* 	w_setmode(win, bgmode); */
 	if (cury > 0) 
	{
	    GrSetGCForeground(gc1,gi.background);
	    GrFillRect(w1,gc1, 0, 0, winw, cury*fonh);
	    GrSetGCForeground(gc1,gi.foreground);
	}
 	if (curx > 0) 
	{
	    GrSetGCForeground(gc1,gi.background);
	    GrFillRect(w1,gc1, 0, cury*fonh, curx*fonw, fonh);
	    GrSetGCForeground(gc1,gi.foreground);
	}
	break;

    case 'e':/* enable cursor */
	curon = 1;
	break;

    case 'f':/* disable cursor */
	curon = 0;
	break;

    case 'j':/* save cursor position */
	savx = curx;
	savy = cury;
	break;

    case 'k':/* restore cursor position */
	curx = savx;
	cury = savy;
	break;

    case 'l':/* erase entire line */
	GrSetGCForeground(gc1,gi.background);
	GrRect(w1,gc1, 0, cury*fonh, winw, fonh);
	GrSetGCForeground(gc1,gi.foreground);
	curx = 0;
	break;

    case 'o':/* erase beginning of line */
	if (curx > 0)
	{
	    GrSetGCForeground(gc1,gi.background);
	    GrRect(w1,gc1,0, cury*fonh, curx*fonw, fonh);
	    GrSetGCForeground(gc1,gi.foreground);
	}
	break;

    case 'p':/* enter reverse video mode */
    {
	if(!ReverseMode)
	{
	    GrSetGCForeground(gc1,gi.background);
	    GrSetGCBackground(gc1,gi.foreground);
 	    ReverseMode=1; 
	}
    }
	break;

    case 'q':/* exit reverse video mode */
    {
	if(ReverseMode)
	{
	    GrSetGCForeground(gc1,gi.foreground);
	    GrSetGCBackground(gc1,gi.background);
 	    ReverseMode=0;
	}
    }
	break;

    case 'v':/* enable wrap at end of line */
	wrap = 1;
	break;

    case 'w':/* disable wrap at end of line */
	wrap = 0;
	break;

/* and these are the extentions not in VT52 */

    case 'G': /* clear all attributes */
	break;

    case 'g': /* enter bold mode */
/*	GrSetGCFont(gc1, boldFont); */
	break;

    case 'h': /* exit bold mode */
/*	GrSetGCFont(gc1, regFont); */
	break;

    case 'i': /* enter underline mode */
	break;

	/* j, k and l are already used */

    case 'm': /* exit underline mode */
	break;

/* these ones aren't yet on the termcap entries */

    case 'n': /* enter italic mode */
	break;
	/* o, p and q are already used */

    case 'r': /* exit italic mode */
	break;

    case 's': /* enter light mode */
	break;

    case 't': /* exit ligth mode */
	break;

    default: /* unknown escape sequence */
	break;
    }
}

/*
 * un-escaped character print routine
 */

void esc0 (unsigned char c)
{
    switch (c) 
    {
    case 0:
	/*
	 * printing \000 on a terminal means "do nothing".
	 * But since we use \000 as string terminator none
	 * of the characters that follow were printed.
	 *
	 * perl -e 'printf("a%ca", 0);'
	 *
	 * said 'a' in a wterm, but should say 'aa'. This
	 * bug screwed up most ncurses programs.
	 *
	 * kay.
	 */
	break;
 
    case 7: /* bell */
	if (visualbell) 
	{
/* 	    w_setmode(win, M_INVERS); */
/* 	    w_pbox(win, 0, 0, winw, winh); */
/* 	    w_test(win, 0, 0); */
/* 	    w_pbox(win, 0, 0, winw, winh); */
	    ;
	} 
	else 
	{
	    ;
 	    GrBell();
	}
	break;

    case 8: /* backspace */
	lineRedraw();
	if (--curx < 0) 
	{
	    curx = 0;
	}
	break;

    case 9: /* tab */
    {
	int borg,i;

	borg=(((curx >> 3) + 1) << 3);
	if(borg >= col)
	{
	    borg=col-1;
	}
	borg=borg-curx;
	for(i=0; i < borg; ++i){sadd(' ');}
 	if ((curx = ((curx >> 3) + 1) << 3) >= col) 
 	{ 
 	    curx = col - 1; 
 	} 
    }
	break;

    case 10: /* line feed */
	sflush();
	if (++cury >= row) 
	{
	    vscroll(1);
	    cury = row-1;
	}
	break;

    case 13: /* carriage return */
	sflush();
	curx = 0;
	break;

    case 27: /* escape */
	sflush();
	escstate = 1;
	break;

    case 127: /* delete */
	break;

    default: /* any printable char */
	sadd(c);
	if (++curx >= col) 
	{
	    sflush();
	    if (!wrap) 
	    {
		curx = col-1;
	    } 
	    else 
	    {
		curx = 0;
		if (++cury >= row) 
		{
		    vscroll(1);
		}
	    }
	}
	break;
    }
}


void printc(unsigned char c)
{
    switch(escstate) 
    {
    case 0:
	esc0(c);
	break;

    case 1:
	sflush();
	esc1(c);
	break;

    case 2:
	sflush();
	esc2(c);
	break;

    case 3:
	sflush();
	esc3(c);
	break;

    case 4:
	sflush();
	esc4(c);
	break;

    case 5:
	sflush();
	esc5(c);
	break;

    default: 
	escstate = 0;
	break;
    }
}


void init()
{
    curx = savx = 0;
    cury = savy = 0;
    wrap = 1;
    curon = 1;
    curvis = 0;
    escstate = 0;
}


/*
 * general code...
 */

void
term(void)
{
	long 		in, l;
	GR_EVENT 	wevent;
	GR_EVENT_KEYSTROKE *kp;
	unsigned char 	buf[LARGEBUFFER];

	GrRegisterInput(pipeh);
	while (42) {
		if (havefocus)
			draw_cursor();
		GrGetNextEvent(&wevent);

		switch(wevent.type) {
		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
			break;

		case GR_EVENT_TYPE_KEY_DOWN:
			kp=(GR_EVENT_KEYSTROKE *)&wevent;
			/* toss all special keys*/
			if (kp->ch & MWKEY_NONASCII_MASK)
				break;
			*buf = kp->ch & 0xff;
			write(pipeh, buf,1);
			break;

		case GR_EVENT_TYPE_FOCUS_IN:
			havefocus = GR_TRUE;
			break;

		case GR_EVENT_TYPE_FOCUS_OUT:
			havefocus = GR_FALSE;
			hide_cursor();
			break;

		case GR_EVENT_TYPE_UPDATE:
			/*
			 * if we get temporarily unmapped (moved),
			 * set cursor state off.
			 */
			if (wevent.update.utype == GR_UPDATE_UNMAPTEMP)
				curvis = 0;
			break;

		case GR_EVENT_TYPE_FDINPUT:
			hide_cursor();
			while ((in = read(pipeh, buf, sizeof(buf))) > 0) {
				for (l=0; l<in; l++) {
					printc(buf[l]); 
					if (buf[l] == '\n')
						printc('\r');
				}
				sflush();
			}
	    		break;
		}
	}
}


void usage(char *s)
{
    if (s) 
	fprintf(stderr, "error: %s\n", s);
    printf("usage: nxterm [-b] [-d] [-f <font family>] [-s <font size>]\n");
    printf("       [-g <geometry>] [-v] [-c] [-h] [program {args}]\n");
    exit(0);
}


void *mysignal(int signum, void *handler)
{
  struct sigaction sa, so;

  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(signum, &sa, &so);

  return so.sa_handler;
}

/*
 * guess what... :)
 */

int main(int argc, char **argv)
{
    GR_BITMAP	bitmap1fg[7];	/* mouse cursor */
    GR_BITMAP	bitmap1bg[7];
    GR_WM_PROPERTIES props;

    short xp, yp, fsize;
    char *family, *shell = NULL, *cptr, *geometry = NULL;
    struct passwd *pw;
    char buf[80];
    short uid;
    char thesh[128];
#ifdef __FreeBSD__
    char *ptr;
#endif

#ifdef SIGTTOU
    /* just in case we're started in the background */
    signal(SIGTTOU, SIG_IGN);
#endif

    /* who am I? */
    if (!(pw = getpwuid((uid = getuid())))) 
    {
	fprintf(stderr, "error: wterm can't determine determine your login name\n");
	exit(-1);
    }

  
    if (GrOpen() < 0) 
    {
	fprintf(stderr, "cannot open graphics\n");
	exit(1);
    }

    GrGetScreenInfo(&si);
    /*
     * scan arguments...
     */

    console = 0;
    argv++;
    while (*argv && **argv=='-') 
	switch (*(*argv+1)) 
	{
	case 'b':
	    cblink = 1;
	    argv++;
	    break;

	case 'c':
	    console = 1;
	    argv++;
	    break;

	case 'd':
	    debug = 1;
	    argv++;
	    break;

	case 'f':
	    if (*++argv) {
		family = *argv++;
	    } else {
		usage("-f option requires an argument");
	    }
	    break;

	case 's':
	    if (*++argv) {
		fsize = atoi(*argv++);
	    } else {
		usage("-s option requires an argument");
	    }
	    break;

	case 'g':
	    if (*++argv) {
		geometry = *argv++;
	    } else {
		usage("-g option requires an argument");
	    }
	    break;

	case 'h':
	    /* this will never return */
	    usage("");

	case 'v':
	    visualbell = 1;
	    argv++;
	    break;

	default:
	    usage("unknown option");
	}

    /*
     * now *argv either points to a program to start or is zero
     */

    if (*argv) {
	shell = *argv;
    }
    if (!shell) {
	shell = getenv("SHELL=");
    }
    if (!shell) {
	shell = pw->pw_shell;
    }
    if (!shell) {
	shell = "/bin/sh";
    }

    if (!*argv) {
	/*
	 * the '-' makes the shell think it is a login shell,
	 * we leave argv[0] alone if it isn`t a shell (ie.
	 * the user specified the program to run as an argument
	 * to wterm.
	 */
	cptr = strrchr(shell, '/');
	sprintf (thesh, "-%s", cptr ? cptr + 1 : shell);
	*--argv = thesh;
    }

    col = 80;
    row = 25;
    xp = 0;
    yp = 0;
    if (geometry) 
    {
	if (col < 1) 
	{
	    col = 80;
	}
	if (row < 1) 
	{
	    row = 25;
	}
	if (col > 0x7f)
	    colmask = 0xffff;
	if (row > 0x7f)
	    rowmask = 0xffff;
    }
    
    regFont=GrCreateFont(GR_FONT_SYSTEM_FIXED, 0, NULL);
    /*regFont=GrCreateFont(GR_FONT_OEM_FIXED, 0, NULL);*/
    /*boldFont=GrCreateFont(GR_FONT_SYSTEM_FIXED, 0, NULL);*/
    GrGetFontInfo(regFont, &fi);

    winw=col*fi.maxwidth;
    winh=row*fi.height;
    w1 = GrNewWindow(GR_ROOT_WINDOW_ID, 10,10,winw, winh,0,BLACK,LTBLUE);
    props.flags = GR_WM_FLAGS_TITLE;
    props.title = TITLE;
    GrSetWMProperties(w1, &props);

    GrSelectEvents(w1, GR_EVENT_MASK_BUTTON_DOWN |
		   GR_EVENT_MASK_KEY_DOWN | 
		   GR_EVENT_MASK_FOCUS_IN | GR_EVENT_MASK_FOCUS_OUT |
		   GR_EVENT_MASK_UPDATE | GR_EVENT_MASK_CLOSE_REQ);

    GrMapWindow(w1);

    gc1 = GrNewGC();
    GrSetGCFont(gc1, regFont);

#define	_	((unsigned) 0)		/* off bits */
#define	X	((unsigned) 1)		/* on bits */
#define	MASK(a,b,c,d,e,f,g) \
	(((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) \
	+ e) * 2) + f) * 2) + g) << 9)
	bitmap1fg[0] = MASK(_,_,X,_,X,_,_);
	bitmap1fg[1] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[2] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[3] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[4] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[5] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[6] = MASK(_,_,X,_,X,_,_);

	bitmap1bg[0] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[1] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[2] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[3] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[4] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[5] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[6] = MASK(_,X,X,X,X,X,_);
    GrSetCursor(w1, 7, 7, 3, 3, GREEN, BLACK, bitmap1fg, bitmap1bg);
    GrSetGCForeground(gc1, GREEN);
    GrSetGCBackground(gc1, BLACK);
    GrGetWindowInfo(w1,&wi);
    GrGetGCInfo(gc1,&gi);

    sprintf(buf, "wterm: %s", shell);

    /*
     * what kind of terminal do we want to emulate?
     */
#ifdef __FreeBSD__
    putenv ("TERM=wterm");
#else
    putenv ("TERM=vt52");
#endif

    /*
     * this one should enable us to get rid of an /etc/termcap entry for
     * both curses and ncurses, hopefully...
     */

    if (termcap_string) 
    {
	sprintf (termcap_string + strlen (termcap_string), "li#%d:co#%d:",
		 row, col);
	putenv (termcap_string);
    }
    /* in case program absolutely needs terminfo entry, these 'should'
     * transmit the screen size of correctly (at least xterm sets these
     * and everything seems to work correctly...). Unlike putenv(),
     * setenv() allocates also the given string not just a pointer.
     */
    sprintf (buf, "%d", col);
    setenv ("COLUMNS", buf, 1);
    sprintf (buf, "%d", row);
    setenv ("LINES", buf, 1);

    init();

    /*
     * create a pty
     */
#ifdef __FreeBSD__
    winsz.ws_col = col;
    winsz.ws_row = row;
    if ((pid = forkpty(&pipeh, pty, NULL, &winsz)) < 0)  
    {
	fprintf(stderr,"wterm: can't create pty\r\n");
	perror("wterm");
	sleep(2);
	GrKillWindow(w1);
	exit(-1);
    }

    if ((ptr = rindex(pty, '/'))) 
    {
	strcpy(pty, ptr + 1);
    }
  
    if (!pid) 
    {
	int i;
	for (i = getdtablesize(); --i >= 3; )
	    close (i);
	/*
	 * SIG_IGN are not reset on exec()
	 */
	for (i = NSIG; --i >= 0; )
	    signal (i, SIG_DFL);
 
	/* caution: start shell with correct user id! */
	seteuid(getuid());
	setegid(getgid());

	/* this shall not return */
	execvp(shell, argv);

	/* oops? */
	fprintf(stderr,"wterm: can't start shell\r\n");
	sleep(3);
	GrKillWindow(w1);
	_exit(-1);
    }
#else
    pipeh = term_init();
#endif

/*    _write_utmp(pty, pw->pw_name, "", time(0)); */

#if 0
    /* catch some signals */
    mysignal(SIGTERM, sigquit);
    mysignal(SIGHUP, sigquit);
    mysignal(SIGINT, SIG_IGN);
    mysignal(SIGQUIT, sigquit);
    mysignal(SIGPIPE, sigpipe);
    mysignal(SIGCHLD, sigchld);
#endif

    /* prepare to catch console output */
    if (console) 
    {
	/* for any OS chr$(7) might cause endless loops if 
	 * catched from console 
	 */
	visualbell = 1;
	console = 0;       /* data will come to normal pipe handle */
	ioctl(pipeh, TIOCCONS, 0);
    }

    term();
    return 0;
}

#if ELKS
char * nargv[2] = {"/bin/sash", NULL};
#else
#if DOS_DJGPP
char * nargv[2] = {"bash", NULL};
#else
char * nargv[2] = {"/bin/sh", NULL};
#endif
#endif

void sigchild(int signo)
{
	GrClose();
	exit(0);
}

int term_init()
{
	int tfd;
	int n = 0;
	pid_t pid;
	char pty_name[12];

again:
	sprintf(pty_name, "/dev/ptyp%d", n);
	if ((tfd = open(pty_name, O_RDWR | O_NONBLOCK)) < 0) {
		if ((errno == EBUSY || errno == EIO) && n < 10) {
			n++;
			goto again;
		}
		fprintf(stderr, "Can't create pty %s\n", pty_name);
		return -1;
	}
	signal(SIGCHLD, sigchild);
	signal(SIGINT, sigchild);
	if ((pid = fork()) == -1) {
		fprintf(stderr, "No processes\n");
		return -1;
	}
	if (!pid) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(tfd);
		
		setsid();
		pty_name[5] = 't';
		if ((tfd = open(pty_name, O_RDWR)) < 0) {
			fprintf(stderr, "Child: Can't open pty %s\n", pty_name);
			exit(1);
		}
		close(STDERR_FILENO);
		dup2(tfd, STDIN_FILENO);
		dup2(tfd, STDOUT_FILENO);
		dup2(tfd, STDERR_FILENO);
		execv(nargv[0], nargv);
		exit(1);
	}
	return tfd;
}

#if 0
void _write_utmp(char *line, char *user, char *host, int time)
{
    int fh, offset, isEmpty, isLine;
    struct utmp ut;

    if ((fh = open("/etc/utmp", O_RDWR)) < 0) {
	return;
    }

    /* first of all try to find an entry with the same line */

    offset = 0;
    isEmpty = -1;
    isLine = -1;

    while ((isLine < 0) && (read(fh, &ut, sizeof(ut)) == sizeof(ut))) {
	if (!ut.ut_line[0]) 
	{
	    if (isEmpty < 0) 
	    {
		isEmpty = offset;
	    }
	} 
	else 
	{
	    if (!strncmp(ut.ut_line, line, sizeof(ut.ut_line))) {
		isLine = offset;
	    }
	}
	offset += sizeof(ut);
    }

    if (isLine != -1) {
	/* we've found a match */
	lseek(fh, isLine, SEEK_SET);
    } else if (isEmpty != -1) {
	/* no match found, but at least an empty entry */
	lseek(fh, isLine, SEEK_SET);
    } else {
	/* not even an empty entry found, assume we can append to the file */
    }

    if (time) 
    {
	strncpy(ut.ut_line, line, sizeof(ut.ut_line));
	strncpy(ut.ut_name, user, sizeof(ut.ut_name));
	strncpy(ut.ut_host, host, sizeof(ut.ut_host));
	ut.ut_time = time;
    } 
    else 
    {
	memset(&ut, 0, sizeof(ut));
    }
    write(fh, &ut, sizeof(ut));
    close(fh);
}
#endif
