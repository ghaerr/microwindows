/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * 4bpp Packed Linear Video Driver for Microwindows
 * 	This driver is written for the Vr41xx Palm PC machines
 * 	Hopefully, we can get the 4bpp mode running 320x240x16
 *
 * 	In this driver, psd->linelen is line byte length, not line pixel length
 */
/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include "device.h"
#include "fb.h"

//static unsigned char notmask[2] = { 0x0f, 0xf0};  // non-linear 
static unsigned char notmask[2] = { 0xf0, 0x0f };   // linear
static unsigned char revnotmask[2] = { 0xf0, 0x0f};

/* Calc linelen and mmap size, return 0 on fail*/
static int
linear4_init(PSD psd)
{
	if (!psd->size)
		psd->size = psd->yres * psd->linelen;
	/* linelen in bytes for bpp 1, 2, 4, 8 so no change*/
	return 1;
}

#if 1  // kykim
/* Read pixel at x, y*/
static MWPIXELVAL
linear4_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[(x>>1) + y * psd->linelen] >> (((x&1))<<2) ) & 0x0f;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear4_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x>>1) + y1 * linelen;
	while(y1++ <= y2) {
		*addr = (*addr & notmask[x&1]) | (c << (((x&1))<<2));
		addr += linelen;
	}
	DRAWOFF;
}
#endif  // kykim
/* ########################################################################## */
#if 0  // For 8 bit memory access
/* Set pixel at x, y, to pixelval c*/
static void
linear4_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x>>1) + y * psd->linelen;
//	if(gr_mode == MWMODE_XOR)
//		*addr ^= c << ((1-(x&1))<<2);
//	else
	*addr = (*addr & notmask[x&1]) | (c << (((x&1))<<2) );
	DRAWOFF;
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear4_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);

	return (addr[(x>>1) + y * psd->linelen] >> (((x&1))<<2) ) & 0x0f;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear4_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;

	assert (addr != 0);
	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x1>>1) + y * psd->linelen;
/*	if(gr_mode == MWMODE_XOR) {
		while(x1 <= x2) {
			*addr ^= c << ((1-(x1&1))<<2);
			if((++x1 & 1) == 0)
				++addr;
		}
	} else {  
*/
	while(x1 <= x2) {
		*addr = (*addr & notmask[x1&1]) | (c << (((x1&1))<<2));
		if((++x1 & 1) == 0)
			++addr;
	}
//	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear4_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	ADDR8	addr = psd->addr;
	int	linelen = psd->linelen;

	assert (addr != 0);
	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	addr += (x>>1) + y1 * linelen;
	while(y1++ <= y2) {
		*addr = (*addr & notmask[x&1]) | (c << (((x&1))<<2));
		addr += linelen;
	}
	DRAWOFF;
}

/* srccopy bitblt, opcode is currently ignored*/
static void
linear4_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op)
{
	ADDR8	dst;
	ADDR8	src;
	int	i;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;

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
	dst = dstpsd->addr + (dstx>>1) + dsty * dlinelen;
	src = srcpsd->addr + (srcx>>1) + srcy * slinelen;
	while(--h >= 0) {
		ADDR8	d = dst;
		ADDR8	s = src;
		MWCOORD	dx = dstx;
		MWCOORD	sx = srcx;
		for(i=0; i<w; ++i) {
			*d = (*d & notmask[dx&1]) |
			   ((*s >> (((sx&1))<<2) & 0x0f) << (((dx&1))<<2));
			if((++dx & 1) == 0)
				++d;
			if((++sx & 1) == 0)
				++s;
		}
		dst += dlinelen;
		src += slinelen;
	}
	DRAWOFF;
}

#endif  // For 8 bit memory access
/* ########################################################################## */


// All these functions are working using the same philosophie:
// There are 8 pixels in an int (pixel0= bit 0 to 3, ....)
// As this is a 32 bits micro, all access should be made
// on 32 bits and should try to work on as many pixels as necceseray
// in order to minimize memory access.

typedef unsigned int T8p;   // set of 8 pixels
typedef unsigned int * P8p; // pointer on a set of 8 pixels

// this set of mask allows to select the n first pixels of an int
static const T8p Masks[]= 
      {0x00000000, 0x0000000f, 0x000000ff, 0x00000fff, 
       0x0000ffff, 0x000fffff, 0x00ffffff, 0x0fffffff};

// this set of mask allow to select the n first pixels of an int. 
// but a selection of 0 pixels selects in fact the 8 pixels.
static const T8p Masks2[]= 
      {0xffffffff, 0x0000000f, 0x000000ff, 0x00000fff, 
       0x0000ffff, 0x000fffff, 0x00ffffff, 0x0fffffff};

// this function create a set of 8 pixels of the same color.
static inline T8p EColor(T8p c)
{
  c|= c<<4; c|= c<<8; c|= c<<16;
  return(c);
}

// this function compute the addres of the group of pixels containing
// pixel x, y when the graphic is pointed by m and the size of a line is
// LineSize bytes
static inline P8p PPointer(P8p m, int LineSize, int x, int y)
{
  return( (P8p) (((int)m+((x>>1)+y*LineSize))&~3) );
}

// this function is a memset, but faster, and for ints....
void intset(int *d, int value, int nbInts);

#define swap(t, a, b) { t temp= a; a= b; b= temp; }

static 
void linear41_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
  P8p p= PPointer(psd->addr, psd->linelen, x, y);       // get pixel address
  DRAWON;
  x= (x&0x7)<<2;                                        // get pixel position in the word (pixel number * 4)
  *p= ((*p)&~(0xf<<x))|(c<<x);                          // graphic = graphic - current pixel at pos + new picel at pos
  DRAWOFF;
}

/*
// timing used to check if the video memory is cachable.
// to use: uncomment, and uncomment the call to ltime in drawhorzline.
// if the result is around 170us, it's not chached.
// if it's around 10us, it's cached.

#include <sys/time.h>
#include <stdio.h>

long gt()
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return(tv.tv_sec*1000000+tv.tv_usec);
}

static void ltime(int *p)
{
  int i;
  static int toto= 0;
  if (toto==0)
  {
     long t2, t3, t= gt();
     for (i= 1000; i!=0; i--)
        intset(p, 12345678, 1024);
     t2= gt(); t3= gt();
     toto= 1;
     printf("4KB = %dus\r\n", (int)(t2-t-t3+t2)/1000);
  }
}*/

static void intset2(int *p, int v, int c) { while(c>0) { c--; *p++= v; } }

static 
void linear41_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
  T8p Mask1, Mask2;                                     // start of line and end of line mask
  P8p p;                                                // pointer on line
  DRAWON;
  if (x1>x2) swap(int, x1, x2);                         // ensure x1<x2
//  ltime(psd->addr);
  x2= x2-x1+1;                                          // get number of pixels to draw
  p= PPointer(psd->addr, psd->linelen, x1, y);          // get pointer on start of line
  x1&= 7;
  Mask1= Masks[x1];                                     // get end and begining of lines
  Mask2= Masks2[(x1+x2)&7];
  x2= (x1+x2+7)>>3;                                     // get number of full words to write
  c= EColor(c);                                         // get 8 times the color
  switch(x2)                                            // unrolled loops for up to 120pixels.
  {
    case 0: break;
    case 1:  Mask1= Mask1|~Mask2; *p= (*p&Mask1)|(c&~Mask1); break;
    case 2:  *p++= (*p&Mask1)|(c&~Mask1); *p= (*p&~Mask2)|(c&Mask2); break;
    case 3:  *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 4:  *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 5:  *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 6:  *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 7:  *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 8:  *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 9:  *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 10: *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 11: *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 12: *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 13: *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 14: *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    case 15: *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); break;
    default:
            x2-= 2;
            *p++= (*p&Mask1)|(c&~Mask1);                        // write start mask
            intset(p, c, x2);                                   // write n time 8 pixels
            *(p+x2)= (*(p+x2)&~Mask2)|(c&Mask2);                // write end mask
            break;
  }
  DRAWOFF;
}

static 
void linear41_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
  P8p p; T8p m; int d;
  DRAWON;
  if (y1>y2) swap(int, y1, y2);                         // ensure y1 < y2
  y2= y2-y1+1;                                          // get number of pixels to draw
  p= PPointer(psd->addr, psd->linelen, x, y1);          // get pointer on first pixel
  c<<= (x&7)<<2;                                        // get the color at the right place
  m= (0xf<<((x&7)<<2));                                 // get the mask
  d= psd->linelen >> 2;                                 // get the line lenght
  for(;y2!=0; y2--) { *p= (*p&~m)|c; p+= d; }            // for all lines, update the video data and jump to next line
  DRAWOFF;
}

static 
void linear41_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
  int d;
  T8p Mask1, Mask2;                                     // start of line and end of line mask    
  P8p p;                                                // pointer on line                       
  if (x1>x2) swap(int, x1, x2);                         // ensure x1<x2                                                                   
  if (y1>y2) swap(int, y1, y2);                         // ensure y1<y2                          
  DRAWON;                                                                                        
  x2= x2-x1+1;                                          // get number of pixels to draw          
  p= PPointer(psd->addr, psd->linelen, x1, y1);         // get pointer on start of line          
  d= (psd->linelen)>>2;                                 // line size                                         
  x1&= 7;                                               
  Mask1= Masks[x1];                                     // get end and begining of lines masks
  Mask2= Masks2[(x1+x2)&7];                              
  x2= (x1+x2+7)>>3;                                     // get number of full words to write     
  c= EColor(c);                                         // get 8 times the color                 
  y2-= y1-1;                                            // number of lines to draw 

  switch(x2)                                            // unrolled loop for up to 120 pixels
  {
    case 0: break;
    case 1:  Mask1= Mask1|~Mask2; for (; y2!=0; y2--) { *p= (*p&Mask1)|(c&~Mask1); p+=d; } break;
    case 2:  d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 3:  d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 4:  d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 5:  d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 6:  d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 7:  d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 8:  d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 9:  d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 10: d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 11: d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 12: d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 13: d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 14: d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    case 15: d= d-x2+1; for (; y2!=0; y2--) { *p++= (*p&Mask1)|(c&~Mask1); *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p++= c; *p= (*p&~Mask2)|(c&Mask2); p+=d; } break;
    default:
             d--; x2-= 2; 
             for (; y2!=0; y2--) 
             { 
               *p++= (*p&Mask1)|(c&~Mask1);                        // write start mask
               intset(p, c, x2);                                   // write n time 8 pixels
               *(p+x2)= (*(p+x2)&~Mask2)|(c&Mask2);                // write end mask
               p+= d;
             }
            break;
  }
  DRAWOFF;
}

#include "SubRepl.h"                                    // includes all the sub functions of blit

static 
void linear41_blit(PSD dstpsd, MWCOORD x, MWCOORD y, MWCOORD w, MWCOORD h, PSD srcpsd, MWCOORD sx, MWCOORD sy, long op)
{
  int ds, ss, shift;
  T8p Mask1, Mask2;
  // compute source and dest pointers
  P8p dData= PPointer(dstpsd->addr, dstpsd->linelen, x, y);
  P8p sData= PPointer(srcpsd->addr, srcpsd->linelen, sx, sy);
  // compute mask and shift
  x&=7; sx&=7;
  shift= (x-sx)<<2;
  Mask1= Masks[x];        // mask used to select what bellongs to the dest bitmap and source bitmap at the begining of each line
  Mask2= Masks2[(x+w)&7]; // mask used to select what bellongs to dest and source bitmap at the end of each line.
  // number of full int to work with+1
  w= -1+((w+x+7)>>3);
  // compute number of workd to skip at end of each line in the source and the destination
  ds= (dstpsd->linelen>>2)-w;
  ss= (srcpsd->linelen>>2)-w;

//printf("blt x %d y %d sx %d sy %d w %d h%d\r\n", x, y, sx, sy, w, h);
  // dispatching on sub functions
  if (shift!=0)
  {
    switch ((4&(shift>>28))|(w>2?3:w)) // sign: bit2, w: bits 0&1
    { 
      case  0: Blt_Shift_Positif_w0(sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case  1: Blt_Shift_Positif_w1(sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case  2: Blt_Shift_Positif_w2(sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case  3: Blt_Shift_Positif   (sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case  4: Blt_Shift_Negatif_w0(sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case  5: Blt_Shift_Negatif_w1(sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case  6: Blt_Shift_Negatif_w2(sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case  7: Blt_Shift_Negatif   (sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      default: return;
    }
  } else {
    switch (w)
    {
      case 0:  Blt_Shift_Null_w0   (sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case 1:  Blt_Shift_Null_w1   (sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case 2:  Blt_Shift_Null_w2   (sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      case 3:  Blt_Shift_Null_w3   (sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
      default: Blt_Shift_Null      (sData, dData, ss, ds, shift, Mask1, Mask2, w-1, h); return;
    }
  }
}

/* srccopy stretchblt*/
static void
linear4_stretchblit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, long op)
{
	ADDR8	dst;
	ADDR8	src;
	int	dlinelen = dstpsd->linelen;
	int	slinelen = srcpsd->linelen;
	int	i, ymax;
	int	row_pos, row_inc;
	int	col_pos, col_inc;
	unsigned char pixel = 0;

	assert (dstpsd->addr != 0);
	assert (dstx >= 0 && dstx < dstpsd->xres);
	assert (dsty >= 0 && dsty < dstpsd->yres);
	assert (dstw > 0);
	assert (dsth > 0);
	assert (srcpsd->addr != 0);
	assert (srcx >= 0 && srcx < srcpsd->xres);
	assert (srcy >= 0 && srcy < srcpsd->yres);
	assert (srcw > 0);
	assert (srch > 0);
	assert (dstx+dstw <= dstpsd->xres);
	assert (dsty+dsth <= dstpsd->yres);
	assert (srcx+srcw <= srcpsd->xres);
	assert (srcy+srch <= srcpsd->yres);

	DRAWON;
	row_pos = 0x10000;
	row_inc = (srch << 16) / dsth;

	/* stretch blit using integer ratio between src/dst height/width*/
	for (ymax = dsty+dsth; dsty<ymax; ++dsty) {
		MWCOORD	dx;
		MWCOORD	sx;

		/* find source y position*/
		while (row_pos >= 0x10000L) {
			++srcy;
			row_pos -= 0x10000L;
		}

		dst = dstpsd->addr + (dstx>>1) + dsty*dlinelen;
		src = srcpsd->addr + (srcx>>1) + (srcy-1)*slinelen;

		/* copy a row of pixels*/
		col_pos = 0x10000;
		col_inc = (srcw << 16) / dstw;
		sx = srcx;
		dx = dstx;
		for (i=0; i<dstw; ++i) {
			/* get source x pixel*/
			while (col_pos >= 0x10000L) {
				//pixel = (*src >> (((sx&1))<<2) ) & 0x0f;
				pixel = (*src >> ((1-(sx&1))<<2) ) & 0x0f;
				if (++sx & 1)
					++src;
				col_pos -= 0x10000L;
			}
			*dst = (*dst & notmask[dx&1]) | (pixel << (((dx&1))<<2) );
			//*dst = (*dst & revnotmask[dx&1]) | (pixel << ((1-(dx&1))<<2));
			if (++dx & 1)
				++dst;
			col_pos += col_inc;
		}

		row_pos += row_inc;
	}
	DRAWOFF;
}

#if 1 
SUBDRIVER fblinear4 = {
	linear4_init,
	linear41_drawpixel,
	linear4_readpixel,
	linear41_drawhorzline,
	linear4_drawvertline,
	linear41_fillrect,
	linear41_blit,
	NULL,
	linear4_stretchblit
};
#else
SUBDRIVER fblinear4 = {
	linear4_init,
	linear4_drawpixel,
	linear4_readpixel,
	linear4_drawhorzline,
	linear4_drawvertline,
	gen_fillrect,
	linear4_blit,
	NULL,
	linear4_stretchblit
};
#endif 
