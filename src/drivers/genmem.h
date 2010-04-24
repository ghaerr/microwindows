/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * Microwindows memory device routines header file
 */

/* entry points*/

/* genmem.c*/
PSD 	gen_allocatememgc(PSD psd);
void	gen_initmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
			int linelen,int size,void *addr);
MWBOOL	gen_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
			int size,void *addr);
void	gen_freememgc(PSD mempsd);
void	gen_setportrait(PSD psd, int portraitmode);
