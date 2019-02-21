/* gettimeofday.c*/
#include "windows.h"
#include "sys_time.h"

#if _MSC_VER >= 1500
typedef unsigned __int64 uint64_t;

/* gettimeofday() for windows*/
typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _FILETIME
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

extern void STDCALL GetSystemTime(LPSYSTEMTIME lpSystemTime);
extern BOOL STDCALL SystemTimeToFileTime(SYSTEMTIME *lpSystemTime,LPFILETIME lpFileTime);
#endif

int
gettimeofday(struct timeval *tv, struct timezone *tz)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

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
