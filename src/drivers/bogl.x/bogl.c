/* BOGL - Ben's Own Graphics Library.
   Written by Ben Pfaff <pfaffben@debian.org>.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#define _GNU_SOURCE 1
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "bogl.h"
#include "boglP.h"
#include "bogl-vga16.h"
#include "bogl-cfb8.h"

/* BOGL main code. */

#ifndef FB_TYPE_VGA_PLANES
#define FB_TYPE_VGA_PLANES 4
#endif

#ifndef unused
#define unused __attribute__((unused))
#endif

/* Global variables. */
int bogl_xres, bogl_yres, bogl_ncols;	/* bogl.h */
int bogl_refresh;
unsigned char *bogl_frame;		/* boglP.h */
int bogl_drawing;
int bogl_line_len;
int bogl_truecolor;

/* Static variables. */
static int fb;			/* Framebuffer file handle. */
static int tty;			/* Tty file handle. */
static size_t map_amt;		/* Size of mmapped block. */

static struct vt_mode mode;	/* Terminal mode. */
static int tty_no;		/* Tty that we own. */
static int type;		/* Video type, one of FB_TYPE_*. */
static int visual;		/* Visual type, one of FB_VISUAL_*. */

static int visible;		/* Is our VT visible? */

static int status;		/* 0=never initialized, 1=once initialized,
				   2=currently initialized. */
static char *error;		/* Error message. */

/* Saved color palette. */
static __u16 saved_red[16];
static __u16 saved_green[16];
static __u16 saved_blue[16];

static int gray;		/* Convert colors to grayscale? */

/* Functions. */

static size_t init_fb (void);
static int draw_enable (void);
static void draw_disable (void);

static void vt_switch (int);

/* Initialize BOGL. */
int
bogl_init (void)
{
  struct fb_fix_screeninfo fb_fix;
  struct fb_var_screeninfo fb_var;
  struct vt_stat vts;
  char *env;

  assert (status < 2);
  visible = 1;
  
  if( !(env = getenv("FRAMEBUFFER")))
	  env = "/dev/fb0";
  fb = open (env, O_RDWR);
  if (fb < 0)
    return bogl_fail ("opening %s: %m", env);

  tty = open ("/dev/tty0", O_RDWR);
  if (tty < 0)
    return bogl_fail ("opening /dev/tty0: %m");

  if (-1 == ioctl (tty, VT_GETSTATE, &vts))
    return bogl_fail ("can't get VT state: %m");
  tty_no = vts.v_active;

  if (-1 == ioctl (fb, FBIOGET_FSCREENINFO, &fb_fix)
      || -1 == ioctl (fb, FBIOGET_VSCREENINFO, &fb_var))
    return bogl_fail ("reading screen info: %m");
  
  bogl_xres = fb_var.xres;
  bogl_yres = fb_var.yres;
  bogl_ncols = 1 << fb_var.bits_per_pixel;
  bogl_line_len = fb_fix.line_length;
  type = fb_fix.type;
  visual = fb_fix.visual;
  bogl_truecolor = (visual == FB_VISUAL_TRUECOLOR);

//printf("smem_start %x\n", (int)fb_fix.smem_start);
//printf("smem_len %x\n", fb_fix.smem_len);
//printf("%dx%dx%d linelen %d type %d visual %d\n", bogl_xres, bogl_yres, bogl_ncols, bogl_line_len, type, visual);

  if (!draw_enable ())
    return bogl_fail ("don't know screen type %d", type);

  if (ioctl (tty, VT_GETMODE, &mode) == -1)
    return bogl_fail ("can't get VT mode: %m");

  mode.mode = VT_PROCESS;
  mode.relsig = SIGUSR2;
  mode.acqsig = SIGUSR2;

  signal (SIGUSR2, vt_switch);

  if (-1 == ioctl (tty, VT_SETMODE, &mode))
    return bogl_fail ("can't set VT mode: %m");

  if (-1 == ioctl (tty, KDSETMODE, KD_GRAPHICS)) 
    return bogl_fail ("setting graphics mode: %m");

  map_amt = init_fb ();
//printf("mapamt %x\n", map_amt);
  if (!map_amt)
    return 0;
  map_amt = (map_amt + getpagesize () - 1) / getpagesize () * getpagesize ();

  bogl_frame = mmap (NULL, map_amt, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

  if (bogl_frame == NULL || bogl_frame == (unsigned char *) -1)
     return bogl_fail ("mmaping /dev/fb0: %m");

  
  {
    static struct fb_cmap cmap;

    cmap.start = 0;
    cmap.len = 16;
    cmap.red = saved_red;
    cmap.green = saved_green;
    cmap.blue = saved_blue;
    cmap.transp = NULL;

    ioctl (fb, FBIOGETCMAP, &cmap);
  }

  if (!status)
    atexit (bogl_done);
  status = 2;

  return 1;
}

static size_t
init_fb (void)
{
#if BOGL_VGA16_FB
  if (type == FB_TYPE_VGA_PLANES)
    return bogl_vga16_init (fb);
#endif

#if BOGL_CFB8_FB
  if (type == FB_TYPE_PACKED_PIXELS
      && (visual == FB_VISUAL_PSEUDOCOLOR || visual == FB_VISUAL_TRUECOLOR)
      && bogl_ncols == 256)
    return bogl_cfb8_init (fb);
#endif

  return bogl_fail ("unknown or unsupported framebuffer: type %d, "
		    "visual %d, %d colors", type, visual, bogl_ncols);
}

/* Close down BOGL. */
void
bogl_done (void)
{
  if (status != 2)
    return;
  status = 1;

  //bogl_clear (0, 0, bogl_xres, bogl_yres, 0);
  
  {
    static struct fb_cmap cmap;

    cmap.start = 0;
    cmap.len = 16;
    cmap.red = saved_red;
    cmap.green = saved_green;
    cmap.blue = saved_blue;
    cmap.transp = NULL;

    ioctl (fb, FBIOPUTCMAP, &cmap);
  }
  
  munmap ((void *) bogl_frame, map_amt);
  
  signal (SIGUSR2, SIG_DFL);

  ioctl (tty, KDSETMODE, KD_TEXT);

  mode.mode = VT_AUTO;
  mode.relsig = 0;
  mode.acqsig = 0;
  ioctl (tty, VT_SETMODE, &mode);
  
  close (tty);
  close (fb);
}

#if 0000
/* Draw a hollow rectangle from (X1,Y1) to (X2,Y2) in color C. */
void
bogl_rectangle (int x1, int y1, int x2, int y2, int c)
{
  bogl_hline (x1, x2, y1, c);
  bogl_vline (x1, y1, y2, c);
  bogl_hline (x1, x2, y2 - 1, c);
  bogl_vline (x2 - 1, y1, y2, c);
}

/* Returns the width of string S when output in font FONT. */
int
bogl_metrics (const char *s, int n, struct bogl_font *font)
{
  int cx = 0;
  
  while (n--)
    cx += font->width[(unsigned char) *s++];
  return cx;
}
#endif

/* Set whether to convert colors to grayscale.  You'll need to re-set
   the palette with bogl_set_palette() for this to take effect. */
void
bogl_gray_scale (int make_gray)
{
  gray = make_gray;
}

/* Set NC color palettes values starting at C to red-green-blue value
   specified in PALETTE.  Use 8-bit color values as input. */
void
bogl_set_palette (int c, int nc, const unsigned char (*palette)[3])
{
  struct fb_cmap cmap;

  __u16 red[nc];
  __u16 green[nc];
  __u16 blue[nc];

  int i;

  for (i = 0; i < nc; i++)
    {
      const unsigned char *e = palette[i];
      
      if (gray)
	red[i] = green[i] = blue[i] = (e[0] * 77 + e[1] * 151 + e[2] * 28);
      else
	{
	  red[i] = e[0] << 8;
	  green[i] = e[1] << 8;
	  blue[i] = e[2] << 8;
	}
    }

  cmap.start = c;
  cmap.len = nc;
  cmap.red = red;
  cmap.green = green;
  cmap.blue = blue;
  cmap.transp = NULL;

  if(ioctl (fb, FBIOPUTCMAP, &cmap) == -1)
	  printf("putcmap fail\n");
}

/* Returns the oldest error message since this function was last
   called.  Clears the error state.  Returns a null pointer if no
   errors have occurred.  The caller must free the returned
   pointer if memory leaks are to be prevented. */
const char *
bogl_error (void)
{
  char *msg = error;
  error = NULL;
  return msg;
}

/* Drawing function setup/disable. */

/* Dummy drawing functions to disable display. */
static void
dummy_pixel (int x unused, int y unused, int c unused)
{}

static int
dummy_readpixel (int x unused, int y unused)
{
	return 0;
}

static void 
dummy_hline (int x1 unused, int x2 unused, int y unused, int c unused)
{}

static void 
dummy_vline (int x unused, int y1 unused, int y2 unused, int c unused)
{}

static void
dummy_text (int x unused, int y unused, const char *s unused, int n unused,
	    int fg unused, int bg unused, struct bogl_font *font unused)
{}

static void
dummy_clear (int x1 unused, int y1 unused, int x2 unused, int y2 unused,
	     int c unused)
{}

static void
dummy_move (int sx unused, int sy unused, int dx unused, int dy unused,
	    int w unused, int h unused)
{}

static void
dummy_put (int x unused, int y unused, const struct bogl_pixmap *pixmap unused,
	   const int color_map[] unused)
{}

static void
dummy_pointer (int visible unused, int x unused, int y unused,
	       const struct bogl_pointer *pointer unused,
	       int colors[2] unused)
{}


/* Enable drawing by setting the bogl_* device-specific functions to
   their appropriate values for the detected device. */
static int
draw_enable (void)
{
#if BOGL_VGA16_FB
  if (type == FB_TYPE_VGA_PLANES)
    {
      bogl_pixel = bogl_vga16_pixel;
      bogl_readpixel = bogl_vga16_readpixel;
      bogl_hline = bogl_vga16_hline;
      bogl_vline = bogl_vga16_vline;
#if !SMALLBOGL
      bogl_clear = bogl_vga16_clear;
      bogl_text = bogl_vga16_text;
      bogl_put = bogl_vga16_put;
      bogl_pointer = bogl_vga16_pointer;
#endif
      return 1;
    }
#endif

#if BOGL_CFB8_FB
  if (type == FB_TYPE_PACKED_PIXELS
      && (visual == FB_VISUAL_PSEUDOCOLOR || visual == FB_VISUAL_TRUECOLOR)
      && bogl_ncols == 256)
    {
      bogl_pixel = bogl_cfb8_pixel;
      bogl_readpixel = bogl_cfb8_readpixel;
      bogl_hline = bogl_cfb8_hline;
      bogl_vline = bogl_cfb8_vline;
#if !SMALLBOGL
      bogl_clear = bogl_cfb8_clear;
      bogl_text = bogl_cfb8_text;
      bogl_put = bogl_cfb8_put;
      bogl_pointer = bogl_cfb8_pointer;
#endif
      return 1;
    }
#endif

  return 0;
}
      
/* Disable drawing by setting all the bogl_* device-specific functions
   to dummy functions. */
static void
draw_disable (void)
{
  bogl_pixel = dummy_pixel;
  bogl_readpixel = dummy_readpixel;
  bogl_hline = dummy_hline;
  bogl_vline = dummy_vline;
  bogl_text = dummy_text;
  bogl_clear = dummy_clear;
  bogl_move = dummy_move;
  bogl_put = dummy_put;
  bogl_pointer = dummy_pointer;
}

/* Signal handler called whenever the kernel wants to switch to or
   from our tty. */
static void
vt_switch (int sig unused)
{
  signal (SIGUSR2, vt_switch);

  /* If a BOGL drawing function is in progress then we cannot mode
     switch right now because the drawing function would continue to
     scribble on the screen after the switch.  So disable further
     drawing and schedule an alarm to try again in .1 second. */
  if (bogl_drawing)
    {
      draw_disable ();

      signal (SIGALRM, vt_switch);
      
      {
	struct itimerval duration;
	
	duration.it_interval.tv_sec = 0;
	duration.it_interval.tv_usec = 0;
	duration.it_value.tv_sec = 0;
	duration.it_value.tv_usec = 100000;
	if (-1 == setitimer (ITIMER_REAL, &duration, NULL))
	  bogl_fail ("can't set timer: %m");
      }
      
      return;
    }
      
  if (visible)
    {
      visible = 0;
      draw_disable ();

      if (-1 == ioctl (tty, VT_RELDISP, 1))
	bogl_fail ("can't switch away from VT: %m");
    }
  else
    {
      visible = 1;
      draw_enable ();
      
      if (-1 == ioctl (tty, VT_RELDISP, VT_ACKACQ))
	bogl_fail ("can't acknowledge VT switch: %m");

      bogl_refresh = 1;
    }
}

/* Sets the BOGL error message to MESSAGE if there is none already
   set.  Returns 0. */
int
bogl_fail (const char *format, ...)
{
  va_list args;

  if (error)
    return 0;

  va_start (args, format);
  vasprintf (&error, format, args);
  va_end (args);
printf("error %s\n", error);
printf("%dx%dx%d linelen %d type %d visual %d\n", bogl_xres, bogl_yres, bogl_ncols, bogl_line_len, type, visual);
printf("Screen types supported:\n");
#if BOGL_VGA16_FB
	printf("%d\n", FB_TYPE_VGA_PLANES);
#endif
#if BOGL_CFB8_FB
	printf("%d\n", FB_TYPE_PACKED_PIXELS);
#endif
  return 0;
}

