/* by julian schroeder <detemp1@germany.cirrus.com>
 * for Cirrus Logic  based on fblin24.c
 *
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * 12bpp Linear Video Driver for Microwindows
 *
 * UNDER CONSTRUCTION
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "fb.h"

int gr_mode=MWMODE_XOR;

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear12_init(PSD psd)
{
	if (!psd->size) {
		psd->size = 960*240/2;
		/* convert linelen from byte to pixel len for bpp 16, 24, 32*/
		psd->linelen *= 2;
		psd->linelen /= 3; /* /1.5*/
	}
	return 1;
}

static inline void setpix(char *cptr,int x, int y, char c)
{
long adr;

adr=(x>>1) + (y*480);  /* change, julian*/

if(gr_mode == MWMODE_XOR) 
  {
  if(x & 0x01) cptr[adr]^=((c << 4) & 0xf0);
  else cptr[adr]^=(c & 0x0f);
  }    
else 
  {
  if(x & 0x01) 
    {
    cptr[adr]&=0x0f;
    cptr[adr]|=((c << 4) & 0xf0);
    }
  else
    { 
    cptr[adr]&=0xf0;
    cptr[adr]|=(c & 0x0f);
    }
  }
}

static inline char getpix(char *cptr,int x, int y)
{
long adr;
adr=(x>>1) + (y*480);  /* change, julian*/

if(x & 0x01) return (cptr[adr] >> 4) & 0x0f;
return cptr[adr] & 0x0f;
}


/* Set pixel at x, y, to pixelval c*/
static void
linear12_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	MWUCHAR	r, g, b;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	r = PIXEL444RED(c);
	g = PIXEL444GREEN(c);
	b = PIXEL444BLUE(c);
        x=x+(x<<1);
	DRAWON;
        setpix(addr,x,y,r);
	setpix(addr,x+1,y,g);
	setpix(addr,x+2,y,b);
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear12_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
        x=x+(x<<1);
	return RGB2PIXEL444(getpix(addr,x,y),getpix(addr,x+1,y),getpix(addr,x+2,y));
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear12_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	MWUCHAR	r, g, b;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	r = PIXEL444RED(c);
	g = PIXEL444GREEN(c);
	b = PIXEL444BLUE(c);
	DRAWON;
	        x1*=3;
		x2*=3;
		while((x1+=3) <= x2) {
		setpix(addr,x1,y,r);
		setpix(addr,x1+1,y,g);
		setpix(addr,x1+2,y,b);
		}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear12_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen * 3;
	MWUCHAR	r, g, b;
        linelen/=2;
	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);
	x=x+(x<<1);
	r = PIXEL444RED(c);
	g = PIXEL444GREEN(c);
	b = PIXEL444BLUE(c);
        
	DRAWON;
		while(y1++ <= y2) 
		{
	        setpix(addr,x,y1,r);
	        setpix(addr,x+1,y1,g);
	        setpix(addr,x+2,y1,b);
		}		
	DRAWOFF;
}

#if 0
/* srccopy bitblt, opcode is currently ignored*/
static void
xlinear12_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR8	dst = dstpsd->addr;
	ADDR8	src = srcpsd->addr;
	int	i;
	int	dlinelen = dstpsd->linelen * 3;
	int	slinelen = srcpsd->linelen * 3;
	int	dlinelen_minus_w = (dstpsd->linelen - w) * 3;
	int	slinelen_minus_w = (srcpsd->linelen - w) * 3;
#if ALPHABLEND
	unsigned int alpha;
#endif

	assert (dst != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (src != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);

	DRAWON;
	dst += (dstx + dsty * dstpsd->linelen) * 3;
	src += (srcx + srcy * srcpsd->linelen) * 3;

#if ALPHABLEND
	if((op & MWROP_EXTENSION) != MWROP_BLENDCONSTANT)
		goto stdblit;
	alpha = op & 0xff;

	while(--h >= 0) {
		for(i=0; i<w; ++i) {
			unsigned long s = *src++;
			unsigned long d = *dst;
			*dst++ = (unsigned char)(((s - d)*alpha)>>8) + d;
			s = *src++;
			d = *dst;
			*dst++ = (unsigned char)(((s - d)*alpha)>>8) + d;
			s = *src++;
			d = *dst;
			*dst++ = (unsigned char)(((s - d)*alpha)>>8) + d;
		}
		dst += dlinelen_minus_w;
		src += slinelen_minus_w;
	}
	DRAWOFF;
	return;
stdblit:
#endif
	while(--h >= 0) {
#if 1
		/* a _fast_ memcpy is a _must_ in this routine*/
		memcpy(dst, src, w*3);
		dst += dlinelen;
		src += slinelen;
#else
		for(i=0; i<w; ++i) {
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
		}
		dst += dlinelen_minus_w;
		src += slinelen_minus_w;
#endif
	}
	DRAWOFF;
}
#endif

/* srccopy bitblt, opcode is currently ignored*/
static void
linear12_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR8	dst = dstpsd->addr;
	ADDR8	src = srcpsd->addr;

	/*if ((srcx & 0x01) || (dstx & 0x01))*/
	/* FIXME where is this if supposed to end?? */

	DRAWON;
        dst+=((dstx*3+1)/2)+480*dsty;
        src+=((srcx*3+1)/2)+480*srcx;

	assert (dst != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (w > 0);
	assert (h > 0);
	assert (src != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (dstx+w <= dstpsd->xres);
	assert (dsty+h <= dstpsd->yres);
	assert (srcx+w <= srcpsd->xres);
	assert (srcy+h <= srcpsd->yres);


        if((srcx & 0x01) && !(dstx & 0x01))
	  {
	  src+=1;
	  /*w-=1;*/
          }

        if(!(srcx & 0x01) && (dstx & 0x01))
	  {
	  dst+=1;
	  /*w-=1;*/
          }

        if((srcx & 0x01) && (dstx & 0x01))
	  {
	  /*w-=1;*/
          }


	while(--h >= 0) {
		/* a _fast_ memcpy is a _must_ in this routine*/
		memcpy(dst, src, (w*3+1)/2);
		dst += 480;
		src += 480;
	}
	DRAWOFF;
}


SUBDRIVER fblinear12 = {
	linear12_init,
	linear12_drawpixel,
	linear12_readpixel,
	linear12_drawhorzline,
	linear12_drawvertline,
	gen_fillrect,
	linear12_blit
};
