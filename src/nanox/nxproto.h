/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 *
 * Nano-X Core Protocol Header
 * 
 * These structures define the Nano-X client/server protocol.
 * Much of this has been modeled after the X11 implementation.
 * Note that all NX Protocol requests should have all data quantities
 * properly aligned. This is assured by hand-coding each NX request
 * structure.  Unlike Xlib, fixed size request structs don't have to
 * be a multiple of 4 bytes, since the length field is a byte count
 * and GetReq() automatically pads all requests to 4-byte boundaries.
 * Request structs for variable size data, however, must be hand-padded
 * to 4-byte alignment, as variable data starts after sizeof(structure).
 * Also, the hilength/length fields store the unaligned byte count, so
 * that extra code isn't required to de-crypt extra data size or
 * big packets.
 */

/*
 * The following is provided to allow limiting the maximum
 * request size that will be sent (not received) using this protocol.
 * The protocol allows for 2^24 byte maximum, but the
 * server currently allocates the MAXREQUESTSZ in a stack buffer.
 * Also, the client realloc's the request queue to 
 * the largest size asked for, and currently never reduces it.
 *
 * Routines like GrArea will split packets to be <= MAXREQUESTSZ
 * automatically.
 *
 * NOTE: MAXREQUESTSZ must be an _aligned_ multiple of 4, meaning
 * that MAXREQUESTSZ = (MAXREQUESTSZ + 3) & ~3.
 */
#define MAXREQUESTSZ	30000		/* max request size (65532)*/

typedef unsigned char	BYTE8;		/* 1 byte*/
typedef unsigned short	UINT16;		/* 2 bytes*/
typedef short		INT16;		/* 2 bytes*/
typedef unsigned long	UINT32;		/* 4 bytes*/

#if ELKS
typedef UINT16		IDTYPE;
#define ALIGNSZ		2	/* 2 byte packet alignment*/
#else
typedef UINT32		IDTYPE;
#define ALIGNSZ		4	/* 4 byte packet alignment*/
#endif

/* all requests share this header*/
typedef struct {
	BYTE8	reqType;	/* request code*/
	BYTE8	hilength;	/* upper 24 bits of unaligned length*/
	UINT16	length;		/* lower 16 bits of unaligned length*/
} nxReq;

/* Allocate a fixed size request from request buffer*/
#define AllocReq(name) \
	((nx##name##Req *)nxAllocReq(GrNum##name,sizeof(nx##name##Req), 0))

/* Allocate a request, but allocate n extra bytes*/
#define AllocReqExtra(name,n) \
	((nx##name##Req *)nxAllocReq(GrNum##name,sizeof(nx##name##Req), n))

/* return pointer to variable length data*/
#define GetReqData(req)		((void *)((char *)req + sizeof(* (req))))

/* FIXME fails when sizeof(int) == 2*/
/* get request total valid data length, including header*/
#define GetReqLen(req)		(((req)->hilength << 16) | (req)->length)

/* get request variable data length, not including fixed size structure*/
#define GetReqVarLen(req)	(GetReqLen(req) - sizeof(* (req)))

/* get request total aligned length*/
#define GetReqAlignedLen(req)	((GetReqLen(req) + (ALIGNSZ-1)) & ~(ALIGNSZ-1))

void * 	nxAllocReq(int type, long size, long extra);
void	nxFlushReq(long newsize, int reply_needed);
void 	nxAssignReqbuffer(char *buffer, long size);
void 	nxWriteSocket(char *buf, int todo);
int	nxCalcStringBytes(void *str, int count, int flags);

#if notyet
/* all replies share this header*/
typedef struct {
	BYTE8	repType;	/* reply code*/
	BYTE8	hilength;	/* upper 24 bits of unaligned length*/
	UINT16	length;		/* lower 16 bits of unaligned length*/
} nxReply;

/* reply types if not equal to request type*/
#define GrNumErrorReply		255
#define GrNumEventReply		254
#endif /* notyet*/

#define GrNumOpen               0
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT32	pid;
} nxOpenReq;

#define GrNumClose              1
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxCloseReq;

#define GrNumGetScreenInfo      2
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxGetScreenInfoReq;

#define GrNumNewWindow          3
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	parentid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
	UINT32	backgroundcolor;
	UINT32	bordercolor;
	INT16	bordersize;
} nxNewWindowReq;

#define GrNumNewInputWindow     4
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	parentid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
} nxNewInputWindowReq;

#define GrNumDestroyWindow      5
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxDestroyWindowReq;

#define GrNumNewGC              6
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxNewGCReq;

#define GrNumCopyGC		7
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
} nxCopyGCReq;

#define GrNumGetGCInfo          8
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
} nxGetGCInfoReq;

#define GrNumDestroyGC          9
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
} nxDestroyGCReq;

#define GrNumMapWindow          10
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxMapWindowReq;

#define GrNumUnmapWindow        11
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxUnmapWindowReq;

#define GrNumRaiseWindow        12
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxRaiseWindowReq;

#define GrNumLowerWindow        13
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxLowerWindowReq;

#define GrNumMoveWindow         14
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
	INT16	x;
	INT16	y;
} nxMoveWindowReq;

#define GrNumResizeWindow       15
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
	INT16	width;
	INT16	height;
} nxResizeWindowReq;

#define GrNumGetWindowInfo      16
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxGetWindowInfoReq;

#define GrNumGetFontInfo        17
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	fontid;
} nxGetFontInfoReq;

#define GrNumSetFocus           18
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxSetFocusReq;

#define GrNumSetWindowCursor    19
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
	IDTYPE	cursorid;
} nxSetWindowCursorReq;

#define GrNumClearArea          20
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
	UINT16	x;
	UINT16	y;
	UINT16	width;
	UINT16	height;
	UINT16	exposeflag;
} nxClearAreaReq;

#define GrNumSelectEvents       21
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
	UINT32	eventmask;
} nxSelectEventsReq;

#define GrNumGetNextEvent       22
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxGetNextEventReq;

#define GrNumCheckNextEvent     23
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxCheckNextEventReq;

#define GrNumPeekEvent          24
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxPeekEventReq;

#define GrNumLine               25
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x1;
	INT16	y1;
	INT16	x2;
	INT16	y2;
} nxLineReq;

#define GrNumPoint              26
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
} nxPointReq;

#define GrNumRect               27
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
} nxRectReq;

#define GrNumFillRect           28
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
} nxFillRectReq;

#define GrNumPoly               29
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	/*INT16 pointtable[];*/
} nxPolyReq;

#define GrNumFillPoly           30
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	/*INT16 pointtable[];*/
} nxFillPolyReq;

#define GrNumEllipse            31
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	rx;
	INT16	ry;
} nxEllipseReq;

#define GrNumFillEllipse        32
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	rx;
	INT16	ry;
} nxFillEllipseReq;

#define GrNumSetGCForeground    33
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
	UINT32	color;
} nxSetGCForegroundReq;

#define GrNumSetGCBackground    34
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
	UINT32	color;
} nxSetGCBackgroundReq;

#define GrNumSetGCUseBackground 35
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
	UINT16	flag;
} nxSetGCUseBackgroundReq;

#define GrNumSetGCMode          36
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
	UINT16	mode;
} nxSetGCModeReq;

#define GrNumSetGCFont          37
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
	IDTYPE	fontid;
} nxSetGCFontReq;

#define GrNumGetGCTextSize      38
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
	UINT16	flags;
	UINT16	pad;
	/*BYTE8	text[];*/
} nxGetGCTextSizeReq;

#define GrNumReadArea           39
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
} nxReadAreaReq;

#define GrNumArea               40
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
	INT16	pixtype;
	INT16	pad;
	/*UINT32 pixels[];*/
} nxAreaReq;

#define GrNumBitmap             41
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
	/*UINT16 bitmaptable[];*/
} nxBitmapReq;

#define GrNumText               42
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	count;
	INT16	flags;
	/*BYTE8	text[];*/
} nxTextReq;

#define GrNumNewCursor          43
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	INT16	width;
	INT16	height;
	INT16	hotx;
	INT16	hoty;
	UINT32	fgcolor;
	UINT32	bgcolor;
	/*UINT16 fgbitmap[];*/
	/*UINT16 bgbitmap[];*/
} nxNewCursorReq;

#define GrNumMoveCursor         44
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	INT16	x;
	INT16	y;
} nxMoveCursorReq;

#define GrNumGetSystemPalette      45
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxGetSystemPaletteReq;

#define GrNumFindColor             46
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT32	color;
} nxFindColorReq;

#define GrNumReparentWindow        47
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
	IDTYPE	parentid;
	INT16	x;
	INT16	y;
} nxReparentWindowReq;

#define GrNumDrawImageFromFile     48
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
	IDTYPE	flags;
	/*char path[];*/
} nxDrawImageFromFileReq;

#define GrNumLoadImageFromFile     49
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	INT16	flags;
	INT16	pad;
} nxLoadImageFromFileReq;

#define GrNumNewPixmap          50
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	INT16	width;
	INT16	height;
/* FIXME: Add support for passing shared memory info */
} nxNewPixmapReq;

#define GrNumCopyArea          51
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
	IDTYPE	srcid;
	INT16	srcx;
	INT16	srcy;
	UINT32	op;
} nxCopyAreaReq;

#define GrNumSetFontSize        52
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	fontid;
	INT16	fontsize;
} nxSetFontSizeReq;

#define GrNumCreateFont		53
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	INT16	height;
	INT16	lf_used;
	MWLOGFONT lf;
} nxCreateFontReq;

#define GrNumDestroyFont	54
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	fontid;
} nxDestroyFontReq;

#define GrNumReqShmCmds         55
typedef struct {
	BYTE8   reqType;
	BYTE8   hilength;
	UINT16  length;
	UINT32  size;
} nxReqShmCmdsReq;

#define GrNumShmCmdsFlush       56
typedef struct {
	BYTE8   reqType;
	BYTE8   hilength;
	UINT16  length;
	UINT32  size;
	UINT32  reply;
} nxShmCmdsFlushReq;

#define GrNumSetFontRotation    57
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	fontid;
	INT16	tenthdegrees;
} nxSetFontRotationReq;

#define GrNumSetFontAttr        58
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	fontid;
	INT16	setflags;
	INT16	clrflags;
} nxSetFontAttrReq;

#define GrNumSetSystemPalette   59
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	INT16	first;
	INT16	count;
	MWPALENTRY palette[256];
} nxSetSystemPaletteReq;

#define GrNumInjectEvent	60
#define GR_INJECT_EVENT_POINTER		0
#define GR_INJECT_EVENT_KEYBOARD	1
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	union {
		struct {
			INT16	x;
			INT16	y;
			UINT16	button;
			BYTE8	visible;
		} pointer;
		struct {
			IDTYPE	wid;
			UINT16	keyvalue;
			UINT16	modifier;
			BYTE8	scancode;
			BYTE8	pressed;
		} keyboard;
	} event;
	UINT16	event_type;
} nxInjectEventReq;

#define GrNumNewRegion		61
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxNewRegionReq;

#define GrNumDestroyRegion	62
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
} nxDestroyRegionReq;

#define GrNumUnionRectWithRegion	63
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
	GR_RECT	rect;
} nxUnionRectWithRegionReq;

#define GrNumUnionRegion	64
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
	IDTYPE	srcregionid1;
	IDTYPE	srcregionid2;
} nxUnionRegionReq;

#define GrNumIntersectRegion	65
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
	IDTYPE	srcregionid1;
	IDTYPE	srcregionid2;
} nxIntersectRegionReq;

#define GrNumSetGCRegion	66
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	gcid;
	IDTYPE	regionid;
} nxSetGCRegionReq;

#define GrNumSubtractRegion	67
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
	IDTYPE	srcregionid1;
	IDTYPE	srcregionid2;
} nxSubtractRegionReq;

#define GrNumXorRegion		68
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
	IDTYPE	srcregionid1;
	IDTYPE	srcregionid2;
} nxXorRegionReq;

#define GrNumPointInRegion	69
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
	INT16	x;
	INT16	y;
} nxPointInRegionReq;

#define GrNumRectInRegion	70
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
	INT16	x;
	INT16	y;
	INT16	w;
	INT16	h;
} nxRectInRegionReq;

#define GrNumEmptyRegion	71
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
} nxEmptyRegionReq;

#define GrNumEqualRegion	72
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	region1;
	IDTYPE	region2;
} nxEqualRegionReq;

#define GrNumOffsetRegion	73
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	region;
	INT16	dx;
	INT16	dy;
} nxOffsetRegionReq;

#define GrNumGetRegionBox	74
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	regionid;
} nxGetRegionBoxReq;

#define GrNumNewPolygonRegion	75
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT16	mode;
	UINT16	pad;
	/*INT16 points[];*/
} nxNewPolygonRegionReq;

#define GrNumArc		76
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	rx;
	INT16	ry;
	INT16	ax;
	INT16	ay;
	INT16	bx;
	INT16	by;
	INT16	type;
} nxArcReq;

#define GrNumArcAngle		77
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	rx;
	INT16	ry;
	INT16	angle1;
	INT16	angle2;
	INT16	type;
} nxArcAngleReq;

#define GrNumSetWMProperties	78
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
	/* GR_WM_PROPERTIES props */
	/* GR_CHAR *title */
} nxSetWMPropertiesReq;

#define GrNumGetWMProperties	79
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxGetWMPropertiesReq;

#define GrNumCloseWindow	80
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxCloseWindowReq;

#define GrNumKillWindow		81
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxKillWindowReq;

#define GrNumDrawImageToFit     82
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
	IDTYPE	imageid;
} nxDrawImageToFitReq;

#define GrNumFreeImage          83
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	id;
} nxFreeImageReq;

#define GrNumGetImageInfo       84
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	id;
} nxGetImageInfoReq;

#define GrNumDrawImageBits      85
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;		/* MWIMAGEHDR start*/
	INT16	height;
	INT16	planes;
	INT16	bpp;
	INT16	pitch;
	INT16	bytesperpixel;
	INT16	compression;
	INT16	palsize;
	UINT32	transcolor;
	/*MWIMAGEBITS imagebits[];*/
	/*MWPALENTRY palette[palsize];*/
} nxDrawImageBitsReq;

#define GrNumPoints             86
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	/*INT16 pointtable[];*/
} nxPointsReq;

#define GrNumGetFocus           87
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxGetFocusReq;

#define GrNumGetSysColor        88
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT16	index;
} nxGetSysColorReq;

#define GrNumSetScreenSaverTimeout	89
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT32	timeout;
} nxSetScreenSaverTimeoutReq;

#define GrNumSetSelectionOwner	90
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	wid;
	/* GR_CHAR *typelist */
} nxSetSelectionOwnerReq;

#define GrNumGetSelectionOwner	91
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxGetSelectionOwnerReq;

#define GrNumRequestClientData	92
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	wid;
	IDTYPE	rid;
	UINT32	serial;
	UINT16	mimetype;
} nxRequestClientDataReq;

#define GrNumSendClientData	93
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	wid;
	IDTYPE	did;
	UINT32	serial;
	UINT32	len;
	/* void *data */
} nxSendClientDataReq;

#define GrNumBell		94
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxBellReq;

#define GrNumSetBackgroundPixmap 95
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	wid;
	IDTYPE	pixmap;
	UINT32	flags;
} nxSetBackgroundPixmapReq;

#define GrNumDestroyCursor	96
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	cursorid;
} nxDestroyCursorReq;

#define GrNumQueryTree   	97
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	windowid;
} nxQueryTreeReq;

#define GrNumCreateTimer	98
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	wid;
	UINT32	period;
} nxCreateTimerReq;

#define GrNumDestroyTimer	99
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	timerid;
} nxDestroyTimerReq;

#define GrNumSetPortraitMode	100
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT32	portraitmode;
} nxSetPortraitModeReq;

#define GrNumImageBufferAlloc   101

typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT32	size;
} nxImageBufferAllocReq;

#define GrNumImageBufferSend    102

typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT32	buffer_id;
	UINT32	size;
} nxImageBufferSendReq;

#define GrNumLoadImageFromBuffer 103
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	UINT32	buffer;
	INT16	flags;
	INT16	pad;
} nxLoadImageFromBufferReq;

#define GrNumDrawImageFromBuffer 104
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE	drawid;
	IDTYPE	gcid;
	INT16	x;
	INT16	y;
	INT16	width;
	INT16	height;
	UINT32	buffer;
	IDTYPE	flags;
} nxDrawImageFromBufferReq;

#define GrNumGetFontList        105
typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
} nxGetFontListReq;

#define GrNumSetGCClipOrigin    106

typedef struct {
	BYTE8	reqType;
	BYTE8	hilength;
	UINT16	length;
	IDTYPE  gcid;
	UINT32  xoff;
	UINT32  yoff;
} nxSetGCClipOriginReq;

#define GrTotalNumCalls         107
