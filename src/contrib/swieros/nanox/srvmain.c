/*
 * Copyright (c) 1999, 2000, 2001, 2003, 2004, 2010, 2019 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 1991 David I. Bell
 *
 * Main module of graphics server.
 */

/*
 * External definitions defined here.
 */
GR_WINDOW_ID	cachewindowid;		/* cached window id */
GR_WINDOW_ID    cachepixmapid;         /* cached pixmap id */
GR_GC_ID	cachegcid;		/* cached graphics context id */
GR_WINDOW	*cachewp;		/* cached window pointer */
GR_GC		*cachegcp;		/* cached graphics context */
GR_PIXMAP       *cachepp;               /* cached pixmap */
GR_PIXMAP       *listpp;                /* List of all pixmaps */
GR_WINDOW	*listwp;		/* list of all windows */
GR_WINDOW	*rootwp;		/* root window pointer */
GR_GC		*listgcp;		/* list of all gc */
GR_REGION	*listregionp;		/* list of all regions */
GR_FONT		*listfontp;		/* list of all fonts */
GR_CURSOR	*listcursorp;		/* list of all cursors */
GR_CURSOR	*stdcursor;		/* root window cursor */
GR_GC		*curgcp;		/* currently enabled gc */
GR_WINDOW	*clipwp;		/* window clipping is set for */
GR_WINDOW	*focuswp;		/* focus window for keyboard */
GR_WINDOW	*mousewp;		/* window mouse is currently in */
GR_WINDOW	*grabbuttonwp;		/* window grabbed by button */
GR_CURSOR	*curcursor;		/* currently enabled cursor */
GR_COORD	cursorx;		/* current x position of cursor */
GR_COORD	cursory;		/* current y position of cursor */
GR_BUTTON	curbuttons;		/* current state of buttons */
GR_CLIENT	*curclient;		/* client currently executing for */
GR_EVENT_LIST	*eventfree;		/* list of free events */
GR_BOOL		focusfixed;		/* TRUE if focus is fixed on a window */
PMWFONT		stdfont;		/* default font*/
char		*progname;		/* Name of this program.. */

int		current_fd;		/* the fd of the client talking to */
int		connectcount = 0;	/* number of connections to server */
GR_CLIENT	*root_client;		/* root entry of the client table */
GR_CLIENT	*current_client;	/* the client we are currently talking*/
char		*current_shm_cmds;
int		current_shm_cmds_size;
static int	keyb_fd;		/* the keyboard file descriptor */
static int	mouse_fd;		/* the mouse file descriptor */
char		*curfunc;		/* the name of the current server func*/
GR_BOOL		screensaver_active;	/* time before screensaver activates */
GR_SELECTIONOWNER selection_owner;	/* the selection owner and typelist */
int		autoportrait = FALSE;	/* auto portrait mode switching*/
MWCOORD		nxres;			/* requested server x resolution*/
MWCOORD		nyres;			/* requested server y resolution*/
GR_GRABBED_KEY  *list_grabbed_keys = NULL;     /* list of all grabbed keys */

static int	persistent_mode = FALSE;
static int	portraitmode = MWPORTRAIT_NONE;

SERVER_LOCK_DECLARE /* Mutex for all public functions (only if NONETWORK and THREADSAFE) */

void
GsAcceptClientFd(int i)
{
	GR_CLIENT *client, *cl;

	if(!(client = malloc(sizeof(GR_CLIENT)))) {
		return;
	}

	client->id = i;
	client->eventhead = NULL;
	client->eventtail = NULL;
	/*client->errorevent.type = GR_EVENT_TYPE_NONE;*/
	client->next = NULL;
	client->prev = NULL;
	client->waiting_for_event = FALSE;
	client->shm_cmds = 0;

	if(connectcount++ == 0)
		root_client = client;
	else {
		cl = root_client;
			while(cl->next)
				cl = cl->next;
		client->prev = cl;
		cl->next = client;
	}
}

/*
 * Open a connection from a new client to the server.
 * Returns -1 on failure.
 */
int
GrOpen(void)
{
	GdPlatformInit();			/* platform-specific initialization*/

	SERVER_LOCK();

	/* Client calls this routine once.  We init everything here*/
	if (connectcount <= 0) {
		if(GsInitialize() < 0) {
			SERVER_UNLOCK();
			return -1;
		}
		GsAcceptClientFd(999);
		curclient = root_client;
	}
	SERVER_UNLOCK();

	wm_init();	/* init built-in window manager*/
    return 1;
}

/*
 * Close the current connection to the server.
 */
void
GrClose(void)
{
	SERVER_LOCK();
	GsClose(current_fd);
	SERVER_UNLOCK();
}

/*
 * Drop a specific server connection.
 */
void
GsClose(int fd)
{
	GsDropClient(fd);
	if(!persistent_mode && connectcount == 0)
		GsTerminate();
}

/* client/server GsDropClient is in srvnet.c*/
void
GsDropClient(int fd)
{
	--connectcount;
}

/*
 * Initialize the graphics and mouse devices at startup.
 * Returns nonzero with a message printed if the initialization failed.
 */
int
GsInitialize(void)
{
	GR_WINDOW	*wp;		/* root window */
	PSD		psd;
	GR_CURSOR_ID	cid;
	static const MWIMAGEBITS cursorbits[16] = {
	      0xe000, 0x9800, 0x8600, 0x4180,
	      0x4060, 0x2018, 0x2004, 0x107c,
	      0x1020, 0x0910, 0x0988, 0x0544,
	      0x0522, 0x0211, 0x000a, 0x0004
	};
	static const MWIMAGEBITS cursormask[16] = {
	      0xe000, 0xf800, 0xfe00, 0x7f80,
	      0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
	      0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
	      0x073e, 0x021f, 0x000e, 0x0004
	};

	/* If needed, initialize the server mutex. */
	SERVER_LOCK_INIT();

	//setbuf(stdout, NULL);
	//setbuf(stderr, NULL);

	wp = (GR_WINDOW *) malloc(sizeof(GR_WINDOW));
	if (wp == NULL) {
		EPRINTF("Cannot allocate root window\n");
		return -1;
	}

	screensaver_active = GR_FALSE;

	selection_owner.wid = 0;
	selection_owner.typelist = NULL;

	if ((keyb_fd = GdOpenKeyboard()) == -1) {
		EPRINTF("Cannot initialise keyboard\n");
		/*GsCloseSocket();*/
		free(wp);
		return -1;
	}

	if ((psd = GdOpenScreen()) == NULL) {
		EPRINTF("Cannot initialise screen\n");
		/*GsCloseSocket();*/
		GdCloseKeyboard();
		free(wp);
		return -1;
	}
	GdSetPortraitMode(psd, portraitmode);

	if ((mouse_fd = GdOpenMouse()) == -1) {
		EPRINTF("Cannot initialise mouse\n");
		/*GsCloseSocket();*/
		GdCloseScreen(psd);
		GdCloseKeyboard();
		free(wp);
		return -1;
	}

	/*
	 * Create std font.
	 */
	stdfont = GdCreateFont(psd, MWFONT_SYSTEM_VAR, 0, 0, NULL);

	/*
	 * Initialize the root window.
	 */
	wp->psd = psd;
	wp->id = GR_ROOT_WINDOW_ID;
	wp->parent = NULL;		/* changed: was = NULL*/
	wp->owner = NULL;
	wp->children = NULL;
	wp->siblings = NULL;
	wp->next = NULL;
	wp->x = 0;
	wp->y = 0;
	wp->width = psd->xvirtres;
	wp->height = psd->yvirtres;
	wp->bordersize = 0;
	wp->background = BLACK;
	wp->bordercolor = wp->background;
	wp->nopropmask = 0;
	wp->bgpixmap = NULL;
	wp->bgpixmapflags = GR_BACKGROUND_TILE;
	wp->eventclients = NULL;
	wp->cursorid = 0;
	wp->mapped = GR_TRUE;
	wp->realized = GR_TRUE;
	wp->output = GR_TRUE;
	wp->props = 0;
	wp->title = NULL;
	wp->clipregion = NULL;

    listpp = NULL;
	listwp = wp;
	rootwp = wp;
	focuswp = wp;
	mousewp = wp;
	focusfixed = GR_FALSE;

	/*
	 * Initialize and position the default cursor.
	 */
	curcursor = NULL;
	cursorx = -1;
	cursory = -1;
	GdShowCursor(psd);
	GrMoveCursor(psd->xvirtres / 2, psd->yvirtres / 2);
	cid = GrNewCursor(16, 16, 0, 0, WHITE, BLACK, (MWIMAGEBITS *)cursorbits,
				(MWIMAGEBITS *)cursormask);
	GrSetWindowCursor(GR_ROOT_WINDOW_ID, cid);
	stdcursor = GsFindCursor(cid);

	psd->FillRect(psd, 0, 0, psd->xvirtres-1, psd->yvirtres-1,
		GdFindColor(psd, wp->background));

	/*
	 * Tell the mouse driver some things.
	 */
	curbuttons = 0;
	GdRestrictMouse(0, 0, psd->xvirtres - 1, psd->yvirtres - 1);
	GdMoveMouse(psd->xvirtres / 2, psd->yvirtres / 2);

	/* Force root window screen paint*/
	GsRedrawScreen();

	/* 
	 * Force the cursor to appear on the screen at startup.
	 * (not required with above GsRedrawScreen)
	GdHideCursor(psd);
	GdShowCursor(psd);
	 */

	/*
	 * All done.
	 */
	connectcount = 0;
	return 0;
}

/*
 * Here to close down the server.
 */
void
GsTerminate(void)
{
	GdCloseScreen(rootwp->psd);
	GdCloseMouse();
	GdCloseKeyboard();
	exit(0);
}
