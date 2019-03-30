/*
 * Copyright (c) 1999, 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 *
 * 32bpp Linear Video Driver for Microwindows (BGRA or RGBA byte order)
 * Writes memory image: |B|G|R|A| LE 0xARGB BE 0xBGRA MWPF_TRUECOLOR8888
 * Writes memory image: |R|G|B|A| LE 0xABGR BE 0xRGBA MWPF_TRUECOLORABGR
 *
 * Inspired from Ben Pfaff's BOGL <pfaffben@debian.org>
 */

/* Set pixel at x, y, to pixelval c*/
static void
linear32_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x << 2);

	DRAWON;
	if (gr_mode == MWROP_COPY)
		*((ADDR32)addr) = c;
	else
		APPLYOP_SRC_INT(1, c, addr, 0);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y, 1, 1);
}

/* Read pixel at x, y*/
static MWPIXELVAL
linear32_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x << 2);
	return *((ADDR32)addr);
}

/* Draw horizontal line from x1,y to x2,y including final point*/
static void
linear32_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	register unsigned char *addr = psd->addr + y * psd->pitch + (x1 << 2);
	int width = x2-x1+1;

	DRAWON;
	if(gr_mode == MWROP_COPY)
	{
		int w = width;
		while (--w >= 0)
		{
			*((ADDR32)addr) = c;
			addr += 4;
		}
	}
	else
		APPLYOP_SRC_INT(width, c, addr, 4);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x1, y, width, 1);
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
static void
linear32_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	int	pitch = psd->pitch;
	register unsigned char *addr = psd->addr + y1 * pitch + (x << 2);
	int height = y2-y1+1;

	DRAWON;
	if(gr_mode == MWROP_COPY)
	{
		int h = height;
		while (--h >= 0)
		{
			*((ADDR32)addr) = c;
			addr += pitch;
		}
	}
	else
		APPLYOP_SRC_INT(height, c, addr, pitch);
	DRAWOFF;

	if (psd->Update)
		psd->Update(psd, x, y1, 1, height);
}

/* RGBA subdriver*/
static SUBDRIVER fblinear32rgba_none = {
	linear32_drawpixel,
	linear32_readpixel,
	linear32_drawhorzline,
	linear32_drawvertline,
	gen_fillrect,
	NULL,			/* no fallback Blit - uses BlitFrameBlit*/
	frameblit_xxxa8888,
	0, //frameblit_stretch_xxxa8888,
	0, //convblit_copy_mask_mono_byte_msb_rgba,		/* ft2 non-alias*/
	0, //convblit_copy_mask_mono_byte_lsb_rgba,		/* t1 non-alias*/
	convblit_copy_mask_mono_word_msb_rgba,		/* core/pcf non-alias*/
	0, //convblit_blend_mask_alpha_byte_rgba,		/* ft2/t1 antialias*/
	0, //convblit_copy_rgba8888_rgba8888,			/* RGBA image copy (GdArea MWPF_RGB)*/
	0, //convblit_srcover_rgba8888_rgba8888,			/* RGBA images w/alpha*/
	0, //convblit_copy_rgb888_rgba8888,				/* RGB images no alpha*/
	0 //frameblit_stretch_xxxa8888					/* RGBA -> RGBA stretchblit*/
};

static SUBDRIVER fblinear32rgba_left = {
};

static SUBDRIVER fblinear32rgba_right = {
};

static SUBDRIVER fblinear32rgba_down = {
};

PSUBDRIVER fblinear32rgba[4] = {
	&fblinear32rgba_none, &fblinear32rgba_left, &fblinear32rgba_right, &fblinear32rgba_down
};

/* Note that the following APPLYOP macro implements the
 * Porter-Duff rules assuming that source and destination
 * both have an alpha of 1.0.  Drivers that support alpha
 * should provide a better implementation of these rules.
 *
 * The following are not handled yet:
 *		MWROP_SRC_IN
 *		MWROP_SRC_ATOP
 *		MWROP_DST_OVER
 *		MWROP_DST_IN
 *		MWROP_DST_ATOP
 *		MWROP_SRC_OUT
 *		MWROP_DST_OUT
 *		MWROP_PORTERDUFF_XOR
 *
 * Arguments:
 *	op		- MWROP code
 *	width	- repeat count
 *	STYPE	- src 'type' e.g. (ADDR32) or (MWPIXELVAL)
 *	s		- src pointer or value
 *	DTYPE	- dst 'type' e.g. *(ADDR32)
 *	d		- dst pointer
 *	ssz		- src pointer increment
 *	dsz		- dst pointer increment
 */
/* applyOp with count, src int, ssz/dsz increment*/
void
APPLYOP_SRC_INT(int count, uint32_t s, unsigned char *d, int dsz)
{											
		switch (gr_mode) {							
		case MWROP_COPY:						
		case MWROP_SRC_OVER:					
		case MWROP_SRC_IN:						
		case MWROP_SRC_ATOP:					
			while(--count >= 0) {				
				*(ADDR32) d = s;				
				d += dsz; }
			return;								
		case MWROP_XOR_FGBG:					
		case MWROP_PORTERDUFF_XOR:				
			while(--count >= 0) {				
				*(ADDR32) d ^= (s) ^ gr_background; 
				d += dsz; }
			return;								
		case MWROP_SRCTRANSCOPY:				
			while(--count >= 0) {				
				*(ADDR32) d = (*(ADDR32) d)? *(ADDR32) d: s; 
				d += dsz; }
			return;								
		case MWROP_XOR:							
			while(--count >= 0) {				
				*(ADDR32) d ^= s;				
				d += dsz; }
			return;								
		case MWROP_AND:							
			while(--count >= 0) {				
				*(ADDR32) d &= s;				
				d += dsz; }
			return;								
		case MWROP_OR:							
			while(--count >= 0) {				
				*(ADDR32) d |= s;				
				d += dsz; }
			return;								
		case MWROP_SRC_OUT:						
		case MWROP_DST_OUT:						
		case MWROP_CLEAR:						
			while(--count >= 0) {				
				*(ADDR32) d = 0;					
				d += dsz; }
			return;								
		case MWROP_SET:							
			while(--count >= 0) {				
				*(ADDR32) d = ~0;					
				d += dsz; }
			return;								
		case MWROP_EQUIV:						
			while(--count >= 0) {				
				*(ADDR32) d = ~(*(ADDR32) d ^ s); 
				d += dsz; }
			return;								
		case MWROP_NOR:							
			while(--count >= 0) {				
				*(ADDR32) d = ~(*(ADDR32) d | s); 
				d += dsz; }
			return;								
		case MWROP_NAND:						
			while(--count >= 0) {				
				*(ADDR32) d = ~(*(ADDR32) d & s); 
				d += dsz; }
			return;								
		case MWROP_INVERT:						
			while(--count >= 0) {				
				*(ADDR32) d = ~*(ADDR32) d;				
				d += dsz; }
			return;								
		case MWROP_COPYINVERTED:				
			while(--count >= 0) {				
				*(ADDR32) d = ~s;				
				d += dsz; }
			return;								
		case MWROP_ORINVERTED:					
			while(--count >= 0) {				
				*(ADDR32) d |= ~s;			
				d += dsz; }
			return;								
		case MWROP_ANDINVERTED:					
			while(--count >= 0) {				
				*(ADDR32) d &= ~s;			
				d += dsz; }
			return;								
		case MWROP_ORREVERSE:					
			while(--count >= 0) {				
				*(ADDR32) d = ~*(ADDR32) d | s; 	
				d += dsz; }
			return;								
		case MWROP_ANDREVERSE:					
			while(--count >= 0) {				
				*(ADDR32) d = ~*(ADDR32) d & s; 	
				d += dsz; }
			return;								
		case MWROP_NOOP:						
		case MWROP_DST_OVER:					
		case MWROP_DST_IN:						
		case MWROP_DST_ATOP:					
			return;								
		}										
}

/* applyOp with count, src int, ssz/dsz increment*/
void
APPLYOP_SRC_PTR(int op, int count, unsigned char *s, unsigned char *d, int ssz, int dsz)
{											
		switch (op) {							
		case MWROP_COPY:						
		case MWROP_SRC_OVER:					
		case MWROP_SRC_IN:						
		case MWROP_SRC_ATOP:					
			while(--count >= 0) {				
				*(ADDR32) d = *(ADDR32) s;				
				d += dsz; s += ssz; }
			return;								
		case MWROP_XOR_FGBG:					
		case MWROP_PORTERDUFF_XOR:				
			while(--count >= 0) {				
				*(ADDR32) d ^= (*(ADDR32) s) ^ gr_background; 
				d += dsz; s += ssz; }
			return;								
		case MWROP_SRCTRANSCOPY:				
			while(--count >= 0) {				
				*(ADDR32) d = (*(ADDR32) d)? *(ADDR32) d: *(ADDR32) s; 
				d += dsz; s += ssz; }
			return;								
		case MWROP_XOR:							
			while(--count >= 0) {				
				*(ADDR32) d ^= *(ADDR32) s;				
				d += dsz; s += ssz; }
			return;								
		case MWROP_AND:							
			while(--count >= 0) {				
				*(ADDR32) d &= *(ADDR32) s;				
				d += dsz; s += ssz; }
			return;								
		case MWROP_OR:							
			while(--count >= 0) {				
				*(ADDR32) d |= *(ADDR32) s;				
				d += dsz; s += ssz; }
			return;								
		case MWROP_SRC_OUT:						
		case MWROP_DST_OUT:						
		case MWROP_CLEAR:						
			while(--count >= 0) {				
				*(ADDR32) d = 0;					
				d += dsz; s += ssz; }
			return;								
		case MWROP_SET:							
			while(--count >= 0) {				
				*(ADDR32) d = ~0;					
				d += dsz; s += ssz; }
			return;								
		case MWROP_EQUIV:						
			while(--count >= 0) {				
				*(ADDR32) d = ~(*(ADDR32) d ^ *(ADDR32) s); 
				d += dsz; s += ssz; }
			return;								
		case MWROP_NOR:							
			while(--count >= 0) {				
				*(ADDR32) d = ~(*(ADDR32) d | *(ADDR32) s); 
				d += dsz; s += ssz; }
			return;								
		case MWROP_NAND:						
			while(--count >= 0) {				
				*(ADDR32) d = ~(*(ADDR32) d & *(ADDR32) s); 
				d += dsz; s += ssz; }
			return;								
		case MWROP_INVERT:						
			while(--count >= 0) {				
				*(ADDR32) d = ~*(ADDR32) d;				
				d += dsz; s += ssz; }
			return;								
		case MWROP_COPYINVERTED:				
			while(--count >= 0) {				
				*(ADDR32) d = ~*(ADDR32) s;				
				d += dsz; s += ssz; }
			return;								
		case MWROP_ORINVERTED:					
			while(--count >= 0) {				
				*(ADDR32) d |= ~*(ADDR32) s;			
				d += dsz; s += ssz; }
			return;								
		case MWROP_ANDINVERTED:					
			while(--count >= 0) {				
				*(ADDR32) d &= ~*(ADDR32) s;			
				d += dsz; s += ssz; }
			return;								
		case MWROP_ORREVERSE:					
			while(--count >= 0) {				
				*(ADDR32) d = ~*(ADDR32) d | *(ADDR32) s; 	
				d += dsz; s += ssz; }
			return;								
		case MWROP_ANDREVERSE:					
			while(--count >= 0) {				
				*(ADDR32) d = ~*(ADDR32) d & *(ADDR32) s; 	
				d += dsz; s += ssz; }
			return;								
		case MWROP_NOOP:						
		case MWROP_DST_OVER:					
		case MWROP_DST_IN:						
		case MWROP_DST_ATOP:					
			return;								
		}										
}
