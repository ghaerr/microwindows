/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * Microwindows memory device routines header file
 */

/* entry points*/

/* genmem.c*/
PSD		GdCreatePixmap(PSD rootpsd, MWCOORD width, MWCOORD height, int format, void *pixels);
PSD 	gen_allocatememgc(PSD psd);
MWBOOL	gen_mapmemgc(PSD mempsd, MWCOORD w, MWCOORD h, int planes, int bpp, int data_format,
			int linelen, int pitch, int size, void *addr);
void	gen_freememgc(PSD mempsd);
int		GdCalcMemGCAlloc(PSD psd, unsigned int width, unsigned int height,
			int planes, int bpp, int *size, int *linelen, int *pitch);

void	gen_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c);

void	gen_setportrait(PSD psd, int portraitmode);
void	set_portrait_subdriver(PSD psd);

MWBOOL	set_subdriver(PSD psd, PSUBDRIVER subdriver, MWBOOL init);
void	get_subdriver(PSD psd, PSUBDRIVER subdriver);
