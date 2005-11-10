/*
 * Copyright (c) 2005 Alexander Neundorf, neundorf@kde.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _ECOSMWCONFIG_H_
#define _ECOSMWCONFIG_H_

#include <pkgconf/microwindows.h>

//0 - GdErrorNull, 1 GdError
#define DEBUG 1

//small ecos config
//#define __ECOS       1
#define UNIX         1
#define MSDOS        0
#define ELKS         0
#define __rtems__    0
#define RTEMS        0
#define _MINIX       0
#undef VXWORKS
#define NOTYET       0
#define DOS_TURBOC  0
#define DOS_DJGPP   0
#undef __PACIFIC__
#define VTSWITCH    0

#if defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_TRUECOLOR0888)
# define MWPIXEL_FORMAT MWPF_TRUECOLOR0888
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_RGB)
# define MWPIXEL_FORMAT MWPF_RGB
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_PIXELVAL)
# define MWPIXEL_FORMAT MWPF_PIXELVAL
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_PALETTE)
# define MWPIXEL_FORMAT MWPF_PALETTE
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_TRUECOLOR888)
# define MWPIXEL_FORMAT MWPF_TRUECOLOR888
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_TRUECOLOR565)
# define MWPIXEL_FORMAT MWPF_TRUECOLOR565
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_TRUECOLOR555)
# define MWPIXEL_FORMAT MWPF_TRUECOLOR555
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_TRUECOLOR332)
# define MWPIXEL_FORMAT MWPF_TRUECOLOR332
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_TRUECOLOR8888)
# define MWPIXEL_FORMAT MWPF_TRUECOLOR8888
#elsif defined(CYGFUN_MICROWINDOWS_PIXELFORMAT_TRUECOLOR233)
# define MWPIXEL_FORMAT MWPF_TRUECOLOR233
#endif

#ifdef CYGSEM_MICROWINDOWS_HAVE_FLOAT
# define HAVEFLOAT 1
#else
# define HAVEFLOAT 0
#endif

#ifdef CYGSEM_MICROWINDOWS_USE_STDIO
# define HAVE_FILEIO 1
#else
# undef  HAVE_FILEIO
# define EOF (-1)
#endif

#ifdef CYGFUN_MICROWINDOWS_DYNAMIC_CLIPPING_REGIONS
# define DYNAMICREGIONS 1
#else
# define DYNAMICREGIONS 0
#endif

#define MW_CPU_BIG_ENDIAN 0

#ifdef CYGFUN_MICROWINDOWS_IMAGES
# define MW_FEATURE_IMAGES 1
# ifdef CYGSEM_MICROWINDOWS_XPM
#  define HAVE_XPM_SUPPORT 1
# endif
# ifdef CYGSEM_MICROWINDOWS_PNM
#  define HAVE_PNM_SUPPORT 1
# endif
# ifdef CYGSEM_MICROWINDOWS_BMP
#  define HAVE_BMP_SUPPORT 1
# endif
# ifdef CYGSEM_MICROWINDOWS_JPEG
#  define HAVE_JPEG_SUPPORT 1
# endif
# ifdef CYGSEM_MICROWINDOWS_PNG
#  define HAVE_PNG_SUPPORT 1
# endif
#else
# define MW_FEATURE_IMAGES 0		/* platform doesn't support images*/
# undef  HAVE_BMP_SUPPORT
# undef  HAVE_PNM_SUPPORT
# undef  HAVE_XPM_SUPPORT
# undef  HAVE_PNG_SUPPORT
# undef  HAVE_TIFF_SUPPORT
# undef  HAVE_GIF_SUPPORT
# undef  HAVE_JPEG_SUPPORT
#endif

#ifdef CYGFUN_MICROWINDOWS_TIMERS
# define MW_FEATURE_TIMERS 1
#else
# define MW_FEATURE_TIMERS 0
#endif

#define HAVE_PCFGZ_SUPPORT      0

#ifdef CYGSEM_MICROWINDOWS_PCF_FONTS
# define HAVE_PCF_SUPPORT        1
# define PCF_FONT_DIR "CYGDAT_MICROWINDOWS_PCF_FONT_PATH"
#else
# define HAVE_PCF_SUPPORT        0
#endif

#ifdef CYGSEM_MICROWINDOWS_FNT_FONTS
# define HAVE_FNT_SUPPORT        1
# define FNT_FONT_DIR "CYGDAT_MICROWINDOWS_FNT_FONT_PATH"
#else
# define HAVE_FNT_SUPPORT        0
#endif


#define HAVE_T1LIB_SUPPORT      0
#define HAVE_FREETYPE_SUPPORT   0
#define HAVE_FREETYPE_2_SUPPORT 0
#define HAVE_HZK_SUPPORT        0
#define HAVE_EUCJP_SUPPORT      0
#define HAVE_SHAPEJOINING_SUPPORT 0
#define HAVE_FRIBIDI_SUPPORT    0


#ifdef CYGSEM_MICROWINDOWS_BIG5
# define HAVE_BIG5_SUPPORT     1
#else
# define HAVE_BIG5_SUPPORT     0
#endif

#ifdef CYGSEM_MICROWINDOWS_GB2312
# define HAVE_GB2312_SUPPORT   1
#else
# define HAVE_GB2312_SUPPORT   0
#endif

#ifdef CYGSEM_MICROWINDOWS_JISX0213
# define HAVE_JISX0213_SUPPORT 1
#else
# define HAVE_JISX0213_SUPPORT 0
#endif

#ifdef CYGSEM_MICROWINDOWS_KSC5601
# define HAVE_KSC5601_SUPPORT  1
#else
# define HAVE_KSC5601_SUPPORT  0
#endif


//what are these good for ?
#define xxxALPHABLEND 0
#define INVERT4BPP 0                    /* fblin4.c and fblin4rev.c */
#define FBVGA 0                         /* fb.c */

#endif

