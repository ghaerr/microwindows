/*
 * Copyright (c) 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2003 Jon Foster <jon@jon-foster.co.uk>
 *
 * Header file to automatically determine CPU endianness (MW_CPU_BIG_ENDIAN)
 * Also 16 and 32 bit conversion routines between little/big endian.
 *
 * Currently defined platforms:
 *	LINUX
 * 	RTEMS
 *	__ECOS
 *	__FreeBSD__
 *	__CYGWIN__
 *	__MINGW32__
 *	TRIMEDIA
 *	MACOSX
 */

#ifndef MW_SWAP_H_INCLUDED
#define MW_SWAP_H_INCLUDED

/*
 *  First try to set MW_CPU_xxx_ENDIAN automatically for those OSes that can do so.
 */
#if __fiwix__
#   define MW_CPU_BIG_ENDIAN	1
#   define MW_CPU_LITTLE_ENDIAN 0
#elif LINUX && !__ECOS && !__MINGW32__
#include <endian.h>
#elif RTEMS | MACOS | defined(__FreeBSD__)
#include <machine/endian.h>
#else
#include <sys/types.h>
#endif

#ifdef __BYTE_ORDER
# if __BYTE_ORDER == __BIG_ENDIAN
#  if !MW_CPU_BIG_ENDIAN
#   error MW_CPU_BIG_ENDIAN and your OS disagree about your CPUs byte-order.  Did you forget to set BIGENDIAN in the config file?
#  endif
#   define MW_CPU_BIG_ENDIAN	1
#   define MW_CPU_LITTLE_ENDIAN 0
# elif __BYTE_ORDER == __LITTLE_ENDIAN
#  if MW_CPU_BIG_ENDIAN
#   error MW_CPU_BIG_ENDIAN and your OS disagree about your CPUs byte-order.  Did you accidentally set BIGENDIAN in the config file?
#  endif
#   define MW_CPU_LITTLE_ENDIAN 1
#   define MW_CPU_BIG_ENDIAN	0
# else
#   error "swap.h: since when did anybody support the PDP-11?"
# endif
#endif

/*
 *  Now pick the implementation of the byte swap routines.
 */

/* ********************************************************************* */
/* First, the default (portable) implementation.                         */
/* ********************************************************************* */

#if !MW_CPU_BIG_ENDIAN

/* little endian - no action required */
# define host_to_little_endian_16(x)	(x)
# define host_to_little_endian_32(x)	(x)

#else
/** Convert little-endian 16-bit number to the host CPU format. */
# define host_to_little_endian_16(x)	((((x) << 8) & 0xff00) | (((x) >> 8) & 0x00ff))

/** Convert little-endian 32-bit number to the host CPU format. */
# define host_to_little_endian_32(x)	((((x) << 24) & 0xff000000L) | \
			 							 (((x) <<  8) & 0x00ff0000L) | \
			 							 (((x) >>  8) & 0x0000ff00L) | \
			 							 (((x) >> 24) & 0x000000ffL) )
#endif /* MW_CPU_BIG_ENDIAN*/

/* ********************************************************************* */
/* Now, some platform-specific optimized macros.                         */
/* ********************************************************************* */

#if LINUX_POWERPPC
# if !MW_CPU_BIG_ENDIAN
#  error POWERPC works in BIG ENDIAN only !!!
# endif

/* ********************************************************************* */
/* Linux                                                                 */
/* Both LINUX and __ECOS are checked, because when compiling for the     */
/* synthetic target of eCos, both LINUX and __ECOS are defined           */
/* ********************************************************************* */
#elif LINUX && !__ECOS && !__MINGW32__ && !__fiwix__

# if __BYTE_ORDER == __BIG_ENDIAN
#  undef host_to_little_endian_16
#  undef host_to_little_endian_32
#  include <byteswap.h>
#  define host_to_little_endian_16(x)	bswap_16(x)
#  define host_to_little_endian_32(x)	bswap_32(x)
# endif /* !__BYTE_ORDER == __BIG_ENDIAN*/
/* end LINUX*/

/* ********************************************************************* */
/* MAC OSX                                                               */
/* ********************************************************************* */
#elif MACOSX

/* ********************************************************************* */
/* FreeBSD                                                               */
/* ********************************************************************* */
#elif defined(__FreeBSD__)

# if __BYTE_ORDER == __BIG_ENDIAN
#  ifndef __byte_swap_word
/* Either this isn't GCC or the implementation changed. */
#   warning __byte_swap_word not defined in endian.h on FreeBSD.
#  else
#   undef host_to_little_endian_16
#   undef host_to_little_endian_32
#   define host_to_little_endian_16(x)	__byte_swap_word(x)
#   define host_to_little_endian_32(x)	__byte_swap_long(x)
#  endif /* ifndef __byte_swap_word*/
# endif /* !__BYTE_ORDER == __BIG_ENDIAN*/
/* end __FreeBSD__*/

/* ********************************************************************* */
/* ECOS                                                                  */
/* ********************************************************************* */
#elif __ECOS

/* although machine/endian.h might provide optimized versions,           */
/* endian.h is only available if ecos is configured with networking      */
/* In order to avoid this dependency of microwindows to networking       */
/* this header is commented out                                          */
/*
#if __BYTE_ORDER == __BIG_ENDIAN
# undef host_to_little_endian_16
# undef host_to_little_endian_32
# include <machine/endian.h>
# define host_to_little_endian_16(x)	letoh16(x)
# define host_to_little_endian_32(x)	letoh32(x)
#endif
*/
/* end __ECOS*/

/* ********************************************************************* */
/* Cygwin only works on x86/win32, therefore it's always little endian   */
/* ********************************************************************* */
#elif defined(__CYGWIN__) | __MINGW32__

/* *********************************************************************
 * RTEMS
 * ********************************************************************* */
#elif RTEMS

/* ********************************************************************* */
/* TriMedia/pSOS                                                         */
/* ********************************************************************* */
#elif TRIMEDIA

/* ********************************************************************* */
/* Other                                                                 */
/* ********************************************************************* */
#else

#endif

#endif /* ifndef MW_SWAP_H_INCLUDED */
