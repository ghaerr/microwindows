/*
 * Copyright (c) 2001, 2003 Greg Haerr <greg@censoft.com>
 * Copyright (c) 2003 Jon Foster <jon@jon-foster.co.uk>
 *
 * Byte and word swapping header file for big/little endian mapping
 *
 * Currently defined platforms:
 *	linux
 *	__FreeBSD__
 *	__ECOS
 *	__CYGWIN__
 *	TRIMEDIA
 */

/* ********************************************************************* */
/* First, the default (portable) implementation.                         */
/* ********************************************************************* */

#if !MW_CPU_BIG_ENDIAN

/* little endian - no action required */
# define wswap(x)	(x)
# define dwswap(x)	(x)

#else
/** Convert little-endian 16-bit number to the host CPU format. */
# define wswap(x)	((((x) << 8) & 0xff00) | (((x) >> 8) & 0x00ff))
/** Convert little-endian 32-bit number to the host CPU format. */
# define dwswap(x)	((((x) << 24) & 0xff000000L) | \
			 (((x) <<  8) & 0x00ff0000L) | \
			 (((x) >>  8) & 0x0000ff00L) | \
			 (((x) >> 24) & 0x000000ffL) )
#endif

/**
 * Read little endian format 32-bit number from buffer, possibly not
 * aligned, and convert to the host CPU format.
 */
#define dwread(addr)	((((unsigned char *)(addr))[0] | \
			 (((unsigned char *)(addr))[1] << 8) | \
			 (((unsigned char *)(addr))[2] << 16) | \
			 (((unsigned char *)(addr))[3] << 24)))


/* ********************************************************************* */
/* Now, some platform-specific optimized macros.                         */
/* ********************************************************************* */

/* ********************************************************************* */
/* Linux                                                                 */
/* ********************************************************************* */
#ifdef linux

# include <endian.h>
# if __BYTE_ORDER == __BIG_ENDIAN
#  if !MW_CPU_BIG_ENDIAN
#   error MW_CPU_BIG_ENDIAN and your OS disagree about your CPUs byte-order.  Did you forget to set BIGENDIAN in the config file?
#  endif
#  undef wswap
#  undef dwswap
#  include <byteswap.h>
#  define wswap(x)	bswap_16(x)
#  define dwswap(x)	bswap_32(x)
# else
#  if MW_CPU_BIG_ENDIAN
#   error MW_CPU_BIG_ENDIAN and your OS disagree about your CPUs byte-order.  Did you accidentally set BIGENDIAN in the config file?
#  endif
# endif /* !__BYTE_ORDER == __BIG_ENDIAN*/

/* ********************************************************************* */
/* FreeBSD                                                               */
/* ********************************************************************* */
#elif defined(__FreeBSD__)

# include <machine/endian.h>
# if __BYTE_ORDER == __BIG_ENDIAN
#  if !MW_CPU_BIG_ENDIAN
#   error MW_CPU_BIG_ENDIAN and your OS disagree about your CPUs byte-order.  Did you forget to set BIGENDIAN in the config file?
#  endif
#  ifndef __byte_swap_word
/* Either this isn't GCC or the implementation changed. */
#   warning __byte_swap_word not defined in endian.h on FreeBSD.
#  else
#   undef wswap
#   undef dwswap
#   define wswap(x)	__byte_swap_word(x)
#   define dwswap(x)	__byte_swap_long(x)
#  endif /* ifndef __byte_swap_word*/
# else
#  if MW_CPU_BIG_ENDIAN
#   error MW_CPU_BIG_ENDIAN and your OS disagree about your CPUs byte-order.  Did you accidentally set BIGENDIAN in the config file?
#  endif
# endif /* !__BYTE_ORDER == __BIG_ENDIAN*/

/* ********************************************************************* */
/* ECOS                                                                  */
/* ********************************************************************* */
#elif __ECOS

# undef wswap
# undef dwswap
# include <machine/endian.h>
# define wswap(x)	letoh16(x)
# define dwswap(x)	letoh32(x)
/* end __ECOS*/

/* ********************************************************************* */
/* Cygwin (on Win32)                                                     */
/* ********************************************************************* */
#elif defined(__CYGWIN__)

/* Cygwin only works on Win32 on x86.  Therefore it is always little-endian */
# if MW_CPU_BIG_ENDIAN
#  error MW_CPU_BIG_ENDIAN and your OS disagree about your CPUs byte-order.  Did you accidentally set BIGENDIAN in the config file?
# endif
/* x86 processors can do unaligned reads without problems (although
 * they're not optimal). */
# undef dwread
# define dwread(addr)	(*(unsigned int *)(addr))
/* end __CYGWIN__*/

/* ********************************************************************* */
/* TriMedia/pSOS                                                         */
/* ********************************************************************* */
#elif defined(TRIMEDIA)

/* The default implementation will be fine for now. */

/* ********************************************************************* */
/* Other                                                                 */
/* ********************************************************************* */
#else

/* If the following warning appears when you compile, then you have two options:
 * 1) Add detection for your platform, possibly with more optimized macros.
 * 2) Just comment out the warning.  If you have performance problems with
 *    BMP files and PCF fonts, you'll need to do (1).
 */
# warning You might want to define optimized byte swapping macros for this machine - if not, you can safely ignore this warning.
#endif
