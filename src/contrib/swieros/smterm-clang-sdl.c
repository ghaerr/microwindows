/* single-file nxterm*/
#define SCREEN_WIDTH		1024
#define SCREEN_HEIGHT		768
#define MWPIXEL_FORMAT		MWPF_TRUECOLORARGB

/* small footprint settings in original source*/
#define SWIEROS				1
#define NONETWORK			1
#define HAVE_SIGNAL			0
#define HAVE_FLOAT			0
#define USE_ALLOCA			0
#define POLYREGIONS			0
#define DYNAMICREGIONS		0
#define MW_CPU_BIG_ENDIAN	0
#define MW_FEATURE_SHAPES 	0
#define MW_FEATURE_INTL 	0
#define MW_FEATURE_PALETTE	0
#define MW_FEATURE_IMAGES	0
#define MW_FEATURE_TIMERS	0

/* uint32_t, int32_t*/
#include <stdint.h> 		/* for uint32_t, int32_t*/
//typedef unsigned long	uint32_t;	/* 32 bit type in 16 or 32 bit environment*/
//typedef long			int32_t;

/* unix settings and includes*/
#define UNIX				1	/* for GrRegisterInput and nxterm pty*/
#define HAVE_SELECT			1
#define _DEFAULT_SOURCE			/* for strcasecmp in string.h and alloca somewhere else*/
//#define _XOPEN_SOURCE 600		/* required for linux, not OSX nxterm.c compile*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>				// for framebuffer open
#include <sys/mman.h>			// for framebuffer mmap

/* include*/
#include "mwtypes.h"			// for public export
#include "nano-X.h"				// for public export
#include "device.h"
#include "nanowm.h"
#include "serv.h"
#include "nxdraw.h"
#include "genmem.h"
#include "genfont.h"
#include "osdep.h"
#include "fb.h"
#include "convblit.h"

/* engine*/
#include "engine/devopen.c"
#include "engine/devdraw.c"
#include "engine/devfont.c"
#include "engine/devclip1.c"
#include "engine/devmouse.c"
#include "engine/devkbd.c"
#include "engine/devblit.c"
#include "engine/convblit_frameb.c"
#include "engine/convblit_mask.c"

/* fonts*/
#include "fonts/X6x13.c"
#include "fonts/winFreeSansSerif11x13.c"

/* drivers*/
#include "drivers/genfont.c"
#include "drivers/genmem.c"
#include "drivers/fb.c"
#include "drivers/osdep.c"
#include "drivers/fblin32_swieros.c"

/* nanox*/
#include "nanox/srvmain.c"
#include "nanox/srvfunc.c"
#include "nanox/srvutil.c"
#include "nanox/srvevent.c"
#include "nanox/srvclip1.c"
#include "nanox/srvnonet.c"
#include "nanox/wmaction.c"
#include "nanox/wmclients.c"
#include "nanox/wmevents.c"
#include "nanox/wmutil.c"
#include "nanox/nxutil.c"
#include "nanox/nxdraw.c"

/* GsSelect*/
#include "nanox/srvunix.c"				// for unix
//#include "nanox/srvpoll.c"			// for swieros

/* framebuffer screen driver*/
//#include "drivers/scr_fbe.c"			// for swieros or unix with x11 framebuffer emulator
//#include "drivers/mou_null.c"
//#include "drivers/kbd_null.c"

/* X11 screen driver*/
#define DRIVER_OKFILEDESC(fd)	fd
#define PIXEL888RED8(pixelval)          (((pixelval) >> 16) & 0xff)
#define PIXEL888GREEN8(pixelval)        (((pixelval) >> 8) & 0xff)
#define PIXEL888BLUE8(pixelval)         ((pixelval) & 0xff)
#define PIXEL888RED32(pixelval)          ((((uint32_t)(pixelval)) <<  8) & 0xff000000UL)
#define PIXEL888GREEN32(pixelval)        ((((uint32_t)(pixelval)) << 16) & 0xff000000UL)
#define PIXEL888BLUE32(pixelval)         ((((uint32_t)(pixelval)) << 24) & 0xff000000UL)
//#include "../drivers/scr_x11.c"
//#include "../drivers/mou_x11.c"
//#include "../drivers/kbd_x11.c"

/* SDL screen driver*/
#include "../drivers/scr_sdl2.c"
#include "../drivers/mou_sdl2.c"
#include "../drivers/kbd_sdl2.c"

/* swieros C lib extras*/
//#include "swutil.c"					// for swieros

/* app*/
#include "demos/nxterm.c"
