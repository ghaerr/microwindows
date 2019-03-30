/*
 * Copyright (c) 2010, 2019 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Framebuffer Emulator screen driver
 * Set SCREEN=FBE in config.
 * Also useful as template when developing new screen drivers
 */

/* path to framebuffer emulator's framebuffer memory file for mmap()*/
#define PATH_FRAMEBUFFER "/tmp/fb0"

static int fb = -1;						/* Framebuffer file handle*/

static PSD  fbe_open(PSD psd);
static void fbe_close(PSD psd);

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	fbe_open,
	fbe_close,
	0,
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait
};

/* init framebuffer*/
static PSD
fbe_open(PSD psd)
{
	char *env;

	int flags = PSF_SCREEN;		/* init psd, don't allocate framebuffer*/

	if (!gen_initpsd(psd, MWPIXEL_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, flags))
		return NULL;

	/* set to copy aggregate screen update region in Update()*/
	psd->flags |= PSF_DELAYUPDATE;

	/* set if screen driver subsystem requires polling and select()*/
	psd->flags |= PSF_CANTBLOCK;

	/* open framebuffer file for mmap*/
	env = PATH_FRAMEBUFFER;
	fb = open(env, O_RDWR);
	if (fb >= 0)
	{
		/* mmap framebuffer into this address space*/
		psd->size = (psd->size + getpagesize() - 1) / getpagesize() * getpagesize();
		psd->addr = mmap(NULL, psd->size, PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);
		if (psd->addr == NULL || psd->addr == (unsigned char *)-1)
		{
			EPRINTF("Error mmaping shared framebuffer %s: %m\n", env);
			close(fb);
			return NULL;
		}
	}
	else {
		EPRINTF("Error opening %s\n", env);
		return NULL;
	}

	return psd;	/* success*/
}

/* close framebuffer*/
static void
fbe_close(PSD psd)
{
	if (fb >= 0)
		close(fb);
	fb = -1;
}
