/**
 * NDS True Color 555 screen driver for Microwindows
 * Copyright (c) 2011 Derek Carter
 *
 * This is for use in driving the screen in true color modes
 */
#include <nds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

/* Used to identify the Alpha bit of the packed color pixel */
#define ALPHA_BIT (1 << 15)

/* VGA driver entry points*/
static PSD  NDS555_open(PSD psd);
static void NDS555_close(PSD psd);
static void NDS555_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void NDS555_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);

static MWBOOL NDS555_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
		int linelen,int size,void *addr);
static void NULL_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w,
		MWCOORD h, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op) {}
static PSD  NULL_allocatememgc(PSD psd) { return NULL; }
static MWBOOL NULL_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
			int linelen,int size,void *addr) { return 0; }
static void NULL_freememgc(PSD mempsd) {}

SUBDRIVER subdrivernds;
SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	NDS555_open,
	NDS555_close,
	NDS555_setpalette,       
	NDS555_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,        
	NULL,				/* Update*/
	NULL				/* PreSelect*/
};

/* operating mode*/
static MWBOOL VGAMODE = TRUE;	/* ega or vga screen rows*/

/* int10 functions*/
#define FNGR640x480	0x0012	/* function for graphics mode 640x480x16*/
#define FNGR640x350	0x0010	/* function for graphics mode 640x350x16*/
#define FNTEXT		0x0003	/* function for 80x25 text mode*/

#if _MINIX
FARADDR int10(int mode, int z)
{
int fd;
struct mio_int86 mint86;

   fd = open("/dev/mem", O_RDONLY);
   memset(&mint86, 0, sizeof(mint86));
   ioctl(fd, MIOCINT86, &mint86);
   mint86.reg86.b.intno = 0x10;
   mint86.reg86.b.al = mode & 0xFF;
   ioctl(fd, MIOCINT86, &mint86);
   close(fd);
}
#endif

static PSD
NDS555_open(PSD psd)
{
  PSUBDRIVER subdriver;

  /* init driver variables depending on ega/vga mode*/
  psd->xres = psd->xvirtres = 256;
  psd->yres = psd->yvirtres = 192;
  psd->planes = 1;
  psd->bpp = 16;
  psd->ncolors = 1 << 16;
  psd->pixtype = MWPF_TRUECOLOR1555;
  psd->portrait = MWPORTRAIT_NONE;
  psd->data_format = set_data_format(psd);

  /* Calculate the correct size and pitch from xres, yres and bpp*/
  GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp,
		   &psd->size, &psd->pitch);


  psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);

  psd->flags = PSF_SCREEN;

  psd->addr = (void*) 0x06000000;

  /*
   * set and initialize subdriver into screen driver
   * psd->size is calculated by subdriver init
   */
  
  // Currently attempting to use FB16 subdriver
  subdriver = select_fb_subdriver(psd);

  // Check that a valid subdriver exists
  if (!subdriver) 
    {
      EPRINTF("No driver for screen bpp %d\n", psd->bpp);
      return NULL;
    }
 
  psd->orgsubdriver = subdriver;

  set_subdriver(psd, subdriver);
  
  // Configure the DS control registers
  powerOn(POWER_ALL_2D);
  videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE);
  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
  REG_BG2CNT = BG_BMP16_256x256 | BG_BMP_BASE(0);
  REG_BG2PA = ((psd->xres / 256) << 8) | (psd->xres % 256) ; 
  REG_BG2PB = 0;
  REG_BG2PC = 0;
  REG_BG2PD = ((psd->yres / 192) << 8) | ((psd->yres % 192) + (psd->yres % 192) / 3) ;
  REG_BG2X  = 0;
  REG_BG2Y  = 0;

  return psd;
}

static void
NDS555_close(PSD psd)
{
  powerOff(POWER_LCD);
}

static void
NDS555_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
  psi->rows = psd->yvirtres;
  psi->cols = psd->xvirtres;
  psi->planes = psd->planes;
  psi->bpp = psd->bpp;
  psi->ncolors = psd->ncolors;
  psi->pixtype = psd->pixtype;
  psi->fonts = NUMBER_FONTS;
  psi->portrait = psd->portrait;
  psi->pixtype = psd->pixtype;
  switch (psd->pixtype) 
    {
    case MWPF_TRUECOLOR8888:
    case MWPF_TRUECOLOR888:
      psi->rmask 	= 0xff0000;
      psi->gmask 	= 0x00ff00;
      psi->bmask	= 0x0000ff;
      break;
    case MWPF_TRUECOLOR565:
      psi->rmask 	= 0xf800;
      psi->gmask 	= 0x07e0;
      psi->bmask	= 0x001f;
      break;
    case MWPF_TRUECOLOR555:
      psi->rmask 	= 0x7c00;
      psi->gmask 	= 0x03e0;
      psi->bmask	= 0x001f;
      break;
    case MWPF_TRUECOLOR332:
      psi->rmask 	= 0xe0;
      psi->gmask 	= 0x1c;
      psi->bmask	= 0x03;
      break;
    case MWPF_PALETTE:
    default:
      psi->rmask 	= 0xff;
      psi->gmask 	= 0xff;
      psi->bmask	= 0xff;
      break;
    }
  
  if(psd->yvirtres > 480) {
    /* SVGA 800x600*/
    psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
    psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
  } else if(psd->yvirtres > 350) {
    /* VGA 640x480*/
    psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
    psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
  } else if(psd->yvirtres <= 240) {
    /* half VGA 640x240 */
    psi->xdpcm = 14;        /* assumes screen width of 24 cm*/ 
    psi->ydpcm =  5;
  } else {
    /* EGA 640x350*/
    psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
    psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
  }
}

static void
NDS555_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	/* not yet implemented, no colour palette assumed*/
}

