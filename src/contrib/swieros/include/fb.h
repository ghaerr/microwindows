/*
 * Copyright (c) 1999, 2000, 2001, 2002, 2010 Greg Haerr <greg@censoft.com>
 *
 * Framebuffer drivers header file for Microwindows Screen Drivers
 *
 * Portions contributed by Koninklijke Philips Electronics N.V.
 * These portions are Copyright 2002 Koninklijke Philips Electronics
 * N.V.  All Rights Reserved.  These portions are licensed under the
 * terms of the Mozilla Public License, version 1.1, or, at your
 * option, the GNU General Public License version 2.0.  Please see
 * the file "ChangeLog" for documentation regarding these
 * contributions.
 */

#define DRAWON
#define DRAWOFF

typedef unsigned char *		ADDR8;
typedef unsigned short *	ADDR16;
typedef uint32_t *			ADDR32;

/* global vars*/
extern int 	gr_mode;	/* temp kluge*/

/* entry points*/
/* scr_fb.c*/
void ioctl_getpalette(int start, int len, short *red, short *green,short *blue);
void ioctl_setpalette(int start, int len, short *red, short *green,short *blue);

/* fb.c*/
int		gen_initpsd(PSD psd, int pixtype, MWCOORD xres, MWCOORD yres, int flags);
PSUBDRIVER select_fb_subdriver(PSD psd);
int		set_data_format(PSD psd);
void	gen_getscreeninfo(PSD psd, PMWSCREENINFO psi);

/* fbportrait_xxx.c*/
extern SUBDRIVER fbportrait_left;
extern SUBDRIVER fbportrait_right;
extern SUBDRIVER fbportrait_down;

void fbportrait_left_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
MWPIXELVAL fbportrait_left_readpixel(PSD psd, MWCOORD x, MWCOORD y);
void fbportrait_left_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
void fbportrait_left_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
void fbportrait_left_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c);
void fbportrait_left_blit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op);
void fbportrait_left_convblit_blend_mask_alpha_byte(PSD dstpsd, PMWBLITPARMS gc);
void fbportrait_left_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc);
void fbportrait_left_convblit_copy_mask_mono_byte_lsb(PSD psd, PMWBLITPARMS gc);

void fbportrait_right_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
MWPIXELVAL fbportrait_right_readpixel(PSD psd, MWCOORD x, MWCOORD y);
void fbportrait_right_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
void fbportrait_right_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
void fbportrait_right_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c);
void fbportrait_right_blit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op);
void fbportrait_right_convblit_blend_mask_alpha_byte(PSD dstpsd, PMWBLITPARMS gc);
void fbportrait_right_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc);
void fbportrait_right_convblit_copy_mask_mono_byte_lsb(PSD psd, PMWBLITPARMS gc);

void fbportrait_down_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c);
MWPIXELVAL fbportrait_down_readpixel(PSD psd, MWCOORD x, MWCOORD y);
void fbportrait_down_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c);
void fbportrait_down_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c);
void fbportrait_down_fillrect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c);
void fbportrait_down_blit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD w, MWCOORD h,
	PSD srcpsd, MWCOORD srcx, MWCOORD srcy, int op);
void fbportrait_down_convblit_blend_mask_alpha_byte(PSD dstpsd, PMWBLITPARMS gc);
void fbportrait_down_convblit_copy_mask_mono_byte_msb(PSD psd, PMWBLITPARMS gc);
void fbportrait_down_convblit_copy_mask_mono_byte_lsb(PSD psd, PMWBLITPARMS gc);

/* rasterops.c*/
void GdRasterOp(PMWIMAGEHDR pixd, MWCOORD dx, MWCOORD dy, MWCOORD dw, MWCOORD dh, int op,
			PMWIMAGEHDR pixs, MWCOORD sx, MWCOORD sy);

void APPLYOP_SRC_INT(int count, uint32_t s, unsigned char *d, int dsz);
void APPLYOP_SRC_PTR(int op, int count, unsigned char *s, unsigned char *d, int ssz, int dsz);
