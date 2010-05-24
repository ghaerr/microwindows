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

/* Note that the following ROP macro implements the
 * Porter-Duff rules assuming that source and destination
 * both have an alpha of 1.0.  Drivers that support alpha
 * should provide a better implementation of these rules.
 */

 /* The following are not handled yet:
		MWROP_SRC_IN
		MWROP_SRC_ATOP
		MWROP_DST_OVER
		MWROP_DST_IN
		MWROP_DST_ATO:
		MWROP_SRC_OU:
		MWROP_DST_OUT
		MWROP_PORTERDUFF_XO:
 */

/* ROP macro for 16 drawing modes*/
#define CHECK(f,d)	


/* applyOp w/stored dst*/
#define	applyOp2(cnt, op, src, d, type)		\
	{											\
		int  count = cnt;						\
		switch (op) {							\
		case MWROP_SRCTRANSCOPY:				\
			while(--count >= 0) {				\
				*d = (*d)? *d:src;				\
				++d; }							\
			break;								\
		case MWROP_XOR:						\
			while(--count >= 0) {				\
				*d ^= (src);					\
				++d; }							\
			CHECK("XOR", *d);					\
			break;								\
		case MWROP_AND:						\
			while(--count >= 0) {				\
				*d &= (src);					\
				++d; }							\
			CHECK("AND", *d);					\
			break;								\
		case MWROP_OR:							\
			while(--count >= 0) {				\
				*d |= (src);					\
				++d; }							\
			CHECK("OR", *d);					\
			break;								\
		case MWROP_SRC_OUT:					\
		case MWROP_DST_OUT:					\
		case MWROP_PORTERDUFF_XOR:				\
		case MWROP_CLEAR:						\
			while(--count >= 0) {				\
				*d = 0;							\
				++d; }							\
			CHECK("CLEAR", *d);					\
			break;								\
		case MWROP_SET:						\
			while(--count >= 0) {				\
				*d = ~0;						\
				++d; }							\
			CHECK("SET", *d);				\
			break;								\
		case MWROP_EQUIV:						\
			while(--count >= 0) {				\
				*d = ~(*d ^ (src));				\
				++d; }							\
			CHECK("EQUIV", *d);					\
			break;								\
		case MWROP_NOR:						\
			while(--count >= 0) {				\
				*d = ~(*d | (src));				\
				++d; }							\
			CHECK("NOR", *d);					\
			break;								\
		case MWROP_NAND:						\
			while(--count >= 0) {				\
				*d = ~(*d & (src));				\
				++d; }							\
			CHECK("NAND", *d);					\
			break;								\
		case MWROP_INVERT:						\
			while(--count >= 0) {				\
				*d = ~*d;						\
				++d; }							\
			CHECK("INVERT", *d);				\
			break;								\
		case MWROP_COPYINVERTED:				\
			while(--count >= 0) {				\
				*d = ~(src);					\
				++d; }							\
			CHECK("COPYINVERTED", *d);			\
			break;								\
		case MWROP_ORINVERTED:					\
			while(--count >= 0) {				\
				*d |= ~(src);					\
				++d; }							\
			CHECK("ORINVERTED", *d);			\
			break;								\
		case MWROP_ANDINVERTED:				\
			while(--count >= 0) {				\
				*d &= ~(src);					\
				++d; }							\
			CHECK("ANDINVERTED", *d);			\
			break;								\
		case MWROP_ORREVERSE:					\
			while(--count >= 0) {				\
				*d = ~*d | (src);				\
				++d; }							\
			CHECK("ORREVERSE", *d);				\
			break;								\
		case MWROP_ANDREVERSE:					\
			while(--count >= 0) {				\
				*d = ~*d & (src);				\
				++d; }							\
			CHECK("ANDREVERSE", *d);			\
			break;								\
		case MWROP_SRC_OVER:					\
		case MWROP_SRC_IN:						\
		case MWROP_SRC_ATOP:					\
		case MWROP_COPY:						\
			while(--count >= 0) {				\
				*d = (src);						\
				++d; }							\
			CHECK("COPY", *d);					\
			break;								\
		case MWROP_DST_OVER:					\
		case MWROP_DST_IN:						\
		case MWROP_DST_ATOP:					\
		case MWROP_NOOP:						\
			CHECK("NOOP", *d);					\
			break;								\
		case MWROP_XOR_FGBG:					\
			while(--count >= 0) {				\
				*d ^= (src) ^ gr_background;	\
				++d; }							\
			CHECK("XOR_FGBG", *d);				\
			break;								\
		}										\
	}

/* applyOp2 with count and step*/
#define	applyOp3(cnt, step, op, src, d, type)	\
	{											\
		int  count = cnt;						\
		switch (op) {							\
		case MWROP_SRCTRANSCOPY:				\
			while(--count >= 0) {				\
				*d = (*d)? *d:src;				\
				d += step; }					\
			break;								\
		case MWROP_XOR:						\
			while(--count >= 0) {				\
				*d ^= (src);					\
				d += step; }					\
			CHECK("XOR", *d);					\
			break;								\
		case MWROP_AND:						\
			while(--count >= 0) {				\
				*d &= (src);					\
				d += step; }					\
			CHECK("AND", *d);					\
			break;								\
		case MWROP_OR:							\
			while(--count >= 0) {				\
				*d |= (src);					\
				d += step; }					\
			CHECK("OR", *d);					\
			break;								\
		case MWROP_SRC_OUT:					\
		case MWROP_DST_OUT:					\
		case MWROP_PORTERDUFF_XOR:				\
		case MWROP_CLEAR:						\
			while(--count >= 0) {				\
				*d = 0;							\
				d += step; }					\
			CHECK("CLEAR", *d);					\
			break;								\
		case MWROP_SET:						\
			while(--count >= 0) {				\
				*d = ~0;						\
				d += step; }					\
			CHECK("SET", *d);				\
			break;								\
		case MWROP_EQUIV:						\
			while(--count >= 0) {				\
				*d = ~(*d ^ (src));				\
				d += step; }					\
			CHECK("EQUIV", *d);					\
			break;								\
		case MWROP_NOR:						\
			while(--count >= 0) {				\
				*d = ~(*d | (src));				\
				d += step; }					\
			CHECK("NOR", *d);					\
			break;								\
		case MWROP_NAND:						\
			while(--count >= 0) {				\
				*d = ~(*d & (src));				\
				d += step; }					\
			CHECK("NAND", *d);					\
			break;								\
		case MWROP_INVERT:						\
			while(--count >= 0) {				\
				*d = ~*d;						\
				d += step; }					\
			CHECK("INVERT", *d);				\
			break;								\
		case MWROP_COPYINVERTED:				\
			while(--count >= 0) {				\
				*d = ~(src);					\
				d += step; }					\
			CHECK("COPYINVERTED", *d);			\
			break;								\
		case MWROP_ORINVERTED:					\
			while(--count >= 0) {				\
				*d |= ~(src);					\
				d += step; }					\
			CHECK("ORINVERTED", *d);			\
			break;								\
		case MWROP_ANDINVERTED:				\
			while(--count >= 0) {				\
				*d &= ~(src);					\
				d += step; }					\
			CHECK("ANDINVERTED", *d);			\
			break;								\
		case MWROP_ORREVERSE:					\
			while(--count >= 0) {				\
				*d = ~*d | (src);				\
				d += step; }					\
			CHECK("ORREVERSE", *d);				\
			break;								\
		case MWROP_ANDREVERSE:					\
			while(--count >= 0) {				\
				*d = ~*d & (src);				\
				d += step; }					\
			CHECK("ANDREVERSE", *d);			\
			break;								\
		case MWROP_SRC_OVER:					\
		case MWROP_SRC_IN:						\
		case MWROP_SRC_ATOP:					\
		case MWROP_COPY:						\
			while(--count >= 0) {				\
				*d = (src);						\
				d += step; }					\
			CHECK("COPY", *d);					\
			break;								\
		case MWROP_DST_OVER:					\
		case MWROP_DST_IN:						\
		case MWROP_DST_ATOP:					\
		case MWROP_NOOP:						\
			CHECK("NOOP", *d);					\
			break;								\
		case MWROP_XOR_FGBG:					\
			while(--count >= 0) {				\
				*d ^= (src) ^ gr_background;	\
				d += step; }					\
			CHECK("XOR_FGBG", *d);				\
			break;								\
		}										\
	}

/* applyOp2 with count and src/dst incr*/
#define	applyOp4(cnt, op, s, d, type)			\
	{											\
		int  count = cnt;						\
		switch (op) {							\
		case MWROP_SRCTRANSCOPY:				\
			while(--count >= 0) {				\
				*d = (*d)? *d:*s;				\
				++d; ++s; }						\
			break;								\
		case MWROP_XOR:						\
			while(--count >= 0) {				\
				*d ^= (*s);						\
				++d; ++s; }						\
			CHECK("XOR", *d);					\
			break;								\
		case MWROP_AND:						\
			while(--count >= 0) {				\
				*d &= (*s);						\
				++d; ++s; }						\
			CHECK("AND", *d);					\
			break;								\
		case MWROP_OR:							\
			while(--count >= 0) {				\
				*d |= (*s);						\
				++d; ++s; }						\
			CHECK("OR", *d);					\
			break;								\
		case MWROP_SRC_OUT:					\
		case MWROP_DST_OUT:					\
		case MWROP_PORTERDUFF_XOR:				\
		case MWROP_CLEAR:						\
			while(--count >= 0) {				\
				*d = 0;							\
				++d; ++s; }						\
			CHECK("CLEAR", *d);					\
			break;								\
		case MWROP_SET:						\
			while(--count >= 0) {				\
				*d = ~0;						\
				++d; ++s; }						\
			CHECK("SET", *d);				\
			break;								\
		case MWROP_EQUIV:						\
			while(--count >= 0) {				\
				*d = ~(*d ^ (*s));				\
				++d; ++s; }						\
			CHECK("EQUIV", *d);					\
			break;								\
		case MWROP_NOR:						\
			while(--count >= 0) {				\
				*d = ~(*d | (*s));				\
				++d; ++s; }						\
			CHECK("NOR", *d);					\
			break;								\
		case MWROP_NAND:						\
			while(--count >= 0) {				\
				*d = ~(*d & (*s));				\
				++d; ++s; }						\
			CHECK("NAND", *d);					\
			break;								\
		case MWROP_INVERT:						\
			while(--count >= 0) {				\
				*d = ~*d;						\
				++d; ++s; }						\
			CHECK("INVERT", *d);				\
			break;								\
		case MWROP_COPYINVERTED:				\
			while(--count >= 0) {				\
				*d = ~(*s);					\
				++d; ++s; }						\
			CHECK("COPYINVERTED", *d);			\
			break;								\
		case MWROP_ORINVERTED:					\
			while(--count >= 0) {				\
				*d |= ~(*s);					\
				++d; ++s; }						\
			CHECK("ORINVERTED", *d);			\
			break;								\
		case MWROP_ANDINVERTED:				\
			while(--count >= 0) {				\
				*d &= ~(*s);					\
				++d; ++s;}						\
			CHECK("ANDINVERTED", *d);			\
			break;								\
		case MWROP_ORREVERSE:					\
			while(--count >= 0) {				\
				*d = ~*d | (*s);				\
				++d; ++s; }						\
			CHECK("ORREVERSE", *d);				\
			break;								\
		case MWROP_ANDREVERSE:					\
			while(--count >= 0) {				\
				*d = ~*d & (*s);				\
				++d; ++s; }						\
			CHECK("ANDREVERSE", *d);			\
			break;								\
		case MWROP_SRC_OVER:					\
		case MWROP_SRC_IN:						\
		case MWROP_SRC_ATOP:					\
		case MWROP_COPY:						\
			while(--count >= 0) {				\
				*d = (*s);						\
				++d; ++s; }						\
			CHECK("COPY", *d);					\
			break;								\
		case MWROP_DST_OVER:					\
		case MWROP_DST_IN:						\
		case MWROP_DST_ATOP:					\
		case MWROP_NOOP:						\
			CHECK("NOOP", *d);					\
			break;								\
		case MWROP_XOR_FGBG:					\
			while(--count >= 0) {				\
				*d ^= (*s) ^ gr_background;		\
				++d; ++s; }						\
			CHECK("XOR_FGBG", *d);				\
			break;								\
		}										\
	}

/* applyOp w/stored dst*/
#define	applyOp(op, src, pdst, type)		\
	{							\
	type d = (pdst);			\
	switch (op) {				\
	case MWROP_SRCTRANSCOPY:   \
		*d = (*d)? *d:src;		\
		break;              \
	case MWROP_XOR:			\
		*d ^= (src);			\
		CHECK("XOR", *d);		\
		break;				\
	case MWROP_AND:			\
		*d &= (src);			\
		CHECK("AND", *d);		\
		break;				\
	case MWROP_OR:				\
		*d |= (src);			\
		CHECK("OR", *d);		\
		break;				\
	case MWROP_SRC_OUT:		\
	case MWROP_DST_OUT:		\
	case MWROP_PORTERDUFF_XOR:		\
	case MWROP_CLEAR:			\
		*d = 0;				\
		CHECK("CLEAR", *d);		\
		break;				\
	case MWROP_SET:			\
		*d = ~0;			\
		CHECK("SET", *d);		\
		break;				\
	case MWROP_EQUIV:			\
		*d = ~(*d ^ (src));		\
		CHECK("EQUIV", *d);		\
		break;				\
	case MWROP_NOR:			\
		*d = ~(*d | (src));		\
		CHECK("NOR", *d);		\
		break;				\
	case MWROP_NAND:			\
		*d = ~(*d & (src));		\
		CHECK("NAND", *d);		\
		break;				\
	case MWROP_INVERT:			\
		*d = ~*d;			\
		CHECK("INVERT", *d);		\
		break;				\
	case MWROP_COPYINVERTED:		\
		*d = ~(src);			\
		CHECK("COPYINVERTED", *d);	\
		break;				\
	case MWROP_ORINVERTED:			\
		*d |= ~(src);			\
		CHECK("ORINVERTED", *d);	\
		break;				\
	case MWROP_ANDINVERTED:		\
		*d &= ~(src);			\
		CHECK("ANDINVERTED", *d);	\
		break;				\
	case MWROP_ORREVERSE:			\
		*d = ~*d | (src);		\
		CHECK("ORREVERSE", *d);		\
		break;				\
	case MWROP_ANDREVERSE:			\
		*d = ~*d & (src);		\
		CHECK("ANDREVERSE", *d);	\
		break;				\
	case MWROP_SRC_OVER:		\
	case MWROP_SRC_IN:			\
	case MWROP_SRC_ATOP:		\
	case MWROP_COPY:			\
		*d = (src);			\
		CHECK("COPY", *d);		\
		break;				\
	case MWROP_DST_OVER:		\
	case MWROP_DST_IN:			\
	case MWROP_DST_ATOP:		\
	case MWROP_NOOP:			\
		CHECK("NOOP", *d);		\
		break;				\
	case MWROP_XOR_FGBG:		\
		*d ^= (src) ^ gr_background;	\
		CHECK("XOR_FGBG", *d);		\
		break;				\
	}					\
}

/* applyOp w/return value*/
#define DEFINE_applyOpR				\
static inline int applyOpR(op, src, dst)	\
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
extern MWPIXELVAL gr_background;

/* entry points*/
/* scr_fb.c*/
void ioctl_getpalette(int start, int len, short *red, short *green,short *blue);
void ioctl_setpalette(int start, int len, short *red, short *green,short *blue);
void setfadelevel(PSD psd, int f);

/* genmem.c*/
void	gen_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
		MWPIXELVAL c);
MWBOOL	set_subdriver(PSD psd, PSUBDRIVER subdriver, MWBOOL init);
void	get_subdriver(PSD psd, PSUBDRIVER subdriver);

/* fb.c*/
PSUBDRIVER select_fb_subdriver(PSD psd);
void set_portrait_subdriver(PSD psd);
