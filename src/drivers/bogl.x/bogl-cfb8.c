/* BOGL - Ben's Own Graphics Library.
   Written by Ben Pfaff <pfaffben@debian.org>.

   Portions Copyright (c) 1999 Greg Haerr <greg@censoft.com>

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

/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "bogl.h"
#include "boglP.h"
#include "bogl-cfb8.h"

extern int gr_mode;		/* temp global kluge*/

/* Set pixel (X,Y) to color C. */
void
bogl_cfb8_pixel (int x, int y, int c)
{
    assert (x >= 0 && x < bogl_xres);
    assert (y >= 0 && y < bogl_yres);
    assert (c >= 0 && c < bogl_ncols);
  
    bogl_drawing = 1;
    if(gr_mode == 0)
    	bogl_frame[x + y * bogl_line_len] = c;
    else
    	bogl_frame[x + y * bogl_line_len] ^= c;
    bogl_drawing = 0;
}

/* Read pixel at (X,Y). */
int
bogl_cfb8_readpixel (int x, int y)
{
    assert (x >= 0 && x < bogl_xres);
    assert (y >= 0 && y < bogl_yres);
  
    return bogl_frame[x + y * bogl_line_len];
}

/* Paint a horizontal line from (X1,Y) to (X2,Y) in color C, where X2
   >= X1.  The final point is not painted. */
void
bogl_cfb8_hline (int x1, int x2, int y, int c)
{
  char *dst;
  int cnt;

  assert (x1 >= 0 && x1 < bogl_xres);
  assert (x2 >= 0 && x2 <= bogl_xres);
  assert (x2 >= x1);
  assert (y >= 0 && y < bogl_yres);
  assert (c >= 0 && c < bogl_ncols);

  bogl_drawing = 1;
  dst = bogl_frame + x1 + y * bogl_line_len;
  if(gr_mode == 0)
  	memset (dst, c, x2 - x1);
  else {
	  cnt = x2 - x1;
	  while(--cnt >= 0)
	    *dst++ ^= c;
  }
  bogl_drawing = 0;
}

/* Paints a vertical line from (X,Y1) to (X,Y2) in color C.  The final
   point is not painted. */
void
bogl_cfb8_vline (int x, int y1, int y2, int c)
{
  assert (x >= 0 && x < bogl_xres);
  assert (y1 >= 0 && y1 < bogl_yres);
  assert (y2 >= 0 && y2 <= bogl_yres);
  assert (y2 >= y1);
  assert (c >= 0 && c < bogl_ncols);

  bogl_drawing = 1;
  if(gr_mode == 0) {
	  for (; y1 < y2; y1++)
	    bogl_frame[x + y1 * bogl_line_len] = c;
  } else {
	  for (; y1 < y2; y1++)
	    bogl_frame[x + y1 * bogl_line_len] ^= c;
  }
  bogl_drawing = 0;
}

void
bogl_cfb8_blit(int destx, int desty, int w, int h, int srcx, int srcy)
{
  char *	dst;
  char *	src;
  
  assert (destx >= 0 && destx < bogl_xres);
  assert (desty >= 0 && desty < bogl_yres);
  assert (w > 0);
  assert (h > 0);
  assert (srcx >= 0 && srcx <= bogl_xres);
  assert (srcy >= 0 && srcy < bogl_yres);

  bogl_drawing = 1;
  dst = bogl_frame + destx + desty * bogl_line_len;
  src = bogl_frame + srcx + srcy * bogl_line_len;
  while(--h >= 0) {
      memcpy (dst, src, w);
      dst += bogl_line_len;
      src += bogl_line_len;
  }
  bogl_drawing = 0;
}

#if !SMALLBOGL
/* Clear the region from (X1,Y1) to (X2,Y2) to color C, not including
   the last row or column.  If C == -1 then the region's colors are
   inverted rather than set to a particular color.  */
void
bogl_cfb8_clear (int x1, int y1, int x2, int y2, int c)
{
  char *dst;
  int cnt;
  
  if (x1 == x2 || y1 == y2)
    return;

  assert (x1 >= 0 && x1 < bogl_xres);
  assert (x2 >= 0 && x2 <= bogl_xres);
  assert (x2 >= x1);
  assert (y1 >= 0 && y1 < bogl_yres);
  assert (y2 >= 0 && y2 <= bogl_yres);
  assert (y2 >= y1);
  assert (c >= -1 && c < bogl_ncols);

  bogl_drawing = 1;
  dst = bogl_frame + x1 + y1 * bogl_line_len;
  if(gr_mode == 0)
	  for (; y1 < y2; y1++) {
	      memset (dst, c, x2 - x1);
	      dst += bogl_line_len;
	  }
  else
	  for (; y1 < y2; y1++) {
	      cnt = x2 - x1;
	      while(--cnt >= 0)
		  *dst++ ^= c;
	      dst = bogl_frame + x1 + y1 * bogl_line_len;
	  }
  bogl_drawing = 0;
}

void
bogl_cfb8_text (int xx, int yy, const char *s, int n, int fg, int bg,
		struct bogl_font *font)
{
  int h;
  
  h = font->height;
yy -= h;  /* bugfix adjust text y origin, must be done before assert*/

  assert (xx >= 0 && xx < bogl_xres);
  assert (yy >= 0 && yy < bogl_yres);
  assert (fg >= 0 && fg < bogl_ncols);
  assert (bg >= -1 && bg < bogl_ncols);

  bogl_drawing = 1;

  if (yy + h > bogl_yres)
    h = bogl_yres - yy;

  for (; n--; s++)
    {
      volatile char *dst = bogl_frame + xx + yy * bogl_line_len;

      const unsigned char ch = *s;
      const unsigned long *character = &font->content[font->offset[ch]];
      int w = font->width[ch];

      int x, y;

      if (xx + w > bogl_xres)
	w = bogl_xres - xx;
      
      for (y = 0; y < h; y++)
	{
	  unsigned long c = *character++;
	  
	  for (x = 0; x < w; x++)
	    {
	      if (c & 0x80000000)
		dst[x] = fg;
	      else if (bg != -1)
		dst[x] = bg;

	      c <<= 1;
	    }

	  dst += bogl_line_len;
	}

      xx += w;
      if (xx >= bogl_xres)
	break;
    }

  bogl_drawing = 0;
}

/* Write PIXMAP at location (XX,YY), with the pixmap's colors mapped
   according to COLOR_MAP. */
void
bogl_cfb8_put (int xx, int yy, const struct bogl_pixmap *pixmap,
	       const int color_map[16])
{
  volatile char *dst;
  const unsigned char *src;
  int h;
  
  assert (xx + pixmap->width <= bogl_xres);
  assert (yy >= 0 && yy < bogl_yres);
  assert (yy + pixmap->width <= bogl_yres);
  src = pixmap->data;

  bogl_drawing = 1;

  h = pixmap->height;
  dst = bogl_frame + xx + yy * bogl_line_len;
  while (h--)
    {
      int w = pixmap->width;
      while (w)
	{
	  int color = *src & 0xf;
	  int count = *src >> 4;
	  src++;

	  w -= count;
	  while (count--)
	    *dst++ = color_map[color];
	}

      dst += bogl_line_len - pixmap->width;
    }

  bogl_drawing = 0;
}

/* Draw mouse pointer POINTER with its hotspot at (X,Y), if VISIBLE !=
   0. Restores the previously saved background at that point, if
   VISIBLE == 0. COLORS[] gives the color indices to paint the cursor.

   This routine performs full clipping on all sides of the screen. */
void bogl_cfb8_pointer(int visible, int x1, int y1,
			const struct bogl_pointer *pointer, int colors[2])
{
	volatile unsigned char *dst, *src;
	int h, w;
	static unsigned char saved[16 * 16];
	static int oldx, oldy, oldxl, oldyl;
	unsigned const short *clr, *msk;
	unsigned short c, m;
	int xl, yl;

	assert(pointer != NULL);

	/* Find edge of bitmap from hotspot */
	x1 += pointer->hx;
	y1 += pointer->hy;

	/* Clip to right hand side of screen: */
	if(x1 >= bogl_line_len - 16) {
		if(x1 >= (bogl_line_len - 3))
			x1 = bogl_line_len - 3;
		/* Calculate overlap: */
		xl = bogl_line_len - x1 - 1;
	} else xl = 16;

	/* Clip to bottom of screen: */
	if(y1 >= bogl_yres - 16) {
		if(y1 >= bogl_yres)
			y1 = bogl_yres;
		/* Calculate overlap: */
		yl = bogl_yres - y1;
	} else yl = 16;

	bogl_drawing = 1;

	if(visible) {
		/* Save the screen under the pointer: */
		dst = &saved[0];
		src = bogl_frame + x1 + xl + y1 * bogl_line_len;

		for(h = yl; h; h--) {
			for(w = xl; w; w--) {
				*dst = *src;
				dst++;
				src--;
			}
			dst += 16 - xl;
			src += bogl_line_len + xl;
		}

		/* Save the current position, so we can restore it later: */
		oldx = x1;
		oldy = y1;
		oldxl = xl;
		oldyl = yl;

		/* Draw the new pointer: */
		dst = bogl_frame + x1 + xl + y1 * bogl_line_len;
		clr = pointer->color;
		msk = pointer->mask;

		for(h = yl; h; h--) {
			c = *clr;
			m = *msk;
			c >>= (16 - xl);
			m >>= (16 - xl);
			for(w = xl; w; w--) {
				if(m & 1) *dst = ((c & 1) ? 0 : 15);
				dst--;
				c >>= 1;
				m >>= 1;
			}
			clr++;
			msk++;
			dst += bogl_line_len + xl;
		}
	} else {
		/* Restore the data under the pointer: */
		dst = bogl_frame + oldx + oldxl + oldy * bogl_line_len;
		src = &saved[0];

		for(h = oldyl; h; h--) {
			for(w = oldxl; w; w--) {
				*dst = *src;
				dst--;
				src++;
			}
			src += 16 - oldxl;
			dst += bogl_line_len + oldxl;
		}
	}

	bogl_drawing = 0;
}
#endif /* !SMALLBOGL*/

/* Initialize CFB8 mode.  Returns the number of bytes to mmap for the
   framebuffer. */
size_t
bogl_cfb8_init (void)
{
  return bogl_line_len * bogl_yres;
}
