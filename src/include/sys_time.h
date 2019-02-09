#ifndef _SYS_TIME_H
#define _SYS_TIME_H
/*
 * Microwindows sys_time.h
 *
 * Usage: replace #include <sys/time.h> with "sys_time.h"
 */


#if _MINIX
#include <sys/times.h>

#elif UNIX | DOS_DJGPP
#include <sys/time.h>

#elif MSDOS
#include <time.h>

#elif defined(_MSC_VER)
struct timeval {
	long tv_sec;
	long tv_usec;
};
#endif /* defined(_MSC_VER)*/

#endif /* _SYS_TIME_H*/
