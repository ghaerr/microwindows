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

#ifndef bogl_h
#define bogl_h

/* Proportional font structure definition. */
struct bogl_font
  {
    char *name;				/* Font name. */
    int height;				/* Height in pixels. */
    unsigned long *content;		/* 32-bit right-padded bitmap array. */
    short *offset;			/* 256 offsets into content. */
    unsigned char *width;		/* 256 character widths. */
  };

/* Pixmap structure definition. */
struct bogl_pixmap
  {
    int width, height;			/* Width, height in pixels. */
    int ncols;				/* Number of colors. */
    int transparent;			/* Transparent color or -1 if none. */
    unsigned char (*palette)[3];	/* Palette. */
    unsigned char *data;		/* Run-length compressed data. */
  };

/* Pointer structure definition. */
struct bogl_pointer
  {
    int hx, hy;				/* Hot spot. */
    unsigned short mask[16];		/* Drawing mask: 0=clear, 1=drawn. */
    unsigned short color[16];		/* Pixel colors: 0=black, 1=white. */
  };

/* Screen parameters. */
extern int bogl_xres, bogl_yres, bogl_ncols;
extern int bogl_truecolor;

/* 1=Must refresh screen due to tty change. */
extern int bogl_refresh;

/* Generic routines. */
int bogl_init (void);
void bogl_done (void);
const char *bogl_error (void);

void bogl_line(int x1, int y1, int x2, int y2, int color);
void bogl_drawellipse(int x, int y, int rx, int ry, int color);
void bogl_fillellipse(int x, int y, int rx, int ry, int color);

void bogl_gray_scale (int make_gray);
void bogl_set_palette (int c, int nc, const unsigned char (*palette)[3]);
void bogl_rectangle (int x1, int y1, int x2, int y2, int c);
int bogl_metrics (const char *s, int n, struct bogl_font *font);

#define bogl_char_width(CH, FONT)		\
	((FONT)->width[(unsigned char) (CH)])

#define bogl_font_height(FONT)			\
	((FONT)->height)

/* Device-specific routines. */
void (*bogl_pixel) (int x, int y, int c);
int (*bogl_readpixel) (int x, int y);
void (*bogl_hline) (int x1, int x2, int y, int c);
void (*bogl_vline) (int x, int y1, int y2, int c);
void (*bogl_text) (int x, int y, const char *s, int n, int fg, int bg,
		   struct bogl_font *font);
void (*bogl_clear) (int x1, int y1, int x2, int y2, int c);
void (*bogl_move) (int sx, int sy, int dx, int dy, int w, int h);
void (*bogl_put) (int x, int y, const struct bogl_pixmap *pixmap,
		  const int color_map[16]);
void (*bogl_pointer) (int visible, int x, int y,
		      const struct bogl_pointer *,
		      int colors[2]);

#endif /* bogl_h */
