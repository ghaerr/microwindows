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

#ifndef bogl_vga16_h
#define bogl_vga16_h

#include <stddef.h>

size_t bogl_vga16_init ();

void bogl_vga16_pixel (int x, int y, int c);
int bogl_vga16_readpixel (int x, int y);
void bogl_vga16_hline (int x1, int x2, int y, int c);
void bogl_vga16_vline (int x, int y1, int y2, int c);
void bogl_vga16_text (int x, int y, const char *s, int n, int fg, int bg,
		      struct bogl_font *font);
void bogl_vga16_clear (int x1, int y1, int x2, int y2, int c);
void bogl_vga16_move (int sx, int sy, int dx, int dy, int w, int h);
void bogl_vga16_put (int x, int y, const struct bogl_pixmap *pixmap,
		     const int color_map[16]);
void bogl_vga16_pointer (int visible, int x1, int y1,
			 const struct bogl_pointer *pointer,
			 int colors[2]);

#endif /* bogl_vga16_h */
