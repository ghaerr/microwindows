/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2004 Bernard Leach <leachbj@bouncycastle.org>
 *
 * 2bpp Packed Linear Video Driver (reversed bit order)
 * For iPod, derived from fblin2rev.c For Psion S5
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */

#include <assert.h>
#include <string.h>
#include "device.h"
#include "fb.h"

static void lcd_update_display(PSD psd, int sx, int sy, int mx, int my);

#define inl(p) (*(volatile unsigned long *) (p))
#define outl(v,p) (*(volatile unsigned long *) (p) = (v))

static const unsigned char notmask[4] = { 0xfc, 0xf3, 0xcf, 0x3f };

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear2_init(PSD psd)
{
	if (!psd->size)
		psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

/* Set pixel at x, y, to pixelval c*/
static void
linear2_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x>>2) + y * psd->linelen;
	if(gr_mode == MWROP_XOR)
		*addr ^= c << ((x&3)<<1);
	else
		*addr = (*addr & notmask[x&3]) | (c << ((x&3)<<1));
	lcd_update_display(psd, x, y, x, y);
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear2_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[(x>>2) + y * psd->linelen] >> ((x&3)<<1) ) & 0x03;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear2_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	x1orig = x1;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x1>>2) + y * psd->linelen;
	if(gr_mode == MWROP_XOR) {
		while(x1 <= x2) {
			*addr ^= c << ((x1&3)<<1);
			if((++x1 & 3) == 0)
				++addr;
		}
	} else {
		while(x1 <= x2) {
			*addr = (*addr & notmask[x1&3]) | (c << ((x1&3)<<1));
			if((++x1 & 3) == 0)
				++addr;
		}
	}
	lcd_update_display(psd, x1orig, y, x2, y);
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear2_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen;
	int	y1orig = y1;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x>>2) + y1 * linelen;
	if(gr_mode == MWROP_XOR)
		while(y1++ <= y2) {
			*addr ^= c << ((x&3)<<1);
			addr += linelen;
		}
	else
		while(y1++ <= y2) {
			*addr = (*addr & notmask[x&3]) | (c << ((x&3)<<1));
			addr += linelen;
		}
	lcd_update_display(psd, x, y1orig, x, y2);
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
static void
linear2_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op)
{
	ADDR8	dst;
	ADDR8	src;
	int	i;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
	int	horig = h;

	assert (dstpsd->addr != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (srcpsd->addr != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	dst = ((ADDR8)dstpsd->addr) + (dstx>>2) + dsty * dlinelen;
	src = ((ADDR8)srcpsd->addr) + (srcx>>2) + srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;
		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&3]) |
			   ((*s >> ((sx&3)<<1) & 0x03) << ((dx&3)<<1));
			if((++dx & 3) == 0)
				++d;
			if((++sx & 3) == 0)
				++s;
		}
		dst += dlinelen;
		src += slinelen;
	}
	lcd_update_display(dstpsd, dstx, dsty, dstx+w, dsty+horig);
	DRAWOFF;
}

SUBDRIVER fblinear2 = {
	linear2_init,
	linear2_drawpixel,
	linear2_readpixel,
	linear2_drawhorzline,
	linear2_drawvertline,
	gen_fillrect,
	linear2_blit
};

#define IPOD_LCD_BASE	0xc0001000
#define IPOD_RTC	0xcf001110
#define IPOD_LCD_WIDTH	160
#define IPOD_LCD_HEIGHT	128


/* get current usec counter */
static int
timer_get_current(void)
{
	return inl(IPOD_RTC);
}

/* check if number of useconds has past */
static int
timer_check(int clock_start, int usecs)
{
	if ( (inl(IPOD_RTC) - clock_start) >= usecs ) {
		return 1;
	} else {
		return 0;
	}
}

/* wait for LCD with timeout */
static void
lcd_wait_write(void)
{
	if ( (inl(IPOD_LCD_BASE) & 0x1) != 0 ) {
		int start = timer_get_current();

		do {
			if ( (inl(IPOD_LCD_BASE) & (unsigned int)0x8000) == 0 ) break;
		} while ( timer_check(start, 1000) == 0 );
	}
}


/* send LCD data */
static void
lcd_send_data(int data_lo, int data_hi)
{
	lcd_wait_write();

	outl(data_lo, 0xc0001010);
	lcd_wait_write();
	outl(data_hi, 0xc0001010);
}

/* send LCD command */
static void
lcd_prepare_cmd(int cmd)
{
	lcd_wait_write();
	outl(0x0, 0xc0001008);
	lcd_wait_write();
	outl(cmd, 0xc0001008);
}

/* send LCD command and data */
static void
lcd_cmd_and_data(int cmd, int data_lo, int data_hi)
{
	lcd_prepare_cmd(cmd);

	lcd_send_data(data_lo, data_hi);
}

static void lcd_update_display(PSD psd, int sx, int sy, int mx, int my)
{
	int cursor_pos;
	int y;

	ADDR8	addr = psd->addr;

	/* only update the ipod if we are writing to the screen */
	if (!(psd->flags & PSF_SCREEN)) return;

	assert (addr != 0);

	sx >>= 3;
	//mx = (mx+7)>>3;
	mx >>= 3;

	cursor_pos = sx + (sy << 5);

	for ( y = sy; y <= my; y++ ) {
		ADDR8 img_data;
		int x;

		// move the cursor
		lcd_cmd_and_data(0x11, cursor_pos >> 8, cursor_pos & 0xff);

		// setup for printing
		lcd_prepare_cmd(0x12);

		img_data = addr + (sx<<1) + (y * psd->linelen);

		// 160/8 -> 20 == loops 20 times
		// make sure we loop at least once
		for ( x = sx; x <= mx; x++ ) {
			// display a character
			lcd_send_data(*(img_data+1), *img_data);

			img_data += 2;
		}

		// update cursor pos counter
		cursor_pos += 0x20;
	}
}

