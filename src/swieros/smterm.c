/* single-file nxterm*/
#define SWIEROS				1
#define NONETWORK			1
#define UNIX				1	/* only for GrRegisterInput for now*/
#define HAVE_SIGNAL			0
#define HAVE_FLOAT			0
#define POLYREGIONS			0
#define DYNAMICREGIONS		0
#define MW_FEATURE_SHAPES 	0
#define MW_FEATURE_IMAGES	0
#define MW_FEATURE_TIMERS	0

#define _DEFAULT_SOURCE			/* for strcasecmp in string.h and alloca somewhere else*/
//#define _XOPEN_SOURCE 600		/* required for linux, not OSX nxterm.c compile*/

/* engine*/
#define NOSTDPAL1
#define NOSTDPAL2
#define NOSTDPAL4
#define NOSTDPAL8
#include "engine/devopen.c"
#include "engine/devdraw.c"
#include "engine/devfont.c"
#include "engine/devclip1.c"
#include "engine/devmouse.c"
#include "engine/devkbd.c"
#include "engine/devblit.c"

#include "drivers/genfont.c"
#include "fonts/X6x13.c"
#include "fonts/winFreeSansSerif11x13.c"

/* drivers*/
#include "drivers/genmem.c"
#include "drivers/fb.c"
#include "drivers/osdep.c"
#include "drivers/fblin32_swieros.c"

/* screen driver*/
#define SCREEN_WIDTH	1024
#define SCREEN_HEIGHT	768
//#include "drivers/scr_x11.c"
//#include "drivers/mou_x11.c"
//#include "drivers/kbd_x11.c"
#include "drivers/scr_sdl2.c"
#include "drivers/mou_sdl2.c"
#include "drivers/kbd_sdl2.c"

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

/* problems with #define A, come after nanox/srvfunc.c*/
#include "engine/convblit_frameb.c"
#include "engine/convblit_mask.c"

/* app*/
#define MACOSX				1
#define LINUX				1
#include "demos/nanox/nxterm.c"
