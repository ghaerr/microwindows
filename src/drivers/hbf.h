/*
 *	Two interfaces to HBF files -- take your pick.
 *
 *	Ross Paterson <rap@doc.ic.ac.uk>
 */
#ifndef _HBF_
#define _HBF_

#ifndef __STDC__
#	ifndef const
#		define const
#	endif
#endif

/*
 *	#1: a lightweight C interface.
 */

typedef	unsigned int	HBF_CHAR;

typedef struct {
	unsigned short	hbf_width;
	unsigned short	hbf_height;
	short		hbf_xDisplacement;
	short		hbf_yDisplacement;
} HBF_BBOX;

typedef struct {
	/* fields corresponding to the definition */
	HBF_BBOX	hbf_bitmap_bbox;	/* HBF_BITMAP_BOUNDING_BOX */
	HBF_BBOX	hbf_font_bbox;		/* FONTBOUNDINGBOX */
} HBF;

extern	HBF *hbfOpen(
#ifdef __STDC__
			const	char	*filename
#endif
		);

extern	void	hbfClose(
#ifdef __STDC__
			HBF		*hbf
#endif
		);

extern	const	char	*hbfProperty(
#ifdef __STDC__
			HBF		*hbf,
			const	char	*propName
#endif
		);

extern	const	unsigned char	*hbfGetBitmap(
#ifdef __STDC__
			HBF		*hbf,
			HBF_CHAR	code
#endif
		);

extern	void	hbfForEach(
#ifdef __STDC__
			HBF	*hbf,
			void	(*func)(HBF *sameHbf, HBF_CHAR code)
#endif
		);

extern	const	char	*hbfFileName(
#ifdef __STDC__
			HBF	*hbf
#endif
		);

extern	long	hbfChars(
#ifdef __STDC__
			HBF	*hbf
#endif
		);

extern	HBF_BBOX *hbfBitmapBBox(
#ifdef __STDC__
			HBF	*hbf
#endif
		);
/* but defined here as a macro */
#define	hbfBitmapBBox(hbf)	(&((hbf)->hbf_bitmap_bbox))

extern	HBF_BBOX *hbfFontBBox(
#ifdef __STDC__
			HBF	*hbf
#endif
		);
/* but defined here as a macro */
#define	hbfFontBBox(hbf)	(&((hbf)->hbf_font_bbox))

#define	HBF_RowSize(hbf)\
	((hbfBitmapBBox(hbf)->hbf_width + 7)/8)

#define	HBF_BitmapSize(hbf)\
	(HBF_RowSize(hbf) * hbfBitmapBBox(hbf)->hbf_height)

#define	HBF_GetBit(hbf,bitmap,x,y)\
	(((bitmap)[(y)*HBF_RowSize(hbf) + (x)/8]>>(7 - (x)%8))&01)

extern	int	hbfDebug;	/* set non-zero for error reporting */

/*
 *	#2: taken from Appendix 2 of the HBF draft.
 */

typedef	unsigned int	HBF_HzCode;
typedef unsigned char	HBF_Byte ;
typedef HBF_Byte *	HBF_BytePtr ;
typedef HBF *	        HBF_Handle ;
typedef HBF_Handle *    HBF_HandlePtr ;
typedef char *	        String ;

extern	int	HBF_OpenFont(
#ifdef __STDC__
		const	char *        filename,
		HBF_HandlePtr ptrHandleStorage
#endif
);

extern	int	HBF_CloseFont(
#ifdef __STDC__
		HBF_Handle handle
#endif
);

extern	const char * HBF_GetProperty(
#ifdef __STDC__
		HBF_Handle	handle,
		const	char *	propertyName
#endif
);

extern	int	HBF_GetFontBoundingBox(
#ifdef __STDC__
		HBF_Handle   handle,
		unsigned int *width,
		unsigned int *height,
		int *xDisplacement,
		int *yDisplacement
#endif
);

extern	int	HBF_GetBitmapBoundingBox(
#ifdef __STDC__
		HBF_Handle   handle,
		unsigned int *width,
		unsigned int *height,
		int *xDisplacement,
		int *yDisplacement
#endif
);

extern	int	HBF_GetBitmap(
#ifdef __STDC__
		HBF_Handle  handle,
		HBF_HzCode  hanziCode,
		HBF_BytePtr ptrBitmapBuffer
#endif
);

#endif /* ! _HBF_ */
