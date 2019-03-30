#ifndef SYS_TIME_H
#define SYS_TIME_H
/*
 * Microwindows sys_time.h
 *
 * Usage: replace #include <sys/time.h> with "sys_time.h"
 */


#if _MINIX
#include <sys/times.h>

#elif MSDOS
#include <time.h>

#elif defined(_MSC_VER)
struct timeval {
	long tv_sec;
	long tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#else	/* UNIX | DOS_DJGPP*/
#include <sys/time.h>
#endif

#endif /* SYS_TIME_H*/
