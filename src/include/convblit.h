/*
 * Conversion blits - public header file
 */


/* convblit_8888.c*/

/* ----- 32bpp output -----*/
void convblit_srcover_rgba8888_bgra8888(PSD psd, PMWBLITPARMS gc);	// png/tiff 32bpp RGBA srcover
void convblit_srcover_rgba8888_bgra8888_left(PSD psd, PMWBLITPARMS gc);
void convblit_srcover_rgba8888_bgra8888_right(PSD psd, PMWBLITPARMS gc);
void convblit_srcover_rgba8888_bgra8888_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_rgb888_bgra8888(PSD psd, PMWBLITPARMS gc);		// png/jpg 24bpp RGB copy
void convblit_copy_rgb888_bgra8888_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_rgb888_bgra8888_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_rgb888_bgra8888_down(PSD psd, PMWBLITPARMS gc);

/* GdArea auto-portait blits*/
void convblit_copy_8888_8888(PSD psd, PMWBLITPARMS gc);				// 32bpp to 32bpp copy
void convblit_copy_rgba8888_bgra8888(PSD psd, PMWBLITPARMS gc);		// 32bpp RGBA to 32bpp BGRA copy

/* ----- 24bpp output -----*/
void convblit_srcover_rgba8888_bgr888(PSD psd, PMWBLITPARMS gc);
void convblit_srcover_rgba8888_bgr888_left(PSD psd, PMWBLITPARMS gc);
void convblit_srcover_rgba8888_bgr888_right(PSD psd, PMWBLITPARMS gc);
void convblit_srcover_rgba8888_bgr888_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_rgb888_bgr888(PSD psd, PMWBLITPARMS gc);
void convblit_copy_rgb888_bgr888_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_rgb888_bgr888_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_rgb888_bgr888_down(PSD psd, PMWBLITPARMS gc);

/* GdArea auto-portait blits*/
void convblit_copy_888_888(PSD psd, PMWBLITPARMS gc);				// 24bpp to 24bpp copy
void convblit_copy_bgra8888_bgr888(PSD psd, PMWBLITPARMS gc);		// 32bpp BGRA to 24bpp BGR copy
void convblit_copy_rgba8888_bgr888(PSD psd, PMWBLITPARMS gc);		// 32bpp RGBA to 24bpp BGR copy

/* ----- 16bpp output -----*/
void convblit_srcover_rgba8888_16bpp(PSD psd, PMWBLITPARMS gc);
void convblit_srcover_rgba8888_16bpp_left(PSD psd, PMWBLITPARMS gc);
void convblit_srcover_rgba8888_16bpp_right(PSD psd, PMWBLITPARMS gc);
void convblit_srcover_rgba8888_16bpp_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_rgb888_16bpp(PSD psd, PMWBLITPARMS gc);
void convblit_copy_rgb888_16bpp_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_rgb888_16bpp_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_rgb888_16bpp_down(PSD psd, PMWBLITPARMS gc);

/* GdArea auto-portait blits*/
void convblit_copy_16bpp_16bpp(PSD psd, PMWBLITPARMS gc);			// 16bpp to 16bpp copy
void convblit_copy_rgba8888_16bpp(PSD psd, PMWBLITPARMS gc);		// 32bpp RGBA to 16bpp copy

/* convblit_mask.c*/
/* 1bpp and 8bpp (alphablend) mask conversion blits - for font display*/

/* ----- 32bpp output -----*/
void convblit_copy_mask_mono_byte_msb_bgra(PSD psd, PMWBLITPARMS gc);		/* ft2 non-alias*/
void convblit_copy_mask_mono_byte_msb_bgra_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_msb_bgra_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_msb_bgra_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_mask_mono_byte_lsb_bgra(PSD psd, PMWBLITPARMS gc);		/* t1lib non-alias*/
void convblit_copy_mask_mono_byte_lsb_bgra_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_lsb_bgra_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_lsb_bgra_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_mask_mono_word_msb_bgra(PSD psd, PMWBLITPARMS gc);		/* pcf non-alias*/
void convblit_copy_mask_mono_word_msb_bgra_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_word_msb_bgra_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_word_msb_bgra_down(PSD psd, PMWBLITPARMS gc);

void convblit_blend_mask_alpha_byte_bgra(PSD psd, PMWBLITPARMS gc);			/* ft2/t1lib alias*/
void convblit_blend_mask_alpha_byte_bgra_left(PSD psd, PMWBLITPARMS gc);
void convblit_blend_mask_alpha_byte_bgra_right(PSD psd, PMWBLITPARMS gc);
void convblit_blend_mask_alpha_byte_bgra_down(PSD psd, PMWBLITPARMS gc);

/* ----- 24bpp output -----*/
void convblit_copy_mask_mono_byte_msb_bgr(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_msb_bgr_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_msb_bgr_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_msb_bgr_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_mask_mono_byte_lsb_bgr(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_lsb_bgr_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_lsb_bgr_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_lsb_bgr_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_mask_mono_word_msb_bgr(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_word_msb_bgr_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_word_msb_bgr_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_word_msb_bgr_down(PSD psd, PMWBLITPARMS gc);

void convblit_blend_mask_alpha_byte_bgr(PSD psd, PMWBLITPARMS gc);
void convblit_blend_mask_alpha_byte_bgr_left(PSD psd, PMWBLITPARMS gc);
void convblit_blend_mask_alpha_byte_bgr_right(PSD psd, PMWBLITPARMS gc);
void convblit_blend_mask_alpha_byte_bgr_down(PSD psd, PMWBLITPARMS gc);

/* ----- 16bpp output -----*/
void convblit_copy_mask_mono_byte_msb_16bpp(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_msb_16bpp_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_msb_16bpp_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_msb_16bpp_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_mask_mono_byte_lsb_16bpp(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_lsb_16bpp_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_lsb_16bpp_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_byte_lsb_16bpp_down(PSD psd, PMWBLITPARMS gc);

void convblit_copy_mask_mono_word_msb_16bpp(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_word_msb_16bpp_left(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_word_msb_16bpp_right(PSD psd, PMWBLITPARMS gc);
void convblit_copy_mask_mono_word_msb_16bpp_down(PSD psd, PMWBLITPARMS gc);

void convblit_blend_mask_alpha_byte_16bpp(PSD psd, PMWBLITPARMS gc);
void convblit_blend_mask_alpha_byte_16bpp_left(PSD psd, PMWBLITPARMS gc);
void convblit_blend_mask_alpha_byte_16bpp_right(PSD psd, PMWBLITPARMS gc);
void convblit_blend_mask_alpha_byte_16bpp_down(PSD psd, PMWBLITPARMS gc);

#if LATER
void convblit_copy_mask_mono_byte_msb_bgra_large(PSD psd, PMWBLITPARMS gc);	/* ft2 non-alias*/
void convblit_copy_mask_mono_byte_lsb_bgra_large(PSD psd, PMWBLITPARMS gc);	/* t1lib non-alias*/
#endif
