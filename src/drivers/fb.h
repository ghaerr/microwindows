/*
 * Copyright (c) 1999, 2000, 2001 Greg Haerr <greg@censoft.com>
 *
 * Framebuffer drivers header file for Microwindows Screen Drivers
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
typedef unsigned long *		ADDR32;

/* ROP macro for 16 drawing modes*/
#define CHECK(f,d)	

/* applyOp w/stored dst*/
#define	applyOp(op, src, pdst, type)		\
{						\
	type d = (pdst);			\
	switch (op) {				\
	case MWMODE_XOR:			\
		*d ^= (src);			\
		CHECK("XOR", *d);		\
		break;				\
	case MWMODE_AND:			\
		*d &= (src);			\
		CHECK("AND", *d);		\
		break;				\
	case MWMODE_OR:				\
		*d |= (src);			\
		CHECK("OR", *d);		\
		break;				\
	case MWMODE_CLEAR:			\
		*d = 0;				\
		CHECK("CLEAR", *d);		\
		break;				\
	case MWMODE_SETTO1:			\
		*d = -1;			\
		CHECK("SETTO1", *d);		\
		break;				\
	case MWMODE_EQUIV:			\
		*d = ~(*d ^ (src));		\
		CHECK("EQUIV", *d);		\
		break;				\
	case MWMODE_NOR:			\
		*d = ~(*d | (src));		\
		CHECK("NOR", *d);		\
		break;				\
	case MWMODE_NAND:			\
		*d = ~(*d & (src));		\
		CHECK("NAND", *d);		\
		break;				\
	case MWMODE_INVERT:			\
		*d = ~*d;			\
		CHECK("INVERT", *d);		\
		break;				\
	case MWMODE_COPYINVERTED:		\
		*d = ~(src);			\
		CHECK("COPYINVERTED", *d);	\
		break;				\
	case MWMODE_ORINVERTED:			\
		*d |= ~(src);			\
		CHECK("ORINVERTED", *d);	\
		break;				\
	case MWMODE_ANDINVERTED:		\
		*d &= ~(src);			\
		CHECK("ANDINVERTED", *d);	\
		break;				\
	case MWMODE_ORREVERSE:			\
		*d = ~*d | (src);		\
		CHECK("ORREVERSE", *d);		\
		break;				\
	case MWMODE_ANDREVERSE:			\
		*d = ~*d & (src);		\
		CHECK("ANDREVERSE", *d);	\
		break;				\
	case MWMODE_COPY:			\
		*d = (src);			\
		CHECK("COPY", *d);		\
		break;				\
	case MWMODE_NOOP:			\
		CHECK("NOOP", *d);		\
		break;				\
	}					\
}

/* applyOp w/return value*/
#define DEFINE_applyOpR				\
static inline int applyOpR(op, src, dst)	\
{						\
	switch (op) {				\
	case MWMODE_XOR:			\
		return (src) ^ (dst);		\
	case MWMODE_AND:			\
		return (src) & (dst);		\
	case MWMODE_OR:				\
		return (src) | (dst);		\
	case MWMODE_CLEAR:			\
		return 0;			\
	case MWMODE_SETTO1:			\
		return -1;			\
	case MWMODE_EQUIV:			\
		return ~((src) ^ (dst));	\
	case MWMODE_NOR:			\
		return ~((src) | (dst));	\
	case MWMODE_NAND:			\
		return ~((src) & (dst));	\
	case MWMODE_INVERT:			\
		return ~(dst);			\
	case MWMODE_COPYINVERTED:		\
		return ~(src);			\
	case MWMODE_ORINVERTED:			\
		return ~(src) | (dst);		\
	case MWMODE_ANDINVERTED:		\
		return ~(src) & (dst);		\
	case MWMODE_ORREVERSE:			\
		return (src) | ~(dst);		\
	case MWMODE_ANDREVERSE:			\
		return (src) & ~(dst);		\
	case MWMODE_COPY:			\
		return (src);			\
	case MWMODE_NOOP:			\
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

/* genmem.c*/
void	gen_fillrect(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,MWCOORD y2,
		MWPIXELVAL c);
MWBOOL	set_subdriver(PSD psd, PSUBDRIVER subdriver, MWBOOL init);
void	get_subdriver(PSD psd, PSUBDRIVER subdriver);

/* fb.c*/
PSUBDRIVER select_fb_subdriver(PSD psd);
