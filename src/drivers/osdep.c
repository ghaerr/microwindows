/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Operating System Dependent Routines
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#if __ECOS
#include <cyg/infra/diag.h>
#elif ANDROID
#include <android/log.h>
#elif EMSCRIPTEN
#include <emscripten.h>
#elif PSP
#include <pspkernel.h>
#include <psputils.h>
#include <pspdebug.h>
#elif _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>		/* pull in REAL windows.h for _MSC_VER, no -Isrc/include*/
#endif

/* the below relative paths required for Win32 compile using real windows.h*/
/* this file is compiled without -I microwindows/src/include*/
#include "../include/uni_std.h"
#include "../include/sys_time.h"
#include "../include/mwtypes.h"
#include "../include/mwconfig.h"
#include "../include/osdep.h"


#if AJAGUAR
extern	uint32_t ajag_mstimer;
#endif


/**
 * Output error message
 */
int
GdError(const char *format, ...)
{
	va_list args;
	char 	buf[1024];

	va_start(args, format);
#if __ECOS
	/* diag_printf() has much less dependencies than write() */
	diag_printf(format, args);
#elif AJAGUAR
#pragma message("GdError - not fully implemented for Atari Jaguar platform")
	vsprintf(buf, format, args);
#elif PSP
	vsprintf(buf, format, args);
	pspDebugScreenPrintf("%s\n", buf);
#elif ANDROID
	vsprintf(buf, format, args);
	__android_log_print(ANDROID_LOG_INFO, "Microwindows", buf);
#elif _MSC_VER
	vsprintf(buf, format, args);
	OutputDebugStringA(buf);
#elif MSDOS
    {
	static int fd = -1;
	vsprintf(buf, format, args);
	if (fd < 0)
		fd = open("/log.txt", O_CREAT|O_TRUNC|O_WRONLY, 0666);
	if (fd >= 0) {
		write(fd, buf, strlen(buf));
		close(fd);
	}
	exit(1);
    }
#elif EMSCRIPTEN
	vsprintf(buf, format, args);
	fprintf(stderr, "%s\n", buf);
#elif SWIEROS
	vsprintf(buf, format, args);
	write(2, buf, strlen(buf));
#elif HAVE_FILEIO
	vsprintf(buf, format, args);
	write(2, buf, strlen(buf));
#else
	/* discard EPRINTF/DPRINTF output!*/
#pragma message("GdError - all EPRINTF/DPRINTF output will be discarded")
#endif
	va_end(args);
	return -1;
}

/*
 * Return # milliseconds elapsed since start of Microwindows
 * Granularity is 25 msec
 */
MWTIMEOUT
GdGetTickCount(void)
{
	static MWTIMEOUT startTicks = 0;
	static int init = 0;

	if (!init)
	{
		init = 1;
		startTicks = GdGetTickCount();
	}
#if AJAGUAR
//#pragma message("GdGetTickCout - not implemented for Atari Jaguar platform")
	return ajag_mstimer;
#elif UNIX | EMSCRIPTEN | _MSC_VER
	{
	struct timeval t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 25000) * 25) - startTicks;
	}
#elif MSDOS
	return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
#elif _MINIX
	{
	struct tms	t;
	
	return (uint32_t)times(&t) * 16;
	}
#elif __ECOS
  /* CYGNUM_HAL_RTC_NUMERATOR/CYGNUM_HAL_RTC_DENOMINATOR gives the length of one tick in nanoseconds */
   return (cyg_current_time()*(CYGNUM_HAL_RTC_NUMERATOR/CYGNUM_HAL_RTC_DENOMINATOR))/(1000*1000);
#else
#pragma message("GdGetTickCout - not implemented")
	return ++startTicks;
#endif
}

/*
 * Suspend execution of the program for the specified number of milliseconds.
 */
void
GdDelay(MWTIMEOUT msecs)
{
#if UNIX && HAVE_SELECT
	struct timeval timeval;

	timeval.tv_sec = msecs / 1000;
	timeval.tv_usec = (msecs % 1000) * 1000;
	select(0, NULL, NULL, NULL, &timeval);
#elif AJAGUAR
#pragma message("GdDelay - not implemented for Atari Jaguar platform")
#elif EMSCRIPTEN
	emscripten_sleep(msecs);
#elif PSP
	sceKernelDelayThread(1000 * msecs);
#elif MSDOS
	/* no delay required*/
#elif _MSC_VER
	Sleep(msecs);
#else
	/* no delay implemented*/
#pragma message("GrDelay - no delay implemented, will have excess CPU in eventloop")
#endif
}

/* ring bell*/
void
GdBell(void)
{
#if HAVE_FILEIO && !(PSP | EMSCRIPTEN | AJAGUAR)
	write(2, "\7", 1);
#endif
}

#if PSP
static int
exit_callback(void)
{
	sceKernelExitGame();
	return 0;
}

static void
CallbackThread(void *arg)
{
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
}

int
exit(int n)
{
	sceKernelExitGame();
}
#endif

void
GdPlatformInit(void)
{
#if AJAGUAR
#pragma message("GdPlatformInit - not implemented for Atari Jaguar platform")
#endif
#if PSP
	int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, 0);
#endif
#if NDS
	consoleDemoInit();  //setup the sub screen for printing
#endif
}

#if _MSC_VER
/* gettimeofday() for Windows*/

int
gettimeofday(struct timeval *tv, struct timezone *tz)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const unsigned __int64 EPOCH = ((unsigned __int64)116444736000000000ULL);

	SYSTEMTIME  		system_time;
	FILETIME    		file_time;
	unsigned __int64    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((unsigned __int64)file_time.dwLowDateTime);
	time += ((unsigned __int64)file_time.dwHighDateTime) << 32;

	if (tv)
	{
		tv->tv_sec = (long)((time - EPOCH) / 10000000L);
		tv->tv_usec = (long)(system_time.wMilliseconds * 1000);
	}
	if (tz)
	{
#if _MSC_VER
		tz->tz_minuteswest = 7 * 60;	// FIXME
		tz->tz_dsttime = 0;
#else
		static int tzflag = 0;
		if (!tzflag)
		{
			_tzset();
			tzflag = 1;
		}
		tz->tz_minuteswest - _timezone / 60;
		tz->tz_dsttime = _daylight;
#endif
	}
	return 0;
}
#endif /* _MSC_VER*/
