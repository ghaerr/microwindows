/*
 * Copyright (c) 2001 Greg Haerr <greg@censoft.com>
 *
 * Byte and word swapping header file for big/little endian mapping
 */

#include <endian.h>
#include <byteswap.h>
#if __BYTE_ORDER == __BIG_ENDIAN
#define wswap(x)	bswap_16(x)
#define dwswap(x)	bswap_32(x)
/* read little endian format from buffer*/
#define dwread(addr)	((*(unsigned char *)(addr)) | \
			 (*(unsigned char *)(addr+1) << 8) | \
			 (*(unsigned char *)(addr+2) << 16) | \
			 (*(unsigned char *)(addr+3) << 24))
#else
/* little endian - no action required*/
#define wswap(x)	(x)
#define dwswap(x)	(x)
#define dwread(addr)	(*(unsigned long *)(addr))
#endif

