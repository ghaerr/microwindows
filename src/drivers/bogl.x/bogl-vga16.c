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

/*#define NDEBUG*/
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* VGA16 support for BOGL. */

/* bits_set[X] is the number of nonzero bits in X. */
static unsigned char bits_set[256];

/* Program the Set/Reset Register for drawing in color COLOR for write
   mode 0. */
static inline void 
set_color (int c)
{
  outb (0, 0x3ce);
  outb (c, 0x3cf);
}

/* Set the Enable Set/Reset Register. */
static inline void
set_enable_sr (int mask)
{
  outb (1, 0x3ce);
  outb (mask, 0x3cf);
}

/* Select the Bit Mask Register on the Graphics Controller. */
static inline void
select_mask (void)
{
  outb (8, 0x3ce);
}

/* Program the Bit Mask Register to affect only the pixels selected in
   MASK.  The Bit Mask Register must already have been selected with
   select_mask (). */
static inline void
set_mask (int mask)
{
  outb (mask, 0x3cf);
}

/* Set the Data Rotate Register.  Bits 0-2 are rotate count, bits 3-4
   are logical operation (0=NOP, 1=AND, 2=OR, 3=XOR). */
static inline void
set_op (int op)
{
  outb (3, 0x3ce);
  outb (op, 0x3cf);
}

/* Set the Memory Plane Write Enable register. */
static inline void
set_write_planes (int mask)
{
  outb (2, 0x3c4);
  outb (mask, 0x3c5);
}

/* Set the Read Map Select register. */
static inline void
set_read_plane (int plane)
{
  outb (4, 0x3ce);
  outb (plane, 0x3cf);
}

/* Set the Graphics Mode Register.  The write mode is in bits 0-1, the
   read mode is in bit 3. */
static inline void
set_mode (int mode)
{
  outb (5, 0x3ce);
  outb (mode, 0x3cf);
}

/* Read-modify-write the specified memory byte. */
static inline void
rmw (volatile char *p)
{
  *p |= 1;
}

/* Set pixel (X,Y) to color C. */
void
bogl_vga16_pixel (int x, int y, int c)
{
    bogl_drawing = 1;

    assert (x >= 0 && x < bogl_xres);
    assert (y >= 0 && y < bogl_yres);
    assert (c >= 0 && c < bogl_ncols);
  
    set_color (c);
    select_mask ();
    set_mask (0x80 >> (x % 8));
    rmw (bogl_frame + x / 8 + y * bogl_line_len);

    bogl_drawing = 0;
}

/* Return 4-bit pixel value at x,y*/
int
bogl_vga16_readpixel(int x,int y)
{
	volatile char *	src;
	int		plane;
	int		c = 0;
	
	assert (x >= 0 && x < bogl_xres);
	assert (y >= 0 && y < bogl_yres);
  
	bogl_drawing = 1;
	src = bogl_frame + x / 8 + y * bogl_line_len;
	for(plane=0; plane<4; ++plane) {
		set_read_plane(plane);
		if(*src & (0x80 >> (x % 8)))
			c |= 1 << plane;
	}
	bogl_drawing = 0;
	return c;
}


/* Paint a horizontal line from (X1,Y) to (X2,Y) in color C, where X2
   >= X1.  The final point is not painted. */
void
bogl_vga16_hline (int x1, int x2, int y, int c)
{
  volatile char *dst;


  if (x1 == x2)
    return;

  assert (x1 >= 0 && x1 < bogl_xres);
  assert (x2 >= 0 && x2 <= bogl_xres);
  assert (x2 >= x1);
  assert (y >= 0 && y < bogl_yres);
  assert (c >= 0 && c < bogl_ncols);

  bogl_drawing = 1;

  set_color (c);
  dst = bogl_frame + x1 / 8 + y * bogl_line_len;
  select_mask ();
  if (x1 / 8 == x2 / 8) 
    {
      set_mask ((0xff >> (x1 % 8)) & (0xff << (7 - x2 % 8)));
      rmw (dst);
    }
  else
    {
      volatile char *last;
      
      set_mask (0xff >> (x1 % 8));
      rmw (dst++);

      set_mask (0xff);
      last = bogl_frame + x2 / 8 + y * bogl_line_len;
      while (dst < last)
	*dst++ = 1;
      
      set_mask (0xff << (7 - x2 % 8));
      rmw (dst);
    }

  bogl_drawing = 0;
}

/* Paints a vertical line from (X,Y1) to (X,Y2) in color C.  The final
   point is not painted. */
void
bogl_vga16_vline (int x, int y1, int y2, int c)
{
  volatile char *dst, *last;

  y2--;
  
  assert (x >= 0 && x < bogl_xres);
  assert (y1 >= 0 && y1 < bogl_yres);
  assert (y2 >= 0 && y2 <= bogl_yres);
  assert (y2 >= y1);
  assert (c >= 0 && c < bogl_ncols);

  bogl_drawing = 1;

  set_color (c);
  select_mask ();
  set_mask (0x80 >> (x % 8));
  dst = bogl_frame + x / 8 + y1 * bogl_line_len;
  last = bogl_frame + x / 8 + y2 * bogl_line_len;
  while (dst <= last)
    {
      rmw (dst);
      dst += bogl_line_len;
    }

  bogl_drawing = 0;
}

#if !SMALLBOGL

/* Clear the region from (X1,Y1) to (X2,Y2) to color C, not including
   the last row or column.  If C == -1 then the region's colors are
   inverted rather than set to a particular color.  */
void
bogl_vga16_clear (int x1, int y1, int x2, int y2, int c)
{
  volatile char *dst;


  if (x1 == x2)
    return;
  x2--;

  assert (x1 >= 0 && x1 < bogl_xres);
  assert (x2 >= 0 && x2 <= bogl_xres);
  assert (x2 >= x1);
  assert (y1 >= 0 && y1 < bogl_yres);
  assert (y2 >= 0 && y2 <= bogl_yres);
  assert (y2 >= y1);
  assert (c >= -1 && c < bogl_ncols);

  bogl_drawing = 1;

  set_color (c);
  if (c == -1)
    set_op (0x18);

  select_mask ();
  if (x1 / 8 == x2 / 8)
    {
      volatile char *last;

      dst = bogl_frame + x1 / 8 + y1 * bogl_line_len;
      last = bogl_frame + x2 / 8 + y2 * bogl_line_len;
      set_mask ((0xff >> (x1 % 8)) & (0xff << (7 - x2 % 8)));
      while (dst < last)
	{
	  rmw (dst);
	  dst += bogl_line_len;
	}
    }
  else 
    {
      /* FIXME: the following code could admittedly be more efficient,
	 but my first attempt at optimization was buggy. */
      int y;
      
      for (y = y1; y < y2; y++)
	{
	  volatile char *last;
	  dst = bogl_frame + x1 / 8 + y * bogl_line_len;
	  set_mask (0xff >> (x1 % 8));
	  rmw (dst++);

	  set_mask (0xff);
	  last = bogl_frame + x2 / 8 + y * bogl_line_len;
	  while (dst < last)
	    *dst++ = 1;
      
	  set_mask (0xff << (7 - x2 % 8));
	  rmw (dst);
	}
    }

  set_op (0);
  bogl_drawing = 0;
}

/* FIXME: it would be faster to use write mode 3 to write the middle
   bytes (but also more complex). */
#define ul_size (sizeof (unsigned long))
#define ul_bits (CHAR_BIT * ul_size)
void
bogl_vga16_text (int xx, int yy, const char *s, int n, int fg, int bg,
		 struct bogl_font *font)
{
  /* Font height, or possibly less due to clipping. */
  int h;
  int x, y;
  unsigned long bits[font->height];

  void plot (void)
    {
      volatile char *dst = bogl_frame + xx / 8 + yy * bogl_line_len;
      int y, i;
      
      for (y = 0; y < h; y++)
	{
	  unsigned long b = bits[y];
	  
	  for (i = ul_size - 1; i >= 0; i--)
	    {
	      set_mask (b);
	      rmw (dst + i);
	      b >>= 8;
	    }
	  
	  dst += bogl_line_len;
	}
    }

  assert (xx >= 0 && xx < bogl_xres);
  assert (yy >= 0 && yy < bogl_yres);
  assert (fg >= 0 && fg < bogl_ncols);
  assert (bg >= -1 && bg < bogl_ncols);

  h = font->height;
  if (yy + h > bogl_yres)
    h = bogl_yres - yy;

  if (bg != -1)
    {
      int x2 = xx + bogl_metrics (s, n, font);
      if (x2 >= bogl_xres)
	x2 = bogl_xres - 1;
      
      bogl_vga16_clear (xx, yy, x2, yy + h, bg);
    }
  
  bogl_drawing = 1;

  for (y = 0; y < h; y++)
    bits[y] = 0;
  
  set_color (fg);
  select_mask ();

  x = xx % ul_bits;
  xx = xx / ul_bits * ul_bits;

  for (; n--; s++)
    {
      const unsigned char ch = *s;
      const unsigned long *character = &font->content[font->offset[ch]];
      const int width = font->width[ch];

      for (y = 0; y < h; y++)
	bits[y] |= character[y] >> x;
      x += width;

      if (x >= (int) ul_bits)
	{
	  plot ();

	  x -= ul_bits;
	  for (y = 0; y < h; y++)
	    bits[y] = character[y] << (width - x);

	  xx += ul_bits;
	  if (xx >= bogl_xres)
	    goto done;
	}
    }
  plot ();

 done:
  bogl_drawing = 0;
}

/* Write PIXMAP at location (XX,YY), with the pixmap's colors mapped
   according to COLOR_MAP. */
void
bogl_vga16_put (int xx, int yy, const struct bogl_pixmap *pixmap,
		const int color_map[16])
{
  volatile char *dst;
  const unsigned char *src;
  int x, y;

  assert (xx >= 0 && xx < bogl_xres);
  assert (xx + pixmap->width <= bogl_xres);
  assert (yy >= 0 && yy < bogl_yres);
  assert (yy + pixmap->width <= bogl_yres);
  src = pixmap->data;
  
  bogl_drawing = 1;

  y = yy;
  while (y < yy + pixmap->height)
    {
      x = xx;
      dst = bogl_frame + x / 8 + y * bogl_line_len;

      while (x < xx + pixmap->width) 
	{
	  int color = *src & 0xf;
	  int count = *src >> 4;
	  src++;
	  
	  if (color == pixmap->transparent)
	    {
	      dst += (x + count) / 8 - x / 8;
	      x += count;
	      continue;
	    }

	  set_color (color_map[color]);
	  select_mask ();

	  if (count == 1)
	    {
	      set_mask (0x80 >> (x % 8));
	      *dst |= 1;
	      if (++x % 8 == 0)
		dst++;
	      continue;
	    }

	  for (;;)
	    {
	      /* Get a mask for COUNT bits starting at X, or at least
                 as many as will fit in one byte. */
	      unsigned mask;
	      mask = (0xffff00 >> count) & 0xff;
	      mask >>= x % 8;
	      set_mask (mask);

	      /* Write the bits and bow out if we don't need to
		 advance to the next byte. */
	      *dst |= 1;
	      if ((mask & 1) == 0)
		{
		  x += count;
		  break;
		}

	      /* Advance to the next byte. */
	      dst++;
	      x += bits_set[mask];
	      count -= bits_set[mask];
	      if (!count)
		break;
	    }
	}
      
      y++;
    }

  bogl_drawing = 0;
}

/* Draw mouse pointer POINTER with its hotspot at (X,Y), if VISIBLE !=
   0.  Restores the previously saved background at that point, if
   VISIBLE == 0.  COLORS[] gives the color indices to paint the
   cursor.

   This routine performs full clipping on all sides of the screen. */
void 
bogl_vga16_pointer (int visible, int x1, int y1,
		    const struct bogl_pointer *pointer,
		    int colors[2])
{
  int y_count;		/* Number of scanlines. */
  int y_ofs;		/* Number of scanlines to skip drawing. */
  int x_ofs;		/* Number of pixels to skip drawing on each line. */

  assert (pointer != NULL);

  x1 -= pointer->hx;
  y1 -= pointer->hy;
  
  if (y1 + 16 > bogl_yres)
    y_count = bogl_yres - y1;
  else
    y_count = 16;

  if (x1 < 0)
    {
      x_ofs = -x1;
      x1 = 0;
    }
  else
    x_ofs = 0;

  if (y1 < 0)
    {
      y_ofs = -y1;
      y1 = 0;
      y_count -= y_ofs;
    }
  else
    y_ofs = 0;

  bogl_drawing = 1;

  /* Save or restore the framebuffer contents. */
  {
    /* Four planes of sixteen rows of four bytes each. */
    static unsigned char saved[4 * 16 * 3];

    int plane;		/* Current plane. */
    int sx_ofs;		/* Byte offset within a scanline to save/restore. */
      
    sx_ofs = x1 / 8;
    if (sx_ofs + 3 > bogl_line_len)
      sx_ofs = bogl_line_len - 3;

    if (visible)
      {
	for (plane = 0; plane < 4; plane++)
	  {
	    volatile char *dst = saved + plane * 16 * 3;
	    volatile char *src = bogl_frame + sx_ofs + y1 * bogl_line_len;
	    int y = y_count;

	    set_read_plane (plane);
	    
	    while (y--)
	      {
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		
		src += bogl_line_len - 3;
	      }
	  }
      }
    else
      {
	set_enable_sr (0);
	select_mask ();
	set_mask (0xff);

	for (plane = 0; plane < 4; plane++)
	  {
	    volatile char *dst = bogl_frame + sx_ofs + y1 * bogl_line_len;
	    volatile char *src = saved + plane * 16 * 3;
	    int y = y_count;
	  
	    set_write_planes (1 << plane);

	    while (y--)
	      {
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;

		dst += bogl_line_len - 3;
	      }
	  }
	
	set_write_planes (0xf);
	set_enable_sr (0xf);
      }
  }

  if (visible)
    {
      const unsigned short *mask_p, *color_p;
      int x_count;
      int y;
      int color;
      
      x_count = x1 % 8 ? 3 : 2;
      if (x1 / 8 + x_count > bogl_line_len)
	x_count = bogl_line_len - x1 / 8;

      for (color = 0; color < 2; color++)
	{
	  set_color (colors[color]);
	  select_mask ();
	  
	  mask_p = pointer->mask + y_ofs;
	  color_p = pointer->color + y_ofs;
	  for (y = 0; y < y_count; y++, mask_p++, color_p++)
	    {
	      volatile char *dst;
	      unsigned long bits;
	      int x;

	      dst = bogl_frame + x1 / 8 + (y1 + y) * bogl_line_len;

	      if (color)
		bits = *mask_p ^ *color_p;
	      else
		bits = *mask_p & *color_p;
	      bits <<= (CHAR_BIT * (sizeof (long) - sizeof (short))) + x_ofs;
	      bits >>= x1 % 8;

	      x = x_count;
	      while (x--)
		{
		  set_mask (bits >> 24);
		  rmw (dst++);
		  bits <<= 8;
		}
	    }
	}
    }

  bogl_drawing = 0;
}
#endif /* !SMALLBOGL*/

/* Initialize the VGA controller.  Returns the number of bytes to
   memory map on success, or zero on failure. */
size_t
bogl_vga16_init (void)
{
  if (-1 == ioperm (0x3c0, 0x20, 1))
    {
      bogl_fail ("can't get IO permissions: %s", strerror(errno));
      return 0;
    }

  /* Set up some default values for the VGA Graphics Registers. */
  set_enable_sr (0xf);
  set_op (0);
  set_mode (0);

  /* Initialize bits_set array. */
  {
    int i;

    for (i = 0; i < 256; i++)
      {
	int c, j;

	for (c = j = 0; j < 8; j++)
	  c += (i & (1 << j)) != 0;

	bits_set[i] = c;
      }
  }
  
  return 0x10000;
}
