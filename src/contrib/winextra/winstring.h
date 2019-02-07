/* winstring.h*/
#pragma once
#include <string.h>

static inline int lstrcmpiW(LPCWSTR str1, LPCWSTR str2)
{
	printf("lstrcmpiW IMPLEMENT ME");
	return 0;
}

/* string functions without the exception handler */

static inline LPWSTR STDCALL lstrcpynW(LPWSTR dst, LPCWSTR src, INT n)
{
	LPWSTR d = dst;
	LPCWSTR s = src;
	UINT count = n;

	while ((count > 1) && *s)
	{
		count--;
		*d++ = *s++;
	}
	if (count) *d = 0;
	return dst;
}

static inline LPSTR STDCALL lstrcpynA(LPSTR dst, LPCSTR src, INT n)
{
	LPSTR d = dst;
	LPCSTR s = src;
	UINT count = n;

	while ((count > 1) && *s)
	{
		count--;
		*d++ = *s++;
	}
	if (count) *d = 0;
	return dst;
}

static inline INT STDCALL lstrlenW(LPCWSTR str)
{
	const WCHAR *s = str;
	while (*s) s++;
	return s - str;
}

static inline INT STDCALL lstrlenA(LPCSTR str)
{
	return strlen(str);
}

static inline LPWSTR STDCALL lstrcpyW(LPWSTR dst, LPCWSTR src)
{
	WCHAR *p = dst;
	while ((*p++ = *src++));
	return dst;
}

static inline LPSTR STDCALL lstrcpyA(LPSTR dst, LPCSTR src)
{
	return strcpy(dst, src);
}

static inline LPWSTR STDCALL lstrcatW(LPWSTR dst, LPCWSTR src)
{
	WCHAR *p = dst;
	while (*p) p++;
	while ((*p++ = *src++));
	return dst;
}

static inline LPSTR STDCALL lstrcatA(LPSTR dst, LPCSTR src)
{
	return strcat(dst, src);
}
