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

/* Linux framebuffer critical sections*/
#if VTSWITCH
extern volatile int mwdrawing;
#define DRAWON		++mwdrawing
#define DRAWOFF		--mwdrawing
#else
#define DRAWON
#define DRAWOFF
#endif

typedef unsigned char *		ADDR8;
typedef unsigned short *	ADDR16;
typedef uint32_t *			ADDR32;

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

/* applyOp with count, src ptr, ssz/dsz increment*/
#define	APPLYOP(op, width, STYPE, s, DTYPE, d, ssz, dsz)	\
	{											\
		int  count = width;						\
		switch (op) {							\
		case MWROP_COPY:						\
		case MWROP_SRC_OVER:					\
		case MWROP_SRC_IN:						\
		case MWROP_SRC_ATOP:					\
			while(--count >= 0) {				\
				DTYPE d = STYPE s;				\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_XOR_FGBG:					\
		case MWROP_PORTERDUFF_XOR:				\
			while(--count >= 0) {				\
				DTYPE d ^= (STYPE s) ^ gr_background; \
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_SRCTRANSCOPY:				\
			while(--count >= 0) {				\
				DTYPE d = (DTYPE d)? DTYPE d: STYPE s; \
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_XOR:							\
			while(--count >= 0) {				\
				DTYPE d ^= STYPE s;				\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_AND:							\
			while(--count >= 0) {				\
				DTYPE d &= STYPE s;				\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_OR:							\
			while(--count >= 0) {				\
				DTYPE d |= STYPE s;				\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_SRC_OUT:						\
		case MWROP_DST_OUT:						\
		case MWROP_CLEAR:						\
			while(--count >= 0) {				\
				DTYPE d = 0;					\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_SET:							\
			while(--count >= 0) {				\
				DTYPE d = ~0;					\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_EQUIV:						\
			while(--count >= 0) {				\
				DTYPE d = ~(DTYPE d ^ STYPE s); \
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_NOR:							\
			while(--count >= 0) {				\
				DTYPE d = ~(DTYPE d | STYPE s); \
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_NAND:						\
			while(--count >= 0) {				\
				DTYPE d = ~(DTYPE d & STYPE s); \
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_INVERT:						\
			while(--count >= 0) {				\
				DTYPE d = ~DTYPE d;				\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_COPYINVERTED:				\
			while(--count >= 0) {				\
				DTYPE d = ~STYPE s;				\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_ORINVERTED:					\
			while(--count >= 0) {				\
				DTYPE d |= ~STYPE s;			\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_ANDINVERTED:					\
			while(--count >= 0) {				\
				DTYPE d &= ~STYPE s;			\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_ORREVERSE:					\
			while(--count >= 0) {				\
				DTYPE d = ~DTYPE d | STYPE s; 	\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_ANDREVERSE:					\
			while(--count >= 0) {				\
				DTYPE d = ~DTYPE d & STYPE s; 	\
				d += dsz; s += ssz; }			\
			break;								\
		case MWROP_NOOP:						\
		case MWROP_DST_OVER:					\
		case MWROP_DST_IN:						\
		case MWROP_DST_ATOP:					\
			break;								\
		}										\
	}

/* APPLYOP w/return value - used only in fblin4.c*/
#define DEFINE_applyOpR				\
static inline int applyOpR(int op, unsigned char src, unsigned char dst)	\
{						\
	switch (op) {				\
	case MWROP_XOR:			\
		return (src) ^ (dst);		\
	case MWROP_AND:			\
		return (src) & (dst);		\
	case MWROP_OR:				\
		return (src) | (dst);		\
	case MWROP_SRC_OUT:		\
	case MWROP_DST_OUT:		\
	case MWROP_PORTERDUFF_XOR:		\
	case MWROP_CLEAR:			\
		return 0;			\
	case MWROP_SET:			\
		return ~0;			\
	case MWROP_EQUIV:			\
		return ~((src) ^ (dst));	\
	case MWROP_NOR:			\
		return ~((src) | (dst));	\
	case MWROP_NAND:			\
		return ~((src) & (dst));	\
	case MWROP_INVERT:			\
		return ~(dst);			\
	case MWROP_COPYINVERTED:		\
		return ~(src);			\
	case MWROP_ORINVERTED:			\
		return ~(src) | (dst);		\
	case MWROP_ANDINVERTED:		\
		return ~(src) & (dst);		\
	case MWROP_ORREVERSE:			\
		return (src) | ~(dst);		\
	case MWROP_ANDREVERSE:			\
		return (src) & ~(dst);		\
	case MWROP_SRC_OVER:		\
	case MWROP_SRC_IN:			\
	case MWROP_SRC_ATOP:		\
	case MWROP_COPY:			\
		return (src);			\
	case MWROP_XOR_FGBG:		\
		return (src) ^ (dst) ^ gr_background;	\
	case MWROP_DST_OVER:		\
	case MWROP_DST_IN:			\
	case MWROP_DST_ATOP:		\
	case MWROP_NOOP:			\
	default:				\
		return (dst);			\
	}					\
}

/* global vars*/
extern int 	gr_mode;	/* temp kluge*/

/* entry points*/
/* scr_fb.c*/
void ioctl_getpalette(int start, int len, short *red, short *green,short *blue);
void ioctl_setpalette(int start, int len, short *red, short *green,short *blue);

/* fb.c*/
int		gen_initpsd(PSD psd, int pixtype, MWCOORD xres, MWCOORD yres, int flags);
PSUBDRIVER select_fb_subdriver(PSD psd);
int		set_data_formatex(int pixtype, int bpp);
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
