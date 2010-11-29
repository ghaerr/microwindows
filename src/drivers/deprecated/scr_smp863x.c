/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Screen Driver for Linux kernel framebuffers
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 *
 * Modified for eCos by
 *   Gary Thomas <gthomas@redhat.com>
 *   Richard Panton <richard.panton@3glab.org>
 * 
 * Note: modify select_fb_driver() to add new framebuffer subdrivers
 */

#define _GNU_SOURCE 1

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

#include "mwtypes.h"

//#define DRIVER_DEBUG

/* #define GFX_KEYCOLOR_TEST  */

/* #define NO_SWITCH_BUFFER_TEST */          // Switch buffer only one time

#define SWITCH_BUFFER_TEST           // Test for changing WaitForPicture and Display sequence

/* #define DRAWAREA_TEST */

extern SUBDRIVER fblinear32alpha;
static PSD  em86xx_open(PSD psd);
static void em86xx_close(PSD psd);
static void em86xx_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void em86xx_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
static void em86xx_drawvertline(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
static void em86xx_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c);
static void em86xx_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op);
#ifdef DRAWAREA_TEST
static void em86xx_drawarea(PSD psd, driver_gc_t * gc, int op);
#endif
static MWBOOL em86xx_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int data_format,int linelen, int size,void *addr);
static void em86xx_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw, MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw, MWCOORD srch, long op);

#ifdef NO_SWITCH_BUFFER_TEST
static int switch_first = 0;
#endif

// This for 8634 with MMU only
// For MMU system, the mapped address has to mapped to 16bit aligned address, thus, the physical address should be aligned first
// Without this, GFX reads from true physical address, however, the true data may start from mapped address
// Thus, due to the aligned offset inconsistency, the picture may flick
#define RUAMAP_ALIGN(x) (((RMuint32)x + 16)&~0xF)

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	em86xx_open,
	em86xx_close,
	em86xx_getscreeninfo,
	NULL,
	NULL,						/* DrawPixel subdriver*/
	NULL,						/* ReadPixel subdriver*/
	em86xx_drawhorzline,		/* DrawHorzLine subdriver*/
	em86xx_drawvertline,		/* DrawVertLine subdriver*/
	em86xx_fillrect, 			/* FillRect subdriver*/
	gen_fonts,
	em86xx_blit,				/* Blit subdriver*/
	NULL,						/* PreSelect*/
	NULL,						/* SetIOPermissions*/
	gen_allocatememgc,
	em86xx_mapmemgc,
	gen_freememgc,
	NULL,						/* SetPortrait */
	0,							/* screen portrait mode */
	NULL,						/* orgsubdriver */
	NULL,						/* StretchBlitEx subdriver */
};

#define ALLOW_OS_CODE 1

#include "common.h"

#include "rmrtk/include/rmrtk.h"

#define TIMEOUT_US 1000000

enum {
	MR_ERROR = -1,
	MR_OK = 0,
};

/* 
 *  use the SEND_GFX_COMMAND if you don't want a continuos polling
 *  otherwise you can just write
 *  while( RUASetProperty(pRUA, moduleID, propertyID, pValue, ValueSize, 0) == RM_PENDING);
 *
 *  uncomment the following line if you want the application to wait for the completion of commands
 *  before going on (otherwise the command is queued, and the application only waits if the command
 *  queue is full)
 * 
 */

#ifdef WAIT_FOR_COMMANDS
#define SEND_GFX_COMMAND(pRUA, moduleID, propertyID, pValue, ValueSize)	\
{	\
	RMstatus err;	\
	RMuint32 n = 5;		\
	\
	struct RUAEvent evt;	\
	evt.ModuleID = moduleID;	\
	evt.Mask = RUAEVENT_COMMANDCOMPLETION;	\
	do{		\
		err = RUASetProperty(pRUA, moduleID, propertyID, pValue, ValueSize, 0);		\
		if(err == RM_PENDING){		\
			while (RUAWaitForMultipleEvents(pRUA, &evt, 1, TIMEOUT_US, NULL) != RM_OK)	\
				printf("%s, %d, Waiting for a command to finish\n", __FUNCTION__, __LINE__);	\
		}	\
		n--;	\
	}while((n>0) && (err == RM_PENDING));	\
	if (err != RM_OK) {		\
		printf("%s, %d, Can't send command to command fifo\n", __FUNCTION__, __LINE__);	\
		return err;		\
	}	\
	while (RUAWaitForMultipleEvents(pRUA, &evt, 1, TIMEOUT_US, NULL) != RM_OK)	\
		printf("%s, %d, Waiting for a command to finish\n", __FUNCTION__, __LINE__);	\
}
#else													
#define SEND_GFX_COMMAND(pRUA, moduleID, propertyID, pValue, ValueSize)		\
{	\
	RMstatus err;	\
	RMuint32 n;		\
	n = 5;	\
	do{		\
		err = RUASetProperty(pRUA, moduleID, propertyID, pValue, ValueSize, 0);		\
		if ((err == RM_PENDING)) {		\
 			struct RUAEvent evt;	\
			evt.ModuleID = moduleID;	\
			evt.Mask = RUAEVENT_COMMANDCOMPLETION;	\
			while (RUAWaitForMultipleEvents(pRUA, &evt, 1, TIMEOUT_US, NULL) != RM_OK)	\
				printf("%s, %d, Waiting for a command to finish\n", __FUNCTION__, __LINE__);	\
		}	\
		n--;	\
	}while((n>0) && (err == RM_PENDING));	\
	if (err != RM_OK) {		\
		printf("\033[41m %s, %d, Can't send command to command fifo \033[0m\n", __FUNCTION__, __LINE__ );	\
		printf("\033[41m %s, %d, Can't send command to command fifo \033[0m\n", __FUNCTION__, __LINE__ );	\
		printf("\033[41m %s, %d, Can't send command to command fifo \033[0m\n", __FUNCTION__, __LINE__ );	\
		printf("return err;");	\
	}	\
}
#endif /* WAIT_FOR_COMMANDS */

static struct playback_cmdline play_opt;
static struct video_cmdline video_opt;
static struct DCC *pDCC = NULL;
struct RUA *pRUA = NULL;
static struct DCCVideoSource *pVideoSource;
static RMuint32 pic_luma_addr[2], pic_index = 0, surface_addr;
static RMuint32 gfx;
static struct GFXEngine_Open_type gfx_profile;
static struct dh_context dh_info = {0,};
static struct dcc_context dcc_info = {0,};
static struct GFXEngine_DisplayPicture_type display_pic;
static struct GFXEngine_Surface_type YZ_surface;
static struct GFXEngine_Surface_type NX_surface;
static struct GFXEngine_Surface_type X_surface;

#include <math.h>

#define GFX_SUBOP_SCALE				(0x00000001)  // Scale image to destination size

typedef struct {
	struct RUA *pRUA;
	struct DCC *pDCC;
	unsigned long int gfx;
	unsigned long int pic_addr[2];
	unsigned long int pic_index;
	struct dcc_context *dcc_info;
#ifdef __EM8622__
	struct dh_context *dh_info;
#endif
} GFXContext;

typedef struct {
		struct DCCOSDProfile bitmap_profile;
		RMuint32 buf_addr;
		RMuint32 buf_size;
} PICContext ;

static PICContext osd;
static GFXContext GFX;

void GFX_Fill_Rect(GFXContext *pGFX, PSD psd, MWCOORD x1,MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c);
void GFX_SingleColor_Blend(GFXContext *pGFX, PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c);
void GFX_Copy_Bitmap(GFXContext *pGFX, PSD dstpsd, int dx, int dy, int dw, int dh, PSD srcpsd, int sx, int sy, int sw, int sh, int flag, int dst_width, int src_width);
#ifdef GFX_KEYCOLOR_TEST
void GFX_Copy_IK_Bitmap(GFXContext *pGFX, PSD dstpsd, int dx, int dy, int dw, int dh, PSD srcpsd, int sx, int sy, int sw, int sh, int flag, int dst_width, int src_width);
#endif
void GFX_Blend_Bitmap(GFXContext *pGFX, PSD dstpsd, int dx, int dy, int dw, int dh, PSD srcpsd, int sx, int sy, int sw, int sh, int flag, int dst_width, int src_width);
void GFX_Blend_BitmapFading(GFXContext *pGFX, PSD dstpsd, int dx, int dy, int dw, int dh, PSD srcpsd, int sx, int sy, int sw, int sh, int flag, int dst_width, int src_width, int alpha);

int Sigma86_Init (void);
int GFX_Init (void);
void GFX_Done (void);
void Sigma86_Done (void);

/* init framebuffer*/
static PSD em86xx_open(PSD psd)
{
	PSUBDRIVER subdriver;

	// Initialize EM86xx
	if (Sigma86_Init() != MR_OK) {
		Sigma86_Done();
		return 0;
	}

	// Initialize GFX Engine
	if (GFX_Init() != MR_OK) { 
		Sigma86_Done();
		return 0;
	}

	psd->xres = psd->xvirtres = osd.bitmap_profile.Width;
	psd->yres = psd->yvirtres = osd.bitmap_profile.Height;

	psd->planes = 1;
	
	switch (osd.bitmap_profile.ColorMode) {
		case EMhwlibColorMode_TrueColor:
		case EMhwlibColorMode_TrueColorWithKey:
			switch (osd.bitmap_profile.ColorFormat) {
				case EMhwlibColorFormat_24BPP_565:
				case EMhwlibColorFormat_24BPP:
					psd->bpp = 24;
					break;
				case EMhwlibColorFormat_32BPP_4444:
				case EMhwlibColorFormat_32BPP:
					psd->bpp = 32;
					break;
				case EMhwlibColorFormat_16BPP_565:
				case EMhwlibColorFormat_16BPP_1555:
				case EMhwlibColorFormat_16BPP_4444:
					psd->bpp = 16;
					break;
			}
			break;
		case EMhwlibColorMode_LUT_1BPP:
			psd->bpp = 1;
			break;
		case EMhwlibColorMode_LUT_2BPP:	
			psd->bpp = 2;
			break;
		case EMhwlibColorMode_LUT_4BPP:
			psd->bpp = 4;
			break;
		case EMhwlibColorMode_LUT_8BPP:
			psd->bpp = 8;
			break;
		default:
			break;
	}

	psd->ncolors = (psd->bpp >= 24) ? (1 << 24) : (1 << psd->bpp);

	/* set linelen to byte length, possibly converted later*/
	psd->linelen = osd.bitmap_profile.Width * ((psd->bpp + 7) / 8);

	/* force subdriver init of size */
	psd->size = 0;

	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;

	/* set pixel format*/
	psd->pixtype = MWPF_TRUECOLOR8888;

	/* select a framebuffer subdriver based on planes and bpp*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver) {
		fprintf(stderr, "No driver for screen %d\n", psd->bpp);
		goto fail;
	}

	/*
	 * set and initialize subdriver into screen driver
	 * psd->size is calculated by subdriver init
	 */
	if(!set_subdriver(psd, subdriver, TRUE)) {
		fprintf(stderr, "Driver initialize failed %d\n", psd->bpp);
		goto fail;
	}
	// Replace with functions have been implemented in screen device driver
	psd->DrawHorzLine = em86xx_drawhorzline;
	psd->DrawVertLine = em86xx_drawvertline;
	psd->FillRect = em86xx_fillrect;
	psd->Blit = em86xx_blit; 
	//psd->StretchBlit = em86xx_stretchblit;
	psd->StretchBlitEx = NULL;

	/* mmap framebuffer into this address space*/
	if (osd.buf_addr == 0) {
		printf("Can't map framebuffer.\n");
		goto fail;
	}
	psd->portrait = MWPORTRAIT_NONE;

	psd->physical = psd->addr = (void *) osd.buf_addr;

#ifdef DRIVER_DEBUG
	printf("\nInfo about OSD driver.\n");
	printf("ColorMode = %d\n", osd.bitmap_profile.ColorMode);
	printf("ColorFormat = %d\n", osd.bitmap_profile.ColorFormat);
	printf("Bits Per Pixel = %d\n", psd->bpp);
	printf("The length of each line = %d, X resolution = %d, Y resolution = %d\n", psd->linelen, psd->xres, psd->yres);
	printf("PSD address = 0x%lx\n", osd.buf_addr);
#endif

	return psd;	/* success*/

fail:
	return NULL;
}

/* close framebuffer*/
static void em86xx_close(PSD psd)
{
	GFX_Done();
	Sigma86_Done();
}

static void em86xx_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->data_format = psd->data_format;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	psi->ydpcm = 42; // 320 / (3 * 2.54)
	psi->xdpcm = 38; //240 / (2.5 * 2.54)


	GFX.pRUA = pRUA;
	GFX.pDCC = pDCC;
	GFX.gfx = gfx;
	GFX.pic_addr[0] = pic_luma_addr[0];
	GFX.pic_addr[1] = pic_luma_addr[1];
	GFX.pic_index = pic_index;
	GFX.dcc_info = &dcc_info;
#ifdef __EM8622__
	GFX.dh_info = &dh_info;
#endif
	
	psi->gfxcontext = (void *) &GFX;
}

inline void flush_instr_cache (void) 
{
#if (EM86XX_CHIP<EM86XX_CHIPID_TANGO2) && defined WITH_IRQHANDLER_BOOTLOADER
	__asm("MCR     p15, 0, r0, c7, c5, 0");
#endif
}

inline void clean_data_cache (void)
{        
#if (EM86XX_CHIP<EM86XX_CHIPID_TANGO2) && defined WITH_IRQHANDLER_BOOTLOADER
	__asm("MCR     p15, 0, r0, c7, c10, 0");

	// drains write buffer
	__asm("LDR     r3, =0");          // reset vector 
	__asm("SWP     r2, r0, [r3]");    // r2 = [r3], [r3] = r0
	__asm("SWP     r0, r2, [r3]");    // r0 = [r3], [r3] = r2
#endif
}

static void em86xx_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	switch (gr_mode) {
		case MWROP_COPY:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (psd->addr != (void *)GFX.pic_addr[0] && psd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Fill_Rect(&GFX, psd, x1, y, x2, y, c);
			break;
		case MWROP_SRC_OVER:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (psd->addr != (void *)GFX.pic_addr[0] && psd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_SingleColor_Blend(&GFX, psd, x1, y, x2, y, c);
			break;
		default:
#ifdef DRIVER_DEBUG
			printf("In %s:%d: grMode = %d\n", __FUNCTION__, __LINE__, gr_mode);
#endif
			fblinear32alpha.DrawHorzLine(psd, x1, x2, y, c);
			break;
	}
}

static void em86xx_drawvertline(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	switch (gr_mode) {
		case MWROP_COPY:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (psd->addr != (void *)GFX.pic_addr[0] && psd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Fill_Rect(&GFX, psd, x1, y1, x1, y2, c);
			break;
		case MWROP_SRC_OVER:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (psd->addr != (void *)GFX.pic_addr[0] && psd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_SingleColor_Blend(&GFX, psd, x1, y1, x1, y2, c);
			break;
		default:
#ifdef DRIVER_DEBUG
			printf("In %s:%d: grMode = %d\n", __FUNCTION__, __LINE__, gr_mode);
#endif
			fblinear32alpha.DrawVertLine(psd, x1, y1, y2, c);
			break;
	}
}

static void em86xx_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	switch (gr_mode) {
		case MWROP_COPY:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (psd->addr != (void *)GFX.pic_addr[0] && psd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Fill_Rect(&GFX, psd, x1, y1, x2, y2, c);
			break;
		case MWROP_SRC_OVER:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (psd->addr != (void *)GFX.pic_addr[0] && psd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_SingleColor_Blend(&GFX, psd, x1, y1, x2, y2, c);
			break;
		default:
#ifdef DRIVER_DEBUG
			printf("In %s:%d: grMode = %d\n", __FUNCTION__, __LINE__, gr_mode);
#endif
			fblinear32alpha.FillRect(psd, x1, y1, x2, y2, c);
			break;
	}
}

static void em86xx_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
#ifdef SWITCH_BUFFER_TEST
	int tmp_index = 0;
#endif
	int switch_flag = 0;

#ifdef DRIVER_DEBUG
	printf("\033[44m In %s:%d: Map (%p) [%d, %d] to (%p) [%d, %d, %d, %d]\033[0m\n", __FUNCTION__, __LINE__, srcpsd->addr, srcx, srcy, dstpsd->addr, dstx, dsty, w, h);
	printf("\033[44m In %s:%d: Info about dstpsd: xres=%d, yres=%d, xvirtres=%d, yvirtres=%d\033[0m\n", __FUNCTION__, __LINE__, dstpsd->xres, dstpsd->yres, dstpsd->xvirtres, dstpsd->yvirtres);
	printf("\033[44m In %s:%d: Info about srcpsd: xres=%d, yres=%d, xvirtres=%d, yvirtres=%d\033[0m\n", __FUNCTION__, __LINE__, srcpsd->xres, srcpsd->yres, srcpsd->xvirtres, srcpsd->yvirtres);
#endif

	//if (dstpsd->addr == (void *)osd.buf_addr) {
	if (dstpsd->physical == (void *)osd.buf_addr) {
		if ((op & MWROP_EXTENSION) == MWROP_SWITCH_BUFFER || op == MWROP_SWITCH_BUFFER) {
#ifdef DRIVER_DEBUG
			printf("\033[44m Start to switch buffer.\033[0m\n");
#endif
			switch_flag = 1;
#ifdef SWITCH_BUFFER_TEST
			tmp_index = pic_index;
#endif
			pic_index++;
			pic_index &= 0x1;
		}
#ifdef NO_SWITCH_BUFFER_TEST
		if (switch_first == 0) {
#ifdef DRIVER_DEBUG
			printf("\033[44m Start to switch buffer.\033[0m\n");
#endif
			switch_flag = 1;
			pic_index++;
			pic_index &= 0x1;
			switch_first = 1;
		}
#endif
	}


	switch ((op & MWROP_EXTENSION)) {
		case MWROP_SRC_OVER:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (srcpsd->addr != (void *)GFX.pic_addr[0] && srcpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			if (dstpsd->addr != (void *)GFX.pic_addr[0] && dstpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Blend_Bitmap(&GFX, dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, w, h, op, dstpsd->xvirtres, srcpsd->xvirtres);
			break;
		case MWROP_COPY_RK:
		case MWROP_COPY:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (srcpsd->addr != (void *)GFX.pic_addr[0] && srcpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			if (dstpsd->addr != (void *)GFX.pic_addr[0] && dstpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Copy_Bitmap(&GFX, dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, w, h, op, dstpsd->xvirtres, srcpsd->xvirtres);
			break;
		case MWROP_BLENDCONSTANT:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (srcpsd->addr != (void *)GFX.pic_addr[0] && srcpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			if (dstpsd->addr != (void *)GFX.pic_addr[0] && dstpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Blend_BitmapFading(&GFX, dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, w, h, op, dstpsd->xvirtres, srcpsd->xvirtres, op & 0xff);
			break;
#ifdef GFX_KEYCOLOR_TEST
		case MWROP_COPY_IK:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (srcpsd->addr != (void *)GFX.pic_addr[0] && srcpsd->addr != (void *)GFX.pic_addr[1]) {
				flush_instr_cache();
				clean_data_cache();
			}
			if (dstpsd->addr != (void *)GFX.pic_addr[0] && dstpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Copy_IK_Bitmap(&GFX, dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, w, h, op, dstpsd->xvirtres, srcpsd->xvirtres);
			break;
#endif
		default:
#ifdef DRIVER_DEBUG
			printf("op is %lx.\n", op);
#endif
			fblinear32alpha.Blit (dstpsd, dstx, dsty, w, h, srcpsd, srcx, srcy, op);
			break;
	}

}

#ifdef DRAWAREA_TEST
static void em86xx_drawarea(PSD psd, driver_gc_t * gc)
{
	clean_data_cache();
	fblinear32alpha.DrawArea(psd, gc);
	clean_data_cache();
}
#endif

/* 
 * Initialize memory device with passed parms,
 * select suitable framebuffer subdriver,
 * and set subdriver in memory device.
 */
MWBOOL
em86xx_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
	int data_format, int size,void *addr)
{
	if (!gen_mapmemgc(mempsd, w, h, planes, bpp, data_format, linelen, size, addr)) {
		printf("%s, %d, gen_mapmemgc fail\n", __FUNCTION__, __LINE__);
		return 0;
	}
	// Replace with functions have been implemented in screen device driver
	mempsd->DrawHorzLine = em86xx_drawhorzline;
	mempsd->DrawVertLine = em86xx_drawvertline;
	mempsd->FillRect = em86xx_fillrect;
	mempsd->Blit = em86xx_blit;
	//mempsd->StretchBlit = em86xx_stretchblit;
	mempsd->StretchBlitEx = NULL;
	return 1;
}

static void em86xx_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw, MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw, MWCOORD srch, long op)
{
#ifdef SWITCH_BUFFER_TEST
	int tmp_index = 0;
#endif
	int switch_flag = 0;

#ifdef DRIVER_DEBUG
	printf("\033[44m In %s:%d: dstpsd->addr=%p, dstx=%d dsty=%d, dstw=%d, dsth=%d, srcpsd->addr=%p, srcx=%d, srcy=%d, srcw=%d, srch=%d, op=%lx\033[0m\n", __FUNCTION__, __LINE__, dstpsd->addr, dstx, dsty, dstw, dsth, srcpsd->addr, srcx, srcy, srcw, srch, op);
	printf("\033[44m In %s:%d: Info about dstpsd: xres=%d, yres=%d, xvirtres=%d, yvirtres=%d\033[0m\n", __FUNCTION__, __LINE__, dstpsd->xres, dstpsd->yres, dstpsd->xvirtres, dstpsd->yvirtres);
	printf("\033[44m In %s:%d: Info about srcpsd: xres=%d, yres=%d, xvirtres=%d, yvirtres=%d\033[0m\n", __FUNCTION__, __LINE__, srcpsd->xres, srcpsd->yres, srcpsd->xvirtres, srcpsd->yvirtres);
#endif

	//if (dstpsd->addr == (void *)osd.buf_addr) {
	if (dstpsd->physical == (void *)osd.buf_addr) {
		if ((op & MWROP_EXTENSION) == MWROP_SWITCH_BUFFER || op == MWROP_SWITCH_BUFFER) {
#ifdef DRIVER_DEBUG
			printf("\033[44m Start to switch buffer.\033[0m\n");
#endif
			switch_flag = 1;
#ifdef SWITCH_BUFFER_TEST
			tmp_index = pic_index;
#endif
			pic_index++;
			pic_index &= 0x1;
		}
#ifdef NO_SWITCH_BUFFER_TEST
		if (switch_first == 0) {
#ifdef DRIVER_DEBUG
			printf("\033[44m Start to switch buffer.\033[0m\n");
#endif
			switch_flag = 1;
			pic_index++;
			pic_index &= 0x1;
			switch_first = 1;
		}
#endif
	}


	switch ((op & MWROP_EXTENSION)) {
		case MWROP_SRC_OVER:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (srcpsd->addr != (void *)GFX.pic_addr[0] && srcpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			if (dstpsd->addr != (void *)GFX.pic_addr[0] && dstpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Blend_Bitmap(&GFX, dstpsd, dstx, dsty, dstw, dsth, srcpsd, srcx, srcy, srcw, srch, op | GFX_SUBOP_SCALE, dstpsd->xvirtres, srcpsd->xvirtres);
			break;
		case MWROP_COPY_RK:
		case MWROP_COPY:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (srcpsd->addr != (void *)GFX.pic_addr[0] && srcpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			if (dstpsd->addr != (void *)GFX.pic_addr[0] && dstpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Copy_Bitmap(&GFX, dstpsd, dstx, dsty, dstw, dsth, srcpsd, srcx, srcy, srcw, srch, op | GFX_SUBOP_SCALE, dstpsd->xvirtres, srcpsd->xvirtres);
			break;
		case MWROP_BLENDCONSTANT:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (srcpsd->addr != (void *)GFX.pic_addr[0] && srcpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			if (dstpsd->addr != (void *)GFX.pic_addr[0] && dstpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Blend_BitmapFading(&GFX, dstpsd, dstx, dsty, dstw, dsth, srcpsd, srcx, srcy, srcw, srch, op, dstpsd->xvirtres, srcpsd->xvirtres, op & 0xff);
			break;
#ifdef GFX_KEYCOLOR_TEST
		case MWROP_COPY_IK:
			// Please note: We need to clean data cache before call GFX command only when dstpsd->addr != osd.buf_addr.
			// If we clean data cache at wrong timeing, it may delay the speed.
			if (srcpsd->addr != (void *)GFX.pic_addr[0] && srcpsd->addr != (void *)GFX.pic_addr[1]) {
				flush_instr_cache();
				clean_data_cache();
			}
			if (dstpsd->addr != (void *)GFX.pic_addr[0] && dstpsd->addr != (void *)GFX.pic_addr[1]) {
				clean_data_cache();
			}
			GFX_Copy_IK_Bitmap(&GFX, dstpsd, dstx, dsty, dstw, dsth, srcpsd, srcx, srcy, srcw, srch, op, dstpsd->xvirtres, srcpsd->xvirtres);
			break;
#endif
		default:
#ifdef DRIVER_DEBUG
			printf("op is 0x%lx.\n", op);
#endif
			fblinear32alpha.Blit (dstpsd, dstx, dsty, dstw, dsth, srcpsd, srcx, srcy, op);
			break;
	}

}

void GFX_Fill_Rect(GFXContext *pGFX, PSD psd, MWCOORD x1,MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	struct RUA *pRUA = pGFX->pRUA;
	RMuint32 gfx = pGFX->gfx;

	struct GFXEngine_FillRectangle_type fill;

	NX_surface.SurfaceID = GFX_SURFACE_ID_NX;
	NX_surface.TotalWidth = psd->xvirtres;

	if (psd->flags & PSF_ADDRDEVMALLOC) 
		NX_surface.StartAddress = (RMuint32)RUAMAP_ALIGN(psd->physical);
	else
		NX_surface.StartAddress = (RMuint32)psd->addr;

	NX_surface.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &NX_surface, sizeof(NX_surface));

	fill.X = x1;
	fill.Y = y1;
	fill.Width = x2 - x1 + 1;
	fill.Height = y2 - y1 + 1;
	fill.Color = c;

#ifdef DRIVER_DEBUG
	printf("In %s:%d: X=%d, Y=%d, Width=%d, Height=%d, Color=%x\n", __FUNCTION__, __LINE__, fill.X, fill.Y, fill.Width, fill.Height, fill.Color);
#endif
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_FillRectangle, &fill, sizeof(fill));

	NX_surface.StartAddress = osd.buf_addr;
	NX_surface.TotalWidth = osd.bitmap_profile.Width;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &NX_surface, sizeof(NX_surface));
}

void GFX_SingleColor_Blend(GFXContext *pGFX, PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
	struct RUA *pRUA = pGFX->pRUA;
	RMuint32 gfx = pGFX->gfx;

	struct GFXEngine_SingleColorBlendRectangles_type sc_blend;


	NX_surface.SurfaceID = GFX_SURFACE_ID_NX;
	NX_surface.TotalWidth = psd->xvirtres;

	if (psd->flags & PSF_ADDRDEVMALLOC) 
		NX_surface.StartAddress = (RMuint32)RUAMAP_ALIGN(psd->physical);
	else
		NX_surface.StartAddress = (RMuint32)psd->addr;

	NX_surface.Tiled = FALSE;

	if (psd->flags & PSF_ADDRDEVMALLOC) 
		X_surface.StartAddress = (RMuint32)RUAMAP_ALIGN(psd->physical);
	else
		X_surface.StartAddress = (RMuint32)psd->addr;

	sc_blend.SaturateAlpha = 0;
	sc_blend.SrcX = x1;
	sc_blend.SrcY = y1;
	sc_blend.DstX = x1;
	sc_blend.DstY = y1;
	sc_blend.Width = x2 - x1 + 1;
	sc_blend.Height = y2 -x1 + 1;
	sc_blend.Color = c;

#ifdef DRIVER_DEBUG
	printf("In %s:%d: psd->addr = %p, srcX=%d, srcY=%d, dstX=%d, dsty=%d, Width=%d, Height=%d, Color=%x\n", __FUNCTION__, __LINE__, psd->addr, sc_blend.SrcX, sc_blend.SrcY, sc_blend.DstX, sc_blend.DstY, sc_blend.Width, sc_blend.Height, sc_blend.Color);
#endif

	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_SingleColorBlendRectangles, &sc_blend, sizeof(sc_blend));

	NX_surface.StartAddress = osd.buf_addr;
	NX_surface.TotalWidth = osd.bitmap_profile.Width;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &NX_surface, sizeof(NX_surface));
}

void GFX_Copy_Bitmap(GFXContext *pGFX, PSD dstpsd, int dx, int dy, int dw, int dh, PSD srcpsd, int sx, int sy, int sw, int sh, int flag, int dst_width, int src_width)
{
	struct RUA *pRUA = pGFX->pRUA;
	RMuint32 gfx = pGFX->gfx;

	struct GFXEngine_Surface_type surface_from;
	struct GFXEngine_Surface_type surface_to;
	struct GFXEngine_ColorFormat_type format;

	struct GFXEngine_MoveReplaceRectangle_type move;
	struct GFXEngine_MoveReplaceScaleRectangle_type move_scale;

#ifdef DRIVER_DEBUG
	printf("In %s:%d: map (src = %p) [%d, %d, %d, %d] to (dst = %p)[%d, %d, %d, %d], dstpsd->width = %d, srcpsd->width = %d\n", __FUNCTION__, __LINE__, srcpsd->addr, sx, sy, dw, dh, dstpsd->addr, dx, dy, dw, dh, dst_width, src_width);
#endif

	// Set input buffer.
	surface_from.SurfaceID = (flag & GFX_SUBOP_SCALE) ? GFX_SURFACE_ID_Z : GFX_SURFACE_ID_Y;

	if (srcpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_from.StartAddress = (RMuint32) RUAMAP_ALIGN(srcpsd->physical);
	else 
		surface_from.StartAddress = (RMuint32) srcpsd->addr;

	surface_from.TotalWidth = src_width;
	surface_from.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_from, sizeof(surface_from));

	format.SurfaceID = (flag & GFX_SUBOP_SCALE) ? GFX_SURFACE_ID_Z : GFX_SURFACE_ID_Y;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Set output buffer.
	surface_to.SurfaceID = GFX_SURFACE_ID_NX;

	if (dstpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_to.StartAddress = (RMuint32) RUAMAP_ALIGN(dstpsd->physical);
	else
		surface_to.StartAddress = (RMuint32) dstpsd->addr;

	surface_to.TotalWidth = dst_width;
	surface_to.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_to, sizeof(surface_to));

	format.SurfaceID = GFX_SURFACE_ID_NX;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	if (flag & GFX_SUBOP_SCALE) {
		move_scale.SrcX = sx;
		move_scale.SrcY = sy;
		move_scale.SrcWidth = sw;
		move_scale.SrcHeight = sh;
		move_scale.DstX = dx;
		move_scale.DstY = dy;
		move_scale.DstWidth = dw;
		move_scale.DstHeight = dh;
		move_scale.AlphaX = 0;
		move_scale.AlphaY = 0;
		move_scale.Merge = FALSE;
		SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_MoveAndScaleRectangle, &move_scale, sizeof(move_scale));
	} else {
		move.Width = dw;
		move.Height = dh;
		move.SrcX = sx;
		move.SrcY = sy;
		move.DstX = dx;
		move.DstY = dy;
		move.AlphaX = 0;
		move.AlphaY = 0;
		move.Merge = FALSE;
		SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_MoveRectangle, &move, sizeof(move));
	}
	NX_surface.StartAddress = osd.buf_addr;
	NX_surface.TotalWidth = osd.bitmap_profile.Width;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &NX_surface, sizeof(NX_surface));
}

#ifdef GFX_KEYCOLOR_TEST
void GFX_Copy_IK_Bitmap(GFXContext *pGFX, PSD dstpsd, int dx, int dy, int dw, int dh, PSD srcpsd, int sx, int sy, int sw, int sh, int flag, int dst_width, int src_width)
{
	struct RUA *pRUA = pGFX->pRUA;
	RMuint32 gfx = pGFX->gfx;

	struct GFXEngine_Surface_type surface_from;
	struct GFXEngine_Surface_type surface_to;

	struct GFXEngine_ColorFormat_type format;

	struct GFXEngine_MoveReplaceScaleRectangle_type move;
	
	struct GFXEngine_KeyColor_type keycolor;

	struct GFXEngine_AlphaPalette_type alpha_palette;

#ifdef DRIVER_DEBUG
	printf("In %s:%d: map (src = %p) [%d, %d, %d, %d] to (dst = %p)[%d, %d, %d, %d], dstpsd->width = %d, srcpsd->width = %d\n", __FUNCTION__, __LINE__, srcpsd->addr, sx, sy, dw, dh, dstpsd->addr, dx, dy, dw, dh, dst_width, src_width);
#endif

	// Set input buffer.
	surface_from.SurfaceID = GFX_SURFACE_ID_Z;

	if (srcpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_from.StartAddress = (RMuint32) RUAMAP_ALIGN(srcpsd->physical);
	else
		surface_from.StartAddress = (RMuint32) srcpsd->addr;

	surface_from.TotalWidth = src_width;
	surface_from.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_from, sizeof(surface_from));

	format.SurfaceID = GFX_SURFACE_ID_Z;
	format.MainMode = EMhwlibColorMode_TrueColor;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Set input buffer to remove keycolor. 
	keycolor.SurfaceID = GFX_SURFACE_ID_X;
	keycolor.Color = 0xeae6dd;
	keycolor.Range = 4;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_KeyColor, &keycolor, sizeof(keycolor));
	
	surface_to.SurfaceID = GFX_SURFACE_ID_X;

	if (srcpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_to.StartAddress = (RMuint32) RUAMAP_ALIGN(srcpsd->physical);
	else
		surface_to.StartAddress = (RMuint32) srcpsd->addr;

	surface_to.TotalWidth = src_width;
	surface_to.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_to, sizeof(surface_to));

	format.SurfaceID = GFX_SURFACE_ID_X;
	format.MainMode = EMhwlibColorMode_TrueColorWithKey;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Set output buffer.
	surface_to.SurfaceID = GFX_SURFACE_ID_NX;

	if (dstpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_to.StartAddress = (RMuint32) RUAMAP_ALIGN(dstpsd->physical);
	else
		surface_to.StartAddress = (RMuint32) dstpsd->addr;

	surface_to.TotalWidth = dst_width;
	surface_to.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_to, sizeof(surface_to));

	format.SurfaceID = GFX_SURFACE_ID_NX;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	move.SrcWidth = sw;
	move.SrcHeight = sh;
	move.DstWidth = dw;
	move.DstHeight = dh;
	move.SrcX = sx;
	move.SrcY = sy;
	move.DstX = dx;
	move.DstY = dy;
	move.AlphaX = 0;
	move.AlphaY = 0;
	move.Merge = TRUE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ReplaceAndScaleRectangle, &move, sizeof(move));

	NX_surface.StartAddress = osd.buf_addr;
	NX_surface.TotalWidth = osd.bitmap_profile.Width;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &NX_surface, sizeof(NX_surface));
}
#endif

void GFX_Blend_Bitmap(GFXContext *pGFX, PSD dstpsd, int dx, int dy, int dw, int dh, PSD srcpsd, int sx, int sy, int sw, int sh, int flag, int dst_width, int src_width)
{
	// Set Global variable
	struct RUA *pRUA = pGFX->pRUA;
	RMuint32 gfx = pGFX->gfx;

	struct GFXEngine_Surface_type surface_from;
	struct GFXEngine_Surface_type surface_on;
	struct GFXEngine_Surface_type surface_to;
	struct GFXEngine_ColorFormat_type format;

	struct GFXEngine_BlendAndScaleRectangles_type blend_param;

//	struct GFXEngine_KeyColor_type keycolor;

#ifdef DRIVER_DEBUG
	printf("In %s:%d: map (src = %p) [%d, %d, %d, %d] to (dst = %p)[%d, %d, %d, %d], dstpsd->width = %d, scrpsd->width = %d\n", __FUNCTION__, __LINE__, srcpsd->addr, sx, sy, sw, sh, dstpsd->addr, dx, dy, dw, dh, dst_width, src_width);
#endif

	// Set input buffer to blend
	surface_from.SurfaceID = GFX_SURFACE_ID_Z;

	if (srcpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_from.StartAddress = (RMuint32) RUAMAP_ALIGN(srcpsd->physical);
	else
		surface_from.StartAddress = (RMuint32) srcpsd->addr;

	surface_from.TotalWidth = src_width;
	surface_from.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_from, sizeof(surface_from));

	format.SurfaceID = GFX_SURFACE_ID_Z;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Set input buffer to be blend on
	surface_on.SurfaceID = GFX_SURFACE_ID_X;

	if (dstpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_on.StartAddress = (RMuint32) RUAMAP_ALIGN(dstpsd->physical);
	else 
		surface_on.StartAddress = (RMuint32) dstpsd->addr;

	surface_on.TotalWidth = dst_width;
	surface_on.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_on, sizeof(surface_on));

	format.SurfaceID = GFX_SURFACE_ID_X;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Set output buffer
	surface_to.SurfaceID = GFX_SURFACE_ID_NX;

	if (dstpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_to.StartAddress = (RMuint32) RUAMAP_ALIGN(dstpsd->physical);
	else
		surface_to.StartAddress = (RMuint32) dstpsd->addr;

	surface_to.TotalWidth = dst_width;
	surface_to.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_to, sizeof(surface_to));

	format.SurfaceID = GFX_SURFACE_ID_NX;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Do command
	blend_param.Src1X = sx;
	blend_param.Src1Y = sy;
	blend_param.SrcWidth = sw;
	blend_param.SrcHeight = sh;
	blend_param.Src2X = dx;  // Second rectangles to blend
	blend_param.Src2Y = dy;
	blend_param.DstX = dx;
	blend_param.DstY = dy;
	blend_param.DstWidth = dw;
	blend_param.DstHeight = dh;
	blend_param.SaturateAlpha = 0;

	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_BlendAndScaleRectangles, &blend_param, sizeof(blend_param));

	NX_surface.StartAddress = osd.buf_addr;
	NX_surface.TotalWidth = osd.bitmap_profile.Width;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &NX_surface, sizeof(NX_surface));
}


void GFX_Blend_BitmapFading(GFXContext *pGFX, PSD dstpsd, int dx, int dy, int dw, int dh, PSD srcpsd, int sx, int sy, int sw, int sh, int flag, int dst_width, int src_width, int alpha)
{
	// Set Global variable
	struct RUA *pRUA = pGFX->pRUA;
	RMuint32 gfx = pGFX->gfx;

	struct GFXEngine_Surface_type surface_from;
	struct GFXEngine_Surface_type surface_on;
	struct GFXEngine_Surface_type surface_to;
	struct GFXEngine_ColorFormat_type format;
	struct GFXEngine_AlphaPalette_type alpha_palette;
	RMbool bEnableAlphaFading = TRUE;

	struct GFXEngine_BlendAndScaleRectangles_type blend_param;

#ifdef DRIVER_DEBUG
	printf("In %s:%d: map (src = %p) [%d, %d, %d, %d] to (dst = %p)[%d, %d, %d, %d], dstpsd->width = %d, scrpsd->width = %d, alpha=%d\n", __FUNCTION__, __LINE__, srcpsd->addr, sx, sy, sw, sh, dstpsd->addr, dx, dy, dw, dh, dst_width, src_width, alpha);
#endif

	alpha_palette.Alpha0 = 0;
	alpha_palette.Alpha1 = alpha;
	alpha_palette.SurfaceID = GFX_SURFACE_ID_Z;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_AlphaPalette, &alpha_palette, sizeof(alpha_palette));

	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_EnableAlphaFading, &bEnableAlphaFading, sizeof(bEnableAlphaFading));

	// Set input buffer to blend
	surface_from.SurfaceID = GFX_SURFACE_ID_Z;

	if (srcpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_from.StartAddress = (RMuint32) RUAMAP_ALIGN(srcpsd->physical);
	else
		surface_from.StartAddress = (RMuint32) srcpsd->addr;

	surface_from.TotalWidth = src_width;
	surface_from.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_from, sizeof(surface_from));

	format.SurfaceID = GFX_SURFACE_ID_Z;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Set input buffer to be blend on
	surface_on.SurfaceID = GFX_SURFACE_ID_X;

	if (dstpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_on.StartAddress = (RMuint32) RUAMAP_ALIGN(dstpsd->physical);
	else
		surface_on.StartAddress = (RMuint32) dstpsd->addr;

	surface_on.TotalWidth = dst_width;
	surface_on.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_on, sizeof(surface_on));

	format.SurfaceID = GFX_SURFACE_ID_X;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Set output buffer
	surface_to.SurfaceID = GFX_SURFACE_ID_NX;

	if (dstpsd->flags & PSF_ADDRDEVMALLOC) 
		surface_to.StartAddress = (RMuint32) RUAMAP_ALIGN(dstpsd->physical);
	else
		surface_to.StartAddress = (RMuint32) dstpsd->addr;

	surface_to.TotalWidth = dst_width;
	surface_to.Tiled = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &surface_to, sizeof(surface_to));

	format.SurfaceID = GFX_SURFACE_ID_NX;
	format.MainMode = osd.bitmap_profile.ColorMode;
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	// Do command
	blend_param.Src1X = sx;
	blend_param.Src1Y = sy;
	blend_param.SrcWidth = sw;
	blend_param.SrcHeight = sh;
	blend_param.Src2X = dx;  // Second rectangles to blend
	blend_param.Src2Y = dy;
	blend_param.DstX = dx;
	blend_param.DstY = dy;
	blend_param.DstWidth = dw;
	blend_param.DstHeight = dh;
	blend_param.SaturateAlpha = 0;

	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_BlendAndScaleRectangles, &blend_param, sizeof(blend_param));

	bEnableAlphaFading = FALSE;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_EnableAlphaFading, &bEnableAlphaFading, sizeof(bEnableAlphaFading));

	alpha_palette.Alpha0 = 0;
	alpha_palette.Alpha1 = 255;
	alpha_palette.SurfaceID = GFX_SURFACE_ID_Z;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_AlphaPalette, &alpha_palette, sizeof(alpha_palette));

	NX_surface.StartAddress = osd.buf_addr;
	NX_surface.TotalWidth = osd.bitmap_profile.Width;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &NX_surface, sizeof(NX_surface));
}


int Sigma86_Init (void)
{
	RMuint32 local_surfaceID = 0, osd_scaler_id;
	RMuint32 osd_scaler = EMHWLIB_MODULE(DispOSDScaler,0);
	RMstatus err = RM_OK;

	init_video_options(&video_opt);
	init_playback_options(&play_opt);

	err = RUACreateInstance(&pRUA, play_opt.chip_num);
	if (RMFAILED(err)) {
		fprintf(stderr, "Error creating RUA instance! %d\n", err);
		return MR_ERROR;
	}
	
	err = DCCOpen(pRUA, &pDCC);
	if (RMFAILED(err)) {
		fprintf(stderr, "Error Opening DCC! %d\n", err);
		return MR_ERROR;
	}

	err = DCCInitChainEx(pDCC, DCCInitMode_LeaveDisplay);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot initialize microcode %d\n", err);
		return MR_ERROR;
	}
	
	dcc_info.pRUA = pRUA;
 	dcc_info.pDCC = pDCC;
	dcc_info.route = DCCRoute_Main;

	/* with bootirq sdk, we must reset osd_scaler here, otherwise
	 * dmaosd can't bootup.
	 */
	err = DCCSetSurfaceSource(dcc_info.pDCC, osd_scaler, NULL);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot unset osd scaler's surface\n");
		return MR_ERROR;
	}

	/* the mixer should not modify the GFX scaler's config */
	{
		enum EMhwlibMixerSourceState state;
		RMuint32 mixer, src_index, mixer_src;

		mixer = EMHWLIB_MODULE(DispMainMixer, 0);


		err = DCCGetScalerModuleID(dcc_info.pDCC, dcc_info.route, DCCSurface_OSD, 0, &(osd_scaler_id));
		if (RMFAILED(err)) {
			RMDBGLOG((ENABLE, "Cannot get surface to display OSD source %d\n", err));
			return MR_ERROR;
		}
		/* let the mixer know that this scaler will be used */

	fprintf(stderr, "########################%s:%d, osd_scaler_id=0x%08lx##########\n", 
			__FUNCTION__, __LINE__, osd_scaler_id);
		err = RUAExchangeProperty(dcc_info.pRUA, mixer, RMGenericPropertyID_MixerSourceIndex, &osd_scaler_id, sizeof(osd_scaler_id), &src_index, sizeof(src_index));
		if (RMFAILED(err)) {
			RMDBGLOG((ENABLE, "Cannot get scaler index\n"));
			return MR_ERROR;
		}
		mixer_src = EMHWLIB_TARGET_MODULE(mixer, 0, src_index);

		state = EMhwlibMixerSourceState_Slave;

		while((err =  RUASetProperty(dcc_info.pRUA, mixer_src, RMGenericPropertyID_MixerSourceState, &state, sizeof(state), 0))==RM_PENDING);
		if (RMFAILED(err)) {
			RMDBGLOG((ENABLE, "Cannot set scaler's state on mixer\n"));
			return MR_ERROR;
		}

		while ((err = RUASetProperty(dcc_info.pRUA, mixer, RMGenericPropertyID_Validate, NULL, 0, 0)) == RM_PENDING);
		if (RMFAILED(err)) {
			fprintf(stderr, "Cannot validate mixer\n");
			return MR_ERROR;
		}
	}

	/* open a video source with two pictures */
	// Set OSD pixmap
	memset(&osd, 0, sizeof(osd));
	osd.bitmap_profile.SamplingMode = EMhwlibSamplingMode_444;
	osd.bitmap_profile.ColorMode = EMhwlibColorMode_TrueColor;
	osd.bitmap_profile.ColorFormat = EMhwlibColorFormat_32BPP;
	osd.bitmap_profile.Width = DESKTOP_WIDTH;
	osd.bitmap_profile.Height = DESKTOP_HEIGHT;
	osd.bitmap_profile.ColorSpace = EMhwlibColorSpace_RGB_0_255;
	osd.bitmap_profile.PixelAspectRatio.X = 1;
	osd.bitmap_profile.PixelAspectRatio.Y = 1;

	err = DCCOpenOSDVideoSource(dcc_info.pDCC, 
				    &(osd.bitmap_profile), 
				    &pVideoSource);

	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot open OSD decoder %d\n", err);
		return MR_ERROR;
	}

	err = DCCGetOSDSurfaceInfo(dcc_info.pDCC, pVideoSource, NULL, &surface_addr, NULL);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot get surface address %d\n", err);
		return MR_ERROR;
	}

	err = DCCGetOSDVideoSourceInfo(pVideoSource, 
				       &(pic_luma_addr[0]),
				       NULL,
				       NULL,
				       NULL);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot DCCGetOSDVideoSourceInfo %d\n", err);
		return MR_ERROR;
	}

	err = DCCClearOSDVideoSource(pVideoSource);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot DCCClearOSDVideoSource %d\n", err);
		return MR_ERROR;
	}

#ifdef DRIVER_DEBUG
	printf("%s, %d, pic_addr=0x%lx, 0x%lx, pic_luma_addr=0x%lx, 0x%lx\n", __FUNCTION__, __LINE__, pic_addr[0], pic_addr[1], pic_luma_addr[0], pic_luma_addr[1]);
#endif

	/*possibly clean it before this with a fill */

	err = DCCSetSurfaceSource(dcc_info.pDCC, osd_scaler, pVideoSource);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot set the surface source %d\n", err);
		return MR_ERROR;
	}




	err = DCCEnableVideoSource(pVideoSource, TRUE);
	if (RMFAILED(err)){
		fprintf(stderr,"Error enabling OSD buffer : %d\n",err);
		return MR_ERROR;
	}


	while ((err = RUASetProperty(dcc_info.pRUA, osd_scaler, RMGenericPropertyID_Validate, NULL, 0, 0)) == RM_PENDING);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot validate scaler input window %d\n", err);
		return MR_ERROR;
	}

	RUAGetProperty(dcc_info.pRUA, osd_scaler, RMGenericPropertyID_Surface, &local_surfaceID, sizeof(local_surfaceID));
	fprintf(stderr,"%s:%d osd.buf_addr=%p, get osd surface = 0x%08lx, surf addr=%p\n",
			__FUNCTION__, __LINE__, (void *)(osd.buf_addr),local_surfaceID, &local_surfaceID);


	osd.buf_addr = pic_luma_addr[0];

	return MR_OK;
}

int GFX_Init (void)
{
	RMuint32 chip_num;
	RMuint32 gfx_count; 

	struct GFXEngine_DRAMSize_in_type  dramSizeIn;
	struct GFXEngine_DRAMSize_out_type dramSizeOut;
	RMint32 i;
	
	RMstatus err = RM_OK;

	struct GFXEngine_ColorFormat_type format;
	
	chip_num = 0;
		
	dramSizeIn.CommandFIFOCount = 10;
	err = RUAExchangeProperty(pRUA, EMHWLIB_MODULE(GFXEngine,0), RMGFXEnginePropertyID_DRAMSize, &dramSizeIn, sizeof(dramSizeIn), &dramSizeOut, sizeof(dramSizeOut));
	if (RMFAILED(err)) {
		fprintf(stderr, "Error getting dram size for gfx engine\n");
		return MR_ERROR;
	}
		
	gfx_profile.CommandFIFOCount = dramSizeIn.CommandFIFOCount;
	gfx_profile.Priority = 1;
	gfx_profile.CachedSize = dramSizeOut.CachedSize;
	gfx_profile.UncachedSize = dramSizeOut.UncachedSize;
	
	if (gfx_profile.CachedSize > 0) {
		gfx_profile.CachedAddress = RUAMalloc(pRUA, 0, RUA_DRAM_CACHED, gfx_profile.CachedSize);
	} else {
		gfx_profile.CachedAddress = 0;
	}
		
	gfx_profile.UncachedSize = dramSizeOut.UncachedSize;
	if (gfx_profile.UncachedSize > 0) {
		gfx_profile.UncachedAddress = RUAMalloc(pRUA, 0, RUA_DRAM_UNCACHED, gfx_profile.UncachedSize);
	} else {
		gfx_profile.UncachedAddress = 0;
	}

	gfx = GFXEngine;

	i = 1;
	{
		enum RMcategoryID category;
		while(TRUE){
			err = RUAExchangeProperty(pRUA, EMHWLIB_MODULE(Enumerator,0),  RMEnumeratorPropertyID_IndexToCategoryID, &i, sizeof(i), &category, sizeof(category) );
			if(err != RM_OK) break;
			i++;
					
			gfx_count = 33;
			
			err = RUAExchangeProperty(pRUA, EMHWLIB_MODULE(Enumerator,0),  RMEnumeratorPropertyID_CategoryIDToNumberOfInstances, &category, sizeof(category), &gfx_count, sizeof(gfx_count));
			if (RMFAILED(err)) {
				fprintf(stderr, "Error getting gfx engine count\n");
				return MR_ERROR;
			}
		}
	}

	gfx_count = 4;
			
	for (i = 0 ; i < (RMint32) gfx_count ; i++) {
		gfx = EMHWLIB_MODULE(GFXEngine, i);
		err = RUASetProperty(pRUA, gfx, RMGFXEnginePropertyID_Open, &gfx_profile, sizeof(gfx_profile), 0);
		if (err == RM_OK) break;
	}
	if (i==(RMint32)gfx_count) {
		fprintf(stderr, "Cannot open a gfx engine [0..%lu[\n", gfx_count);
		return MR_ERROR;
	}


	/* 
	 * wait for the picture to be on display. This is the correct way to make the
	 * next double buffering implementation work 100%
	 */
	{
		struct RUAEvent e;
		RMuint32 index;

		e.ModuleID = EMHWLIB_MODULE(DisplayBlock, 0);
		e.Mask = EMHWLIB_DISPLAY_NEW_PICTURE_EVENT_ID(DispOSDScaler);
		err = RUAWaitForMultipleEvents(pRUA, &e, 1, 1000000, &index);
		if (err == RM_ERROR) {
			fprintf(stderr, "cannot wait for the picture to be on display\n");
		}
	}

	display_pic.Pts = 0;
	display_pic.Surface = surface_addr;

	X_surface.SurfaceID = GFX_SURFACE_ID_X;
	X_surface.Tiled = FALSE;

	GFX.pRUA = pRUA;
	GFX.gfx = gfx;
	GFX.pic_addr[0] = (unsigned long int) pic_luma_addr[0];
	GFX.pic_addr[1] = (unsigned long int) pic_luma_addr[1];
	GFX.pic_index = (unsigned long int) pic_index;

	YZ_surface.StartAddress = osd.buf_addr;
	YZ_surface.TotalWidth = osd.bitmap_profile.Width;
	YZ_surface.Tiled = FALSE;

	NX_surface.SurfaceID = GFX_SURFACE_ID_NX;
	NX_surface.StartAddress = osd.buf_addr;
	NX_surface.TotalWidth = osd.bitmap_profile.Width;
	NX_surface.Tiled = FALSE;

	format.MainMode = osd.bitmap_profile.ColorMode; 
	format.SubMode = osd.bitmap_profile.ColorFormat;
	format.SamplingMode = osd.bitmap_profile.SamplingMode;

	format.SurfaceID = GFX_SURFACE_ID_NX;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));

	format.SurfaceID = GFX_SURFACE_ID_Z;
	YZ_surface.SurfaceID = GFX_SURFACE_ID_Z;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &YZ_surface, sizeof(YZ_surface));

	format.SurfaceID = GFX_SURFACE_ID_Y;
	YZ_surface.SurfaceID = GFX_SURFACE_ID_Y;
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_ColorFormat, &format, sizeof(format));
	SEND_GFX_COMMAND(pRUA, gfx, RMGFXEnginePropertyID_Surface, &YZ_surface, sizeof(YZ_surface));

	return MR_OK;
}

void GFX_Done (void)
{
	RMstatus err = RM_OK;
	RMuint32 close_profile;

	err = RUASetProperty(pRUA, gfx, RMGFXEnginePropertyID_Close, &close_profile, sizeof(close_profile), 0);
	if (RMFAILED(err)) fprintf(stderr, "Cannot close the gfx accelerator\n");
	
	if(gfx_profile.CachedAddress)		
		RUAFree(pRUA, gfx_profile.CachedAddress);
	
	if(gfx_profile.UncachedAddress)		
		RUAFree(pRUA, gfx_profile.UncachedAddress);
}

void Sigma86_Done (void)
{
	RMstatus err = RM_OK;

	err = DCCCloseVideoSource(pVideoSource);
	if (RMFAILED(err)) {
		RMDBGLOG((ENABLE, "Cannot close osd source %s\n", RMstatusToString(err)));
	}
	
	err = DCCClose(pDCC);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot close DCC %d\n", err);
	}

	err = RUADestroyInstance(pRUA);
	if (RMFAILED(err)) {
		fprintf(stderr, "Cannot destroy RUA instance %d\n", err);
	}
}

