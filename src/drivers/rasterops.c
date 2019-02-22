/*
 * Fast Raster Operations for Microwindows - used in fblin1.c driver
 *
 * GdRasterOps() - Perform any of 16 base MWROP raster operations on
 *	DWORD-aligned MWIMAGEHDR data (pixmaps).  All data formats supported,
 *  with all bits modified.  Operates on 32-bit quantities at all times,
 *  using masks when required.
 *
 * 1/7/2011 g haerr
 *
 * The author gratefully acknowledges the work of Dan Bloomberg
 * at http://www.leptonica.com for his most excellent image processing
 * library. His raster ops implementation is used here with to provide
 * a general purpose raster ops blitter. Thanks Dan!
 *
 * Porting to Microwindows required typedefs for l_int32, pix* accessors,
 * and changing masks to allow little endian image storage.
 */
/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/
#include <stdio.h>
#include <assert.h>
#include "device.h"
#include "fb.h"
#include "swap.h"

/* typedefs and defines for Microwindows port*/
typedef int32_t		l_int32;
typedef uint32_t	l_uint32;

#define pixGetDepth(pimage)		(pimage->bpp)
#define pixGetWidth(pimage)		(pimage->width)
#define pixGetHeight(pimage)	(pimage->height)
#define pixGetWpl(pimage)		(pimage->pitch>>2)
#define pixGetData(pimage)		((l_uint32 *)pimage->imagebits)

/* fwd decls*/
static void rasteropUniLow(l_uint32 *datad, l_int32 dpixw, l_int32 dpixh,
				l_int32 depth, l_int32 dwpl, l_int32 dx, l_int32 dy,
				l_int32 dw, l_int32 dh, l_int32 op);
static void rasteropLow(l_uint32 *datad, l_int32 dpixw, l_int32 dpixh,
				l_int32 depth, l_int32 dwpl, l_int32 dx, l_int32 dy,
				l_int32 dw, l_int32 dh, l_int32 op,
				l_uint32 *datas, l_int32 spixw, l_int32 spixh,
				l_int32 swpl, l_int32 sx, l_int32 sy);

/*-------------------------------------------------------------------------*
 * Extracted from pix.h
 *
 * The following operation bit flags have been modified from
 * Sun's pixrect.h.
 *
 * The 'op' in 'rasterop' is represented by an integer
 * composed with Boolean functions using the set of five integers
 * given below.  The integers, and the op codes resulting from
 * boolean expressions on them, need only be in the range from 0 to 15.
 * The function is applied on a per-pixel basis.
 *
 * Examples: the op code representing ORing the src and dest
 * is computed using the bit OR, as PIX_SRC | PIX_DST;  the op
 * code representing XORing src and dest is found from
 * PIX_SRC ^ PIX_DST;  the op code representing ANDing src and dest
 * is found from PIX_SRC & PIX_DST.  Note that
 * PIX_NOT(PIX_CLR) = PIX_SET, and v.v., as they must be.
 *
 * We would like to use the following set of definitions:
 *
 *      #define   PIX_SRC      0xc
 *      #define   PIX_DST      0xa
 *      #define   PIX_NOT(op)  ((op) ^ 0xf)
 *      #define   PIX_CLR      0x0
 *      #define   PIX_SET      0xf
 *
 * Now, these definitions differ from Sun's, in that Sun
 * left-shifted each value by 1 pixel, and used the least
 * significant bit as a flag for the "pseudo-operation" of
 * clipping.  We don't need this bit, because it is both
 * efficient and safe ALWAYS to clip the rectangles to the src
 * and dest images, which is what we do.  See the notes in rop.h
 * on the general choice of these bit flags.
 *
 * However, if you include Sun's xview package, you will get their
 * definitions, and because I like using these flags, we will
 * adopt the original Sun definitions to avoid redefinition conflicts.
 *
 * Then we have, for reference, the following 16 unique op flags:
 *
 *      PIX_CLR                           00000             0x0
 *      PIX_SET                           11110             0x1e
 *      PIX_SRC                           11000             0x18
 *      PIX_DST                           10100             0x14
 *      PIX_NOT(PIX_SRC)                  00110             0x06
 *      PIX_NOT(PIX_DST)                  01010             0x0a
 *      PIX_SRC | PIX_DST                 11100             0x1c
 *      PIX_SRC & PIX_DST                 10000             0x10
 *      PIX_SRC ^ PIX_DST                 01100             0x0c
 *      PIX_NOT(PIX_SRC) | PIX_DST        10110             0x16
 *      PIX_NOT(PIX_SRC) & PIX_DST        00100             0x04
 *      PIX_SRC | PIX_NOT(PIX_DST)        11010             0x1a
 *      PIX_SRC & PIX_NOT(PIX_DST)        01000             0x08
 *      PIX_NOT(PIX_SRC | PIX_DST)        00010             0x02
 *      PIX_NOT(PIX_SRC & PIX_DST)        01110             0x0e
 *      PIX_NOT(PIX_SRC ^ PIX_DST)        10010             0x12
 *
 *-------------------------------------------------------------------------*/
#define   PIX_SRC      (0xc << 1)
#define   PIX_DST      (0xa << 1)
#define   PIX_NOT(op)  ((op) ^ 0x1e)
#define   PIX_CLR      (0x0 << 1)
#define   PIX_SET      (0xf << 1)

#define   PIX_PAINT    (PIX_SRC | PIX_DST)
#define   PIX_MASK     (PIX_SRC & PIX_DST)
#define   PIX_SUBTRACT (PIX_DST & PIX_NOT(PIX_SRC))
#define   PIX_XOR      (PIX_SRC ^ PIX_DST)

/* raster op translation table from Microwindows to internal PIX_ format*/
static int raster_ops[MWROP_MAX+1] = {
	PIX_SRC,						/* MWROP_COPY*/
	PIX_SRC ^ PIX_DST,				/* MWROP_XOR*/
	PIX_SRC | PIX_DST,				/* MWROP_OR*/
	PIX_SRC & PIX_DST,				/* MWROP_AND*/
	PIX_CLR,						/* MWROP_CLEAR*/
	PIX_SET,						/* MWROP_SET*/
	PIX_NOT(PIX_SRC ^ PIX_DST),		/* MWROP_EQUIV*/
	PIX_NOT(PIX_SRC | PIX_DST),		/* MWROP_NOR*/
	PIX_NOT(PIX_SRC & PIX_DST),		/* MWROP_NAND*/
	PIX_NOT(PIX_DST),				/* MWROP_INVERT*/
	PIX_NOT(PIX_SRC),				/* MWROP_COPYINVERTED*/
	PIX_NOT(PIX_SRC) | PIX_DST,		/* MWROP_ORINVERTED*/
	PIX_NOT(PIX_SRC) & PIX_DST,		/* MWROP_ANDINVERTED*/
	PIX_SRC | PIX_NOT(PIX_DST),		/* MWROP_ORREVERSE*/
	PIX_SRC & PIX_NOT(PIX_DST),		/* MWROP_ANDREVERSE*/
	PIX_DST,						/* MWROP_NOOP*/
	PIX_SRC ^ PIX_DST,				/* substitute XOR for XOR_FGBG*/
	PIX_SRC,						/* substitute COPY for SRC_OVER*/
	PIX_DST,						/* substitute NOOP for DST_OVER*/
	PIX_SRC,						/* substitute COPY for SRC_IN*/
	PIX_DST,						/* substitute NOOP for DST_IN*/
	PIX_DST,						/* substitute NOOP for SRC_OUT*/
	PIX_SRC,						/* substitute COPY for DST_OUT*/
	PIX_SRC,						/* substitute COPY for SRC_ATOP*/
	PIX_DST,						/* substitute NOOP for DST_ATOP*/
	PIX_SRC ^ PIX_DST,				/* substitute XOR for PORTERDUFF_XOR*/
	PIX_SRC							/* substitute COPY for SRC_TRANSCOPY*/
};

/*--------------------------------------------------------------------*
 * rop.c -        General rasterop (basic pix interface)              *
 *--------------------------------------------------------------------*/
/*!
 *  GdRasterOp() (old pixRasterop)
 *
 *      Input:  pixd   (dest pix)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *              pixs   (src pix)
 *              sx     (x val of UL corner of src rectangle)
 *              sy     (y val of UL corner of src rectangle)
 *      Return: 0 if OK; 1 on error.
 *
 *  Notes:
 *      (1) This has the standard set of 9 args for rasterop.
 *          This function is your friend; it is worth memorizing!
 *      (2) If the operation involves only dest, this calls
 *          rasteropUniLow().  Otherwise, checks depth of the
 *          src and dest, and if they match, calls rasteropLow().
 *      (3) For the two-image operation, where both pixs and pixd
 *          are defined, they are typically different images.  However
 *          there are cases, such as pixSetMirroredBorder(), where
 *          in-place operations can be done, blitting pixels from
 *          one part of pixd to another.  Consequently, we permit
 *          such operations.  If you use them, be sure that there
 *          is no overlap between the source and destination rectangles
 *          in pixd (!)
 *
 *  Background:
 *  -----------
 *
 *  There are 18 operations, described by the op codes in pix.h.
 *
 *  One, PIX_DST, is a no-op.
 *
 *  Three, PIX_CLR, PIX_SET, and PIX_NOT(PIX_DST) operate only on the dest.
 *  These are handled by the low-level rasteropUniLow().
 *
 *  The other 14 involve the both the src and the dest, and depend on
 *  the bit values of either just the src or the bit values of both
 *  src and dest.  They are handled by rasteropLow():
 *
 *          PIX_SRC                             s
 *          PIX_NOT(PIX_SRC)                   ~s
 *          PIX_SRC | PIX_DST                   s | d
 *          PIX_SRC & PIX_DST                   s & d
 *          PIX_SRC ^ PIX_DST                   s ^ d
 *          PIX_NOT(PIX_SRC) | PIX_DST         ~s | d
 *          PIX_NOT(PIX_SRC) & PIX_DST         ~s & d
 *          PIX_NOT(PIX_SRC) ^ PIX_DST         ~s ^ d
 *          PIX_SRC | PIX_NOT(PIX_DST)          s | ~d
 *          PIX_SRC & PIX_NOT(PIX_DST)          s & ~d
 *          PIX_SRC ^ PIX_NOT(PIX_DST)          s ^ ~d
 *          PIX_NOT(PIX_SRC | PIX_DST)         ~(s | d)
 *          PIX_NOT(PIX_SRC & PIX_DST)         ~(s & d)
 *          PIX_NOT(PIX_SRC ^ PIX_DST)         ~(s ^ d)
 *
 *  Each of these is implemented with one of three low-level
 *  functions, depending on the alignment of the left edge
 *  of the src and dest rectangles:
 *      * a fastest implementation if both left edges are
 *        (32-bit) word aligned
 *      * a very slightly slower implementation if both left
 *        edges have the same relative (32-bit) word alignment
 *      * the general routine that is invoked when
 *        both left edges have different word alignment
 *
 *  Of the 14 binary rasterops above, only 12 are unique
 *  logical combinations (out of a possible 16) of src
 *  and dst bits:
 *
 *        (sd)         (11)   (10)   (01)   (00)
 *   -----------------------------------------------
 *         s            1      1      0      0
 *        ~s            0      1      0      1
 *       s | d          1      1      1      0
 *       s & d          1      0      0      0
 *       s ^ d          0      1      1      0
 *      ~s | d          1      0      1      1
 *      ~s & d          0      0      1      0
 *      ~s ^ d          1      0      0      1
 *       s | ~d         1      1      0      1
 *       s & ~d         0      1      0      0
 *       s ^ ~d         1      0      0      1
 *      ~(s | d)        0      0      0      1
 *      ~(s & d)        0      1      1      1
 *      ~(s ^ d)        1      0      0      1
 *
 *  Note that the following three operations are equivalent:
 *      ~(s ^ d)
 *      ~s ^ d
 *      s ^ ~d
 *  and in the implementation, we call them out with the first form;
 *  namely, ~(s ^ d).
 *
 *  Of the 16 possible binary combinations of src and dest bits,
 *  the remaining 4 unique ones are independent of the src bit.
 *  They depend on either just the dest bit or on neither
 *  the src nor dest bits:
 *
 *         d            1      0      1      0    (indep. of s)
 *        ~d            0      1      0      1    (indep. of s)
 *        CLR           0      0      0      0    (indep. of both s & d)
 *        SET           1      1      1      1    (indep. of both s & d)
 *
 *  As mentioned above, three of these are implemented by
 *  rasteropUniLow(), and one is a no-op.
 *
 *  How can these operation codes be represented by bits
 *  in such a way that when the basic operations are performed
 *  on the bits the results are unique for unique
 *  operations, and mimic the logic table given above?
 *
 *  The answer is to choose a particular order of the pairings:
 *         (sd)         (11)   (10)   (01)   (00)
 *  (which happens to be the same as in the above table)
 *  and to translate the result into 4-bit representations
 *  of s and d.  For example, the Sun rasterop choice
 *  (omitting the extra bit for clipping) is
 *
 *      PIX_SRC      0xc
 *      PIX_DST      0xa
 *
 *  This corresponds to our pairing order given above:
 *         (sd)         (11)   (10)   (01)   (00)
 *  where for s = 1 we get the bit pattern
 *       PIX_SRC:        1      1      0      0     (0xc)
 *  and for d = 1 we get the pattern
 *       PIX_DST:         1      0      1      0    (0xa)
 *
 *  OK, that's the pairing order that Sun chose.  How many different
 *  ways can we assign bit patterns to PIX_SRC and PIX_DST to get
 *  the boolean ops to work out?  Any of the 4 pairs can be put
 *  in the first position, any of the remaining 3 pairs can go
 *  in the second; and one of the remaining 2 pairs can go the the third.
 *  There is a total of 4*3*2 = 24 ways these pairs can be permuted.
 */
void
GdRasterOp(PMWIMAGEHDR pixd, MWCOORD dx, MWCOORD dy, MWCOORD dw, MWCOORD dh, int op,
			PMWIMAGEHDR pixs, MWCOORD sx, MWCOORD sy)
{
	int dd;

	/* translate op to pix library format*/
	op = raster_ops[op];

    if (op == PIX_DST)   /* no-op */
        return;

	/* check padding to 32-bit boundary*/
	assert((pixd->pitch & 3) == 0);

	/* Check if operation is only on dest */
    dd = pixGetDepth(pixd);
    if (op == PIX_CLR || op == PIX_SET || op == PIX_NOT(PIX_DST)) {
        rasteropUniLow(pixGetData(pixd),
                       pixGetWidth(pixd), pixGetHeight(pixd), dd,
                       pixGetWpl(pixd),
                       dx, dy, dw, dh,
                       op);
        return;
    }

	/* Check depth of src and dest; these must agree */
    assert(dd == pixGetDepth(pixs));

    rasteropLow(pixGetData(pixd),
                pixGetWidth(pixd), pixGetHeight(pixd), dd,
                pixGetWpl(pixd),
                dx, dy, dw, dh,
                op,
                pixGetData(pixs),
                pixGetWidth(pixs), pixGetHeight(pixs),
                pixGetWpl(pixs),
                sx, sy);
}

/*
 *  roplow.c
 *
 *      Low level dest-only
 *           void            rasteropUniLow()
 *           static void     rasteropUniWordAlignedlLow()
 *           static void     rasteropUniGeneralLow()
 *
 *      Low level src and dest
 *           void            rasteropLow()
 *           static void     rasteropWordAlignedLow()
 *           static void     rasteropVAlignedLow()
 *           static void     rasteropGeneralLow()
 *
 */

#define COMBINE_PARTIAL(d, s, m)     ( ((d) & ~(m)) | ((s) & (m)) )

static const l_int32  SHIFT_LEFT  = 0;
static const l_int32  SHIFT_RIGHT = 1;

static void rasteropUniWordAlignedLow(l_uint32 *datad, l_int32 dwpl, l_int32 dx,
                                      l_int32 dy, l_int32  dw, l_int32 dh,
                                      l_int32 op);

static void rasteropUniGeneralLow(l_uint32 *datad, l_int32 dwpl, l_int32 dx,
                                  l_int32 dy, l_int32 dw, l_int32  dh,
                                  l_int32 op);

static void rasteropWordAlignedLow(l_uint32 *datad, l_int32 dwpl, l_int32 dx,
                                   l_int32 dy, l_int32 dw, l_int32 dh,
                                   l_int32 op, l_uint32 *datas, l_int32 swpl,
                                   l_int32 sx, l_int32 sy);

static void rasteropVAlignedLow(l_uint32 *datad, l_int32 dwpl, l_int32 dx,
                                l_int32 dy, l_int32 dw, l_int32 dh,
                                l_int32 op, l_uint32 *datas, l_int32 swpl,
                                l_int32 sx, l_int32 sy);

static void rasteropGeneralLow(l_uint32 *datad, l_int32 dwpl, l_int32 dx,
                               l_int32 dy, l_int32 dw, l_int32 dh,
                               l_int32 op, l_uint32 *datas, l_int32 swpl,
                               l_int32 sx, l_int32 sy);


#if MW_CPU_BIG_ENDIAN
static const l_uint32 lmask32[] = {0x0,
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000, 0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0, 0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff};

static const l_uint32 rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};
#else
/* masks re-ordered for little endian byte order dword [3,2,1,0]*/
static const l_uint32 lmask32[] = {0x0,
	0x00000080, 0x000000c0, 0x000000e0, 0x000000f0, 0x000000f8, 0x000000fc, 0x000000fe, 0x000000ff, 
	0x000080ff, 0x0000c0ff, 0x0000e0ff, 0x0000f0ff, 0x0000f8ff, 0x0000fcff, 0x0000feff, 0x0000ffff, 
	0x0080ffff, 0x00c0ffff, 0x00e0ffff, 0x00f0ffff, 0x00f8ffff, 0x00fcffff, 0x00feffff, 0x00ffffff, 
	0x80ffffff, 0xc0ffffff, 0xe0ffffff, 0xf0ffffff, 0xf8ffffff, 0xfcffffff, 0xfeffffff, 0xffffffff};

static const l_uint32 rmask32[] = {0x0,
	0x01000000, 0x03000000, 0x07000000, 0x0f000000, 0x1f000000, 0x3f000000, 0x7f000000, 0xff000000, 
	0xff010000, 0xff030000, 0xff070000, 0xff0f0000, 0xff1f0000, 0xff3f0000, 0xff7f0000, 0xffff0000, 
	0xffff0100, 0xffff0300, 0xffff0700, 0xffff0f00, 0xffff1f00, 0xffff3f00, 0xffff7f00, 0xffffff00, 
	0xffffff01, 0xffffff03, 0xffffff07, 0xffffff0f, 0xffffff1f, 0xffffff3f, 0xffffff7f, 0xffffffff};
#endif

/*--------------------------------------------------------------------*
 *                     Low-level dest-only rasterops                  *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropUniLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dpixw  (width of dest)
 *              dpixh  (height of dest)
 *              depth  (depth of src and dest)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *      Return: void
 *
 *  Action: scales width, performs clipping, checks alignment, and
 *          dispatches for the rasterop.
 */
static void
rasteropUniLow(l_uint32  *datad,
               l_int32    dpixw,
               l_int32    dpixh,
               l_int32    depth,
               l_int32    dwpl,
               l_int32    dx,
               l_int32    dy,
               l_int32    dw,
               l_int32    dh,
               l_int32    op)
{
l_int32  dhangw, dhangh;

   /* -------------------------------------------------------*
    *            scale horizontal dimensions by depth
    * -------------------------------------------------------*/
    if (depth != 1) {
        dpixw *= depth;
        dx *= depth;
        dw *= depth;
    }

   /* -------------------------------------------------------*
    *            clip rectangle to dest image
    * -------------------------------------------------------*/
       /* first, clip horizontally (dx, dw) */
    if (dx < 0) {
        dw += dx;  /* reduce dw */
        dx = 0;
    }
    dhangw = dx + dw - dpixw;  /* rect ovhang dest to right */
    if (dhangw > 0)
        dw -= dhangw;  /* reduce dw */

       /* then, clip vertically (dy, dh) */
    if (dy < 0) {
        dh += dy;  /* reduce dh */
        dy = 0;
    }
    dhangh = dy + dh - dpixh;  /* rect ovhang dest below */
    if (dhangh > 0)
        dh -= dhangh;  /* reduce dh */

        /* if clipped entirely, quit */
    if ((dw <= 0) || (dh <= 0))
        return;

   /* -------------------------------------------------------*
    *       dispatch to aligned or non-aligned blitters
    * -------------------------------------------------------*/
    if ((dx & 31) == 0)
        rasteropUniWordAlignedLow(datad, dwpl, dx, dy, dw, dh, op);
    else
        rasteropUniGeneralLow(datad, dwpl, dx, dy, dw, dh, op);
    return;
}



/*--------------------------------------------------------------------*
 *           Static low-level uni rasterop with word alignment        *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropUniWordAlignedLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *      Return: void
 *
 *  This is called when the dest rect is left aligned
 *  on (32-bit) word boundaries.   That is: dx & 31 == 0.
 *
 *  We make an optimized implementation of this because
 *  it is a common case: e.g., operating on a full dest image.
 */
static void
rasteropUniWordAlignedLow(l_uint32  *datad,
                          l_int32    dwpl,
                          l_int32    dx,
                          l_int32    dy,
                          l_int32    dw,
                          l_int32    dh,
                          l_int32    op)
{
l_int32    nfullw;     /* number of full words */
l_uint32  *pfword;     /* ptr to first word */
l_int32    lwbits;     /* number of ovrhang bits in last partial word */
l_uint32   lwmask;     /* mask for last partial word */
l_uint32  *lined;
l_int32    i, j;

    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
    nfullw = dw >> 5;
    lwbits = dw & 31;
    if (lwbits)
        lwmask = lmask32[lwbits];
    pfword = datad + dwpl * dy + (dx >> 5);
    

    /*--------------------------------------------------------*
     *            Now we're ready to do the ops               *
     *--------------------------------------------------------*/
    switch (op)
    {
    case PIX_CLR:
        for (i = 0; i < dh; i++) {
            lined = pfword + i * dwpl;
            for (j = 0; j < nfullw; j++)
                *lined++ = 0x0;
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, 0x0, lwmask);
        }
        break;
    case PIX_SET:
        for (i = 0; i < dh; i++) {
            lined = pfword + i * dwpl;
            for (j = 0; j < nfullw; j++)
                *lined++ = 0xffffffff;
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, 0xffffffff, lwmask);
        }
        break;
    case PIX_NOT(PIX_DST):
        for (i = 0; i < dh; i++) {
            lined = pfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = ~(*lined);
                lined++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, ~(*lined), lwmask);
        }
        break;
    //default:
        //EPRINTF("Operation %d not permitted here!\n", op);
    }

    return;
}


/*--------------------------------------------------------------------*
 *        Static low-level uni rasterop without word alignment        *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropUniGeneralLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *      Return: void
 */
static void
rasteropUniGeneralLow(l_uint32  *datad,
                      l_int32    dwpl,
                      l_int32    dx,
                      l_int32    dy,
                      l_int32    dw,
                      l_int32    dh,
                      l_int32    op)
{
l_int32    dfwpartb;   /* boolean (1, 0) if first dest word is partial */
l_int32    dfwpart2b;  /* boolean (1, 0) if first dest word is doubly partial */
l_uint32   dfwmask;    /* mask for first partial dest word */
l_int32    dfwbits;    /* first word dest bits in ovrhang */
l_uint32  *pdfwpart;   /* ptr to first partial dest word */
l_int32    dfwfullb;   /* boolean (1, 0) if there exists a full dest word */
l_int32    dnfullw;    /* number of full words in dest */
l_uint32  *pdfwfull;   /* ptr to first full dest word */
l_int32    dlwpartb;   /* boolean (1, 0) if last dest word is partial */
l_uint32   dlwmask;    /* mask for last partial dest word */
l_int32    dlwbits;    /* last word dest bits in ovrhang */
l_uint32  *pdlwpart;   /* ptr to last partial dest word */
l_int32    i, j;


    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
        /* is the first word partial? */
    if ((dx & 31) == 0) {  /* if not */
        dfwpartb = 0;
        dfwbits = 0;
    }
    else {  /* if so */
        dfwpartb = 1;
        dfwbits = 32 - (dx & 31);
        dfwmask = rmask32[dfwbits];
        pdfwpart = datad + dwpl * dy + (dx >> 5);
    }

        /* is the first word doubly partial? */
    if (dw >= dfwbits)  /* if not */
        dfwpart2b = 0;
    else {  /* if so */
        dfwpart2b = 1;
        dfwmask &= lmask32[32 - dfwbits + dw];
    }

        /* is there a full dest word? */
    if (dfwpart2b == 1) {  /* not */
        dfwfullb = 0;
        dnfullw = 0;
    }
    else {
        dnfullw = (dw - dfwbits) >> 5;
        if (dnfullw == 0)  /* if not */
            dfwfullb = 0;
        else {  /* if so */
            dfwfullb = 1;
            if (dfwpartb)
                pdfwfull = pdfwpart + 1;
            else
                pdfwfull = datad + dwpl * dy + (dx >> 5);
        }
    }

        /* is the last word partial? */
    dlwbits = (dx + dw) & 31;
    if (dfwpart2b == 1 || dlwbits == 0)  /* if not */
        dlwpartb = 0;
    else {
        dlwpartb = 1;
        dlwmask = lmask32[dlwbits];
        if (dfwpartb)
            pdlwpart = pdfwpart + 1 + dnfullw;
        else
            pdlwpart = datad + dwpl * dy + (dx >> 5) + dnfullw;
    }


    /*--------------------------------------------------------*
     *            Now we're ready to do the ops               *
     *--------------------------------------------------------*/
    switch (op)
    {
    case PIX_CLR:
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart, 0x0, dfwmask);
                pdfwpart += dwpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = 0x0;
                pdfwfull += dwpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, 0x0, dlwmask);
                pdlwpart += dwpl;
            }
        }
        break;
    case PIX_SET:
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart, 0xffffffff, dfwmask);
                pdfwpart += dwpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = 0xffffffff;
                pdfwfull += dwpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, 0xffffffff, dlwmask);
                pdlwpart += dwpl;
            }
        }
        break;
    case PIX_NOT(PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart, ~(*pdfwpart), dfwmask);
                pdfwpart += dwpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = ~(*(pdfwfull + j));
                pdfwfull += dwpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, ~(*pdlwpart), dlwmask);
                pdlwpart += dwpl;
            }
        }
        break;
    //default:
        //EPRINTF("Operation %d not permitted here!\n", op);
    }

    return;
}



/*--------------------------------------------------------------------*
 *                   Low-level src and dest rasterops                 *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dpixw  (width of dest)
 *              dpixh  (height of dest)
 *              depth  (depth of src and dest)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *              datas  (ptr to src image data)
 *              spixw  (width of src)
 *              spixh  (height of src)
 *              swpl   (wpl of src)
 *              sx     (x val of UL corner of src rectangle)
 *              sy     (y val of UL corner of src rectangle)
 *      Return: void
 *
 *  Action: Scales width, performs clipping, checks alignment, and
 *          dispatches for the rasterop.
 *
 *  Warning: the two images must have equal depth.  This is not checked.
 */
static void
rasteropLow(l_uint32  *datad,
            l_int32    dpixw,
            l_int32    dpixh,
            l_int32    depth,
            l_int32    dwpl,
            l_int32    dx,
            l_int32    dy,
            l_int32    dw,
            l_int32    dh,
            l_int32    op,
            l_uint32  *datas,
            l_int32    spixw,
            l_int32    spixh,
            l_int32    swpl,
            l_int32    sx,
            l_int32    sy)
{
l_int32  dhangw, shangw, dhangh, shangh;

   /* -------------------------------------------------------*
    *            scale horizontal dimensions by depth
    * -------------------------------------------------------*/
    if (depth != 1) {
        dpixw *= depth;
        dx *= depth;
        dw *= depth;
        spixw *= depth;
        sx *= depth;
    }


   /* -------------------------------------------------------*
    *      clip to max rectangle within both src and dest
    * -------------------------------------------------------*/
       /* first, clip horizontally (sx, dx, dw) */
    if (dx < 0) {
        sx -= dx;  /* increase sx */
        dw += dx;  /* reduce dw */
        dx = 0;
    }
    if (sx < 0) {
        dx -= sx;  /* increase dx */
        dw += sx;  /* reduce dw */
        sx = 0;
    }
    dhangw = dx + dw - dpixw;  /* rect ovhang dest to right */
    if (dhangw > 0)
        dw -= dhangw;  /* reduce dw */
    shangw = sx + dw - spixw;   /* rect ovhang src to right */
    if (shangw > 0)
        dw -= shangw;  /* reduce dw */

       /* then, clip vertically (sy, dy, dh) */
    if (dy < 0) {
        sy -= dy;  /* increase sy */
        dh += dy;  /* reduce dh */
        dy = 0;
    }
    if (sy < 0) {
        dy -= sy;  /* increase dy */
        dh += sy;  /* reduce dh */
        sy = 0;
    }
    dhangh = dy + dh - dpixh;  /* rect ovhang dest below */
    if (dhangh > 0)
        dh -= dhangh;  /* reduce dh */
    shangh = sy + dh - spixh;  /* rect ovhang src below */
    if (shangh > 0)
        dh -= shangh;  /* reduce dh */

        /* if clipped entirely, quit */
    if ((dw <= 0) || (dh <= 0))
        return;

   /* -------------------------------------------------------*
    *       dispatch to aligned or non-aligned blitters
    * -------------------------------------------------------*/
    if (((dx & 31) == 0) && ((sx & 31) == 0))
        rasteropWordAlignedLow(datad, dwpl, dx, dy, dw, dh, op,
                               datas, swpl, sx, sy);
    else if ((dx & 31) == (sx & 31))
        rasteropVAlignedLow(datad, dwpl, dx, dy, dw, dh, op,
                            datas, swpl, sx, sy);
    else
        rasteropGeneralLow(datad, dwpl, dx, dy, dw, dh, op,
                           datas, swpl, sx, sy);

    return;
}


/*--------------------------------------------------------------------*
 *        Static low-level rasterop with vertical word alignment      *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropWordAlignedLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *              datas  (ptr to src image data)
 *              swpl   (wpl of src)
 *              sx     (x val of UL corner of src rectangle)
 *              sy     (y val of UL corner of src rectangle)
 *      Return: void
 *
 *  This is called when both the src and dest rects 
 *  are left aligned on (32-bit) word boundaries.
 *  That is: dx & 31 == 0 and sx & 31 == 0
 *
 *  We make an optimized implementation of this because
 *  it is a common case: e.g., two images are rasterop'd
 *  starting from their UL corners (0,0).
 */
static void
rasteropWordAlignedLow(l_uint32  *datad,
                       l_int32    dwpl,
                       l_int32    dx,
                       l_int32    dy,
                       l_int32    dw,
                       l_int32    dh,
                       l_int32    op,
                       l_uint32  *datas,
                       l_int32    swpl,
                       l_int32    sx,
                       l_int32    sy)
{
l_int32    nfullw;     /* number of full words */
l_uint32  *psfword;    /* ptr to first src word */
l_uint32  *pdfword;    /* ptr to first dest word */
l_int32    lwbits;     /* number of ovrhang bits in last partial word */
l_uint32   lwmask;     /* mask for last partial word */
l_uint32  *lines, *lined;
l_int32    i, j;


    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
    nfullw = dw >> 5;
    lwbits = dw & 31;
    if (lwbits)
        lwmask = lmask32[lwbits];
    psfword = datas + swpl * sy + (sx >> 5);
    pdfword = datad + dwpl * dy + (dx >> 5);
    
    /*--------------------------------------------------------*
     *            Now we're ready to do the ops               *
     *--------------------------------------------------------*/
    switch (op)
    {
    case PIX_SRC:
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = *lines;
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, *lines, lwmask);
        }
        break;
    case PIX_NOT(PIX_SRC):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = ~(*lines);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, ~(*lines), lwmask);
        }
        break;
    case (PIX_SRC | PIX_DST):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = (*lines | *lined);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, (*lines | *lined), lwmask);
        }
        break;
    case (PIX_SRC & PIX_DST):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = (*lines & *lined);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, (*lines & *lined), lwmask);
        }
        break;
    case (PIX_SRC ^ PIX_DST):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = (*lines ^ *lined);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, (*lines ^ *lined), lwmask);
        }
        break;
    case (PIX_NOT(PIX_SRC) | PIX_DST):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = (~(*lines) | *lined);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, (~(*lines) | *lined), lwmask);
        }
        break;
    case (PIX_NOT(PIX_SRC) & PIX_DST):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = (~(*lines) & *lined);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, (~(*lines) & *lined), lwmask);
        }
        break;
    case (PIX_SRC | PIX_NOT(PIX_DST)):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = (*lines | ~(*lined));
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, (*lines | ~(*lined)), lwmask);
        }
        break;
    case (PIX_SRC & PIX_NOT(PIX_DST)):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = (*lines & ~(*lined));
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, (*lines & ~(*lined)), lwmask);
        }
        break;
    case (PIX_NOT(PIX_SRC | PIX_DST)):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = ~(*lines  | *lined);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, ~(*lines  | *lined), lwmask);
        }
        break;
    case (PIX_NOT(PIX_SRC & PIX_DST)):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = ~(*lines  & *lined);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, ~(*lines  & *lined), lwmask);
        }
        break;
        /* this is three cases: ~(s ^ d), ~s ^ d, s ^ ~d  */
    case (PIX_NOT(PIX_SRC ^ PIX_DST)):
        for (i = 0; i < dh; i++) {
            lines = psfword + i * swpl;
            lined = pdfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = ~(*lines ^ *lined);
                lined++;
                lines++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, ~(*lines ^ *lined), lwmask);
        }
        break;
    //default:
        //EPRINTF("Operation %d invalid\n", op);
    }

    return;
}



/*--------------------------------------------------------------------*
 *        Static low-level rasterop with vertical word alignment      *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropVAlignedLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *              datas  (ptr to src image data)
 *              swpl   (wpl of src)
 *              sx     (x val of UL corner of src rectangle)
 *              sy     (y val of UL corner of src rectangle)
 *      Return: void
 *
 *  This is called when the left side of the src and dest
 *  rects have the same alignment relative to (32-bit) word
 *  boundaries; i.e., (dx & 31) == (sx & 31)
 */
static void
rasteropVAlignedLow(l_uint32  *datad,
                    l_int32    dwpl,
                    l_int32    dx,
                    l_int32    dy,
                    l_int32    dw,
                    l_int32    dh,
                    l_int32    op,
                    l_uint32  *datas,
                    l_int32    swpl,
                    l_int32    sx,
                    l_int32    sy)
{
l_int32    dfwpartb;   /* boolean (1, 0) if first dest word is partial */
l_int32    dfwpart2b;  /* boolean (1, 0) if first dest word is doubly partial */
l_uint32   dfwmask;    /* mask for first partial dest word */
l_int32    dfwbits;    /* first word dest bits in ovrhang */
l_uint32  *pdfwpart;   /* ptr to first partial dest word */
l_uint32  *psfwpart;   /* ptr to first partial src word */
l_int32    dfwfullb;   /* boolean (1, 0) if there exists a full dest word */
l_int32    dnfullw;    /* number of full words in dest */
l_uint32  *pdfwfull;   /* ptr to first full dest word */
l_uint32  *psfwfull;   /* ptr to first full src word */
l_int32    dlwpartb;   /* boolean (1, 0) if last dest word is partial */
l_uint32   dlwmask;    /* mask for last partial dest word */
l_int32    dlwbits;    /* last word dest bits in ovrhang */
l_uint32  *pdlwpart;   /* ptr to last partial dest word */
l_uint32  *pslwpart;   /* ptr to last partial src word */
l_int32    i, j;


    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
        /* is the first word partial? */
    if ((dx & 31) == 0) {  /* if not */
        dfwpartb = 0;
        dfwbits = 0;
    }
    else {  /* if so */
        dfwpartb = 1;
        dfwbits = 32 - (dx & 31);
        dfwmask = rmask32[dfwbits];
        pdfwpart = datad + dwpl * dy + (dx >> 5);
        psfwpart = datas + swpl * sy + (sx >> 5);
    }

        /* is the first word doubly partial? */
    if (dw >= dfwbits)  /* if not */
        dfwpart2b = 0;
    else {  /* if so */
        dfwpart2b = 1;
        dfwmask &= lmask32[32 - dfwbits + dw];
    }

        /* is there a full dest word? */
    if (dfwpart2b == 1) {  /* not */
        dfwfullb = 0;
        dnfullw = 0;
    }
    else {
        dnfullw = (dw - dfwbits) >> 5;
        if (dnfullw == 0)  /* if not */
            dfwfullb = 0;
        else {  /* if so */
            dfwfullb = 1;
            if (dfwpartb) {
                pdfwfull = pdfwpart + 1;
                psfwfull = psfwpart + 1;
            }
            else {
                pdfwfull = datad + dwpl * dy + (dx >> 5);
                psfwfull = datas + swpl * sy + (sx >> 5);
            }
        }
    }

        /* is the last word partial? */
    dlwbits = (dx + dw) & 31;
    if (dfwpart2b == 1 || dlwbits == 0)  /* if not */
        dlwpartb = 0;
    else {
        dlwpartb = 1;
        dlwmask = lmask32[dlwbits];
        if (dfwpartb) {
            pdlwpart = pdfwpart + 1 + dnfullw;
            pslwpart = psfwpart + 1 + dnfullw;
        }
        else {
            pdlwpart = datad + dwpl * dy + (dx >> 5) + dnfullw;
            pslwpart = datas + swpl * sy + (sx >> 5) + dnfullw;
        }
    }


    /*--------------------------------------------------------*
     *            Now we're ready to do the ops               *
     *--------------------------------------------------------*/
    switch (op)
    {
    case PIX_SRC:
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart, *psfwpart, dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = *(psfwfull + j);
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, *pslwpart, dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case PIX_NOT(PIX_SRC):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart, ~(*psfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = ~(*(psfwfull + j));
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, ~(*pslwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC | PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    (*psfwpart | *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) |= *(psfwfull + j);
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     (*pslwpart | *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC & PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    (*psfwpart & *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) &= *(psfwfull + j);
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     (*pslwpart & *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC ^ PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    (*psfwpart ^ *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) ^= *(psfwfull + j);
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     (*pslwpart ^ *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_NOT(PIX_SRC) | PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    (~(*psfwpart) | *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) |= ~(*(psfwfull + j));
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     (~(*pslwpart) | *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_NOT(PIX_SRC) & PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    (~(*psfwpart) & *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) &= ~(*(psfwfull + j));
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     (~(*pslwpart) & *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC | PIX_NOT(PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    (*psfwpart | ~(*pdfwpart)), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = *(psfwfull + j) | ~(*(pdfwfull + j));
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     (*pslwpart | ~(*pdlwpart)), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC & PIX_NOT(PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    (*psfwpart & ~(*pdfwpart)), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = *(psfwfull + j) & ~(*(pdfwfull + j));
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     (*pslwpart & ~(*pdlwpart)), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_NOT(PIX_SRC | PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    ~(*psfwpart | *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = ~(*(psfwfull + j) | *(pdfwfull + j));
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     ~(*pslwpart | *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_NOT(PIX_SRC & PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    ~(*psfwpart & *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = ~(*(psfwfull + j) & *(pdfwfull + j));
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     ~(*pslwpart & *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
        /* this is three cases: ~(s ^ d), ~s ^ d, s ^ ~d  */
    case (PIX_NOT(PIX_SRC ^ PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                    ~(*psfwpart ^ *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = ~(*(psfwfull + j) ^ *(pdfwfull + j));
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                                     ~(*pslwpart ^ *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    //default: 
        //EPRINTF("Operation %x invalid\n", op);
    }

    return;
}


/*--------------------------------------------------------------------*
 *     Static low-level rasterop without vertical word alignment      *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropGeneralLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *              datas  (ptr to src image data)
 *              swpl   (wpl of src)
 *              sx     (x val of UL corner of src rectangle)
 *              sy     (y val of UL corner of src rectangle)
 *      Return: void
 *
 *  This is called when the src and dest rects are
 *  do not have the same (32-bit) word alignment.
 *
 *  The method is a generalization of rasteropVAlignLow().
 *  There, the src image pieces were directly merged
 *  with the dest.  Here, we shift the source bits
 *  to fill words that are aligned with the dest, and
 *  then use those "source words" exactly in place
 *  of the source words that were used in rasteropVAlignLow().
 *
 *  The critical parameter is thus the shift required
 *  for the src.  Consider the left edge of the rectangle.
 *  The overhang into the src and dest words are found,
 *  and the difference is exactly this shift.  There are
 *  two separate cases, depending on whether the src pixels
 *  are shifted left or right.  If the src overhang is
 *  larger than the dest overhang, the src is shifted to
 *  the right, a number of pixels equal to the shift are
 *  left over for filling the next dest word, if necessary.
 *  But if the dest overhang is larger than the src,
 *  the src is shifted to the left, and it may also be 
 *  necessary to shift an equal number of pixels in from
 *  the next src word.  However, in both cases, after
 *  the first partial (or complete) dest word has been
 *  filled, the next src pixels will come from a left
 *  shift that exhausts the pixels in the src word.
 */
static void
rasteropGeneralLow(l_uint32  *datad,
                   l_int32    dwpl,
                   l_int32    dx,
                   l_int32    dy,
                   l_int32    dw,
                   l_int32    dh,
                   l_int32    op,
                   l_uint32  *datas,
                   l_int32    swpl,
                   l_int32    sx,
                   l_int32    sy)
{
l_int32    dfwpartb;    /* boolean (1, 0) if first dest word is partial      */
l_int32    dfwpart2b;   /* boolean (1, 0) if 1st dest word is doubly partial */
l_uint32   dfwmask;     /* mask for first partial dest word                  */
l_int32    dfwbits;     /* first word dest bits in overhang; 0-31            */
l_int32    dhang;       /* dest overhang in first partial word,              */
                        /* or 0 if dest is word aligned (same as dfwbits)    */
l_uint32  *pdfwpart;    /* ptr to first partial dest word                    */
l_uint32  *psfwpart;    /* ptr to first partial src word                     */
l_int32    dfwfullb;    /* boolean (1, 0) if there exists a full dest word   */
l_int32    dnfullw;     /* number of full words in dest                      */
l_uint32  *pdfwfull;    /* ptr to first full dest word                       */
l_uint32  *psfwfull;    /* ptr to first full src word                        */
l_int32    dlwpartb;    /* boolean (1, 0) if last dest word is partial       */
l_uint32   dlwmask;     /* mask for last partial dest word                   */
l_int32    dlwbits;     /* last word dest bits in ovrhang                    */
l_uint32  *pdlwpart;    /* ptr to last partial dest word                     */
l_uint32  *pslwpart;    /* ptr to last partial src word                      */
l_uint32   sword;       /* compose src word aligned with the dest words      */
l_int32    sfwbits;     /* first word src bits in overhang (1-32),           */
                        /* or 32 if src is word aligned                      */
l_int32    shang;       /* source overhang in the first partial word,        */
                        /* or 0 if src is word aligned (not same as sfwbits) */
l_int32    sleftshift;  /* bits to shift left for source word to align       */
                        /* with the dest.  Also the number of bits that      */
                        /* get shifted to the right to align with the dest.  */
l_int32    srightshift; /* bits to shift right for source word to align      */
                        /* with dest.  Also, the number of bits that get     */
                        /* shifted left to align with the dest.              */
l_int32    srightmask;  /* mask for selecting sleftshift bits that have      */
                        /* been shifted right by srightshift bits            */
l_int32    sfwshiftdir; /* either SHIFT_LEFT or SHIFT_RIGHT                  */
l_int32    sfwaddb;     /* boolean: do we need an additional sfw right shift? */
l_int32    slwaddb;     /* boolean: do we need an additional slw right shift? */
l_int32    i, j;


    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
        /* To get alignment of src with dst (e.g., in the
         * full words) the src must do a left shift of its
         * relative overhang in the current src word,
         * and OR that with a right shift of
         * (31 -  relative overhang) from the next src word.
         * We find the absolute overhangs, the relative overhangs,
         * the required shifts and the src mask */
    if ((sx & 31) == 0)
        shang = 0;
    else
        shang = 32 - (sx & 31);
    if ((dx & 31) == 0)
        dhang = 0;
    else
        dhang = 32 - (dx & 31);

    if (shang == 0 && dhang == 0) {  /* this should be treated by an
                                        aligned operation, not by
                                        this general rasterop! */
        sleftshift = 0;
        srightshift = 0;
        srightmask = rmask32[0];
    }
    else {
        if (dhang > shang)
            sleftshift = dhang - shang;
        else
            sleftshift = 32 - (shang - dhang);
        srightshift = 32 - sleftshift; 
        srightmask = rmask32[sleftshift];
    }
    
        /* is the first dest word partial? */
    if ((dx & 31) == 0) {  /* if not */
        dfwpartb = 0;
        dfwbits = 0;
    }
    else {  /* if so */
        dfwpartb = 1;
        dfwbits = 32 - (dx & 31);
        dfwmask = rmask32[dfwbits];
        pdfwpart = datad + dwpl * dy + (dx >> 5);
        psfwpart = datas + swpl * sy + (sx >> 5);
        sfwbits = 32 - (sx & 31);
        if (dfwbits > sfwbits) {
            sfwshiftdir = SHIFT_LEFT;  /* and shift by sleftshift */
            if (dw < shang)
                sfwaddb = 0;
            else
                sfwaddb = 1;   /* and rshift in next src word by srightshift */
        }
        else
            sfwshiftdir = SHIFT_RIGHT;  /* and shift by srightshift */
    }

        /* is the first dest word doubly partial? */
    if (dw >= dfwbits)  /* if not */
        dfwpart2b = 0;
    else {  /* if so */
        dfwpart2b = 1;
        dfwmask &= lmask32[32 - dfwbits + dw];
    }

        /* is there a full dest word? */
    if (dfwpart2b == 1) {  /* not */
        dfwfullb = 0;
        dnfullw = 0;
    }
    else {
        dnfullw = (dw - dfwbits) >> 5;
        if (dnfullw == 0)  /* if not */
            dfwfullb = 0;
        else {  /* if so */
            dfwfullb = 1;
            pdfwfull = datad + dwpl * dy + ((dx + dhang) >> 5);
            psfwfull = datas + swpl * sy + ((sx + dhang) >> 5); /* yes, dhang */
        }
    }

        /* is the last dest word partial? */
    dlwbits = (dx + dw) & 31;
    if (dfwpart2b == 1 || dlwbits == 0)  /* if not */
        dlwpartb = 0;
    else {
        dlwpartb = 1;
        dlwmask = lmask32[dlwbits];
        pdlwpart = datad + dwpl * dy + ((dx + dhang) >> 5) + dnfullw;
        pslwpart = datas + swpl * sy + ((sx + dhang) >> 5) + dnfullw;
        if (dlwbits <= srightshift)   /* must be <= here !!! */
            slwaddb = 0;  /* we got enough bits from current src word */
        else
            slwaddb = 1;   /* must rshift in next src word by srightshift */
    }


    /*--------------------------------------------------------*
     *            Now we're ready to do the ops               *
     *--------------------------------------------------------*/
    switch (op)
    {
    case PIX_SRC:
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart, sword, dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) = sword;
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, sword, dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case PIX_NOT(PIX_SRC):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart, ~sword, dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) = ~sword;
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, ~sword, dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC | PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 (sword | *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) |= sword;
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               (sword | *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC & PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 (sword & *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) &= sword;
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               (sword & *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC ^ PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 (sword ^ *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) ^= sword;
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               (sword ^ *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_NOT(PIX_SRC) | PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 (~sword | *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) |= ~sword;
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               (~sword | *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_NOT(PIX_SRC) & PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 (~sword & *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) &= ~sword;
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               (~sword & *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC | PIX_NOT(PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 (sword | ~(*pdfwpart)), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) = sword | ~(*(pdfwfull + j));
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               (sword | ~(*pdlwpart)), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_SRC & PIX_NOT(PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 (sword & ~(*pdfwpart)), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) = sword & ~(*(pdfwfull + j));
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               (sword & ~(*pdlwpart)), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_NOT(PIX_SRC | PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 ~(sword | *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) = ~(sword | *(pdfwfull + j));
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               ~(sword | *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    case (PIX_NOT(PIX_SRC & PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 ~(sword & *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) = ~(sword & *(pdfwfull + j));
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               ~(sword & *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
        /* this is three cases: ~(s ^ d), ~s ^ d, s ^ ~d  */
    case (PIX_NOT(PIX_SRC ^ PIX_DST)):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++)
            {
                if (sfwshiftdir == SHIFT_LEFT) {
                    sword = *psfwpart << sleftshift;
                    if (sfwaddb) 
                        sword = COMBINE_PARTIAL(sword,
                                      *(psfwpart + 1) >> srightshift,
                                       srightmask);
                }
                else /* shift right */
                    sword = *psfwpart >> srightshift;

                *pdfwpart = COMBINE_PARTIAL(*pdfwpart,
                                 ~(sword ^ *pdfwpart), dfwmask);
                pdfwpart += dwpl;
                psfwpart += swpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++) {
                    sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                   *(psfwfull + j + 1) >> srightshift,
                                   srightmask);
                    *(pdfwfull + j) = ~(sword ^ *(pdfwfull + j));
                }
                pdfwfull += dwpl;
                psfwfull += swpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                sword = *pslwpart << sleftshift;
                if (slwaddb) 
                    sword = COMBINE_PARTIAL(sword,
                                  *(pslwpart + 1) >> srightshift,
                                  srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart,
                               ~(sword ^ *pdlwpart), dlwmask);
                pdlwpart += dwpl;
                pslwpart += swpl;
            }
        }
        break;
    //default: 
        //EPRINTF("Operation %x invalid\n", op);
    }

    return;
}
