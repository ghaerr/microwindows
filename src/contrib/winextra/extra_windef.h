/* extra_windef.h*/
#pragma once

typedef signed char INT8, *PINT8;
typedef unsigned char UINT8, *PUINT8;
typedef signed short INT16, *PINT16;
typedef unsigned short UINT16, *PUINT16;

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef unsigned int ULONG32, *PULONG32;
typedef unsigned int DWORD32, *PDWORD32;
typedef unsigned int UINT32, *PUINT32;

#if defined(_LP64) || defined(_WIN64)
//typedef int64_t LONG_PTR, *PLONG_PTR, INT_PTR, *PINT_PTR;
//typedef uint64_t ULONG_PTR, DWORD_PTR, *PULONG_PTR, UINT_PTR, *PUINT_PTR;
#else
//typedef int32_t LONG_PTR, *PLONG_PTR, INT_PTR, *PINT_PTR;
//typedef uint32_t DWORD_PTR, UINT_PTR, *PUINT_PTR;
#endif

#if _MSC_VER
typedef __int64 LONG64, *PLONG64;
typedef __int64 INT64, *PINT64;
typedef unsigned __int64 ULONG64, *PULONG64;
typedef unsigned __int64 DWORD64, *PDWORD64;
typedef unsigned __int64 UINT64, *PUINT64;
#else
typedef long long LONG64, *PLONG64;
typedef long long INT64, *PINT64;
typedef unsigned long long ULONG64, *PULONG64;
typedef unsigned long long DWORD64, *PDWORD64;
typedef unsigned long long UINT64, *PUINT64;
#endif

#if _MSC_VER
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
#else
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
#endif
typedef unsigned long long  DWORDLONG;

typedef struct _ULARGE_INTEGER {
	ULONGLONG QuadPart;
} ULARGE_INTEGER;


typedef struct timeval TIMEVAL, *PTIMEVAL, *LPTIMEVAL;

#define M_E        2.71828182845904523536   // e
#define M_LOG2E    1.44269504088896340736   // log2(e)
#define M_LOG10E   0.434294481903251827651  // log10(e)
#define M_LN2      0.693147180559945309417  // ln(2)
#define M_LN10     2.30258509299404568402   // ln(10)
#define M_PI       3.14159265358979323846   // pi
#define M_PI_2     1.57079632679489661923   // pi/2
#define M_PI_4     0.785398163397448309616  // pi/4
#define M_1_PI     0.318309886183790671538  // 1/pi
#define M_2_PI     0.636619772367581343076  // 2/pi
#define M_2_SQRTPI 1.12837916709551257390   // 2/sqrt(pi)
#define M_SQRT2    1.41421356237309504880   // sqrt(2)
#define M_SQRT1_2  0.707106781186547524401  // 1/sqrt(2)

typedef ULONG PROPID;
typedef USHORT LANGID;

#define _HRESULT_TYPEDEF_(x) ((HRESULT)x)

#define NOERROR                                            _HRESULT_TYPEDEF_(0L)
#define S_OK                                               _HRESULT_TYPEDEF_(0L)
#define SEC_E_OK                                           _HRESULT_TYPEDEF_(0L)
#define S_FALSE                                            _HRESULT_TYPEDEF_(1L)

#define E_OUTOFMEMORY                                      _HRESULT_TYPEDEF_(0x8007000EL)
#define E_INVALIDARG                                       _HRESULT_TYPEDEF_(0x80070057L)

#define FIELD_OFFSET(t,f)       ((LONG)(LONG_PTR)&(((t*) 0)->f))
 
typedef HANDLE		HENHMETAFILE;
typedef HANDLE		HMETAFILE;
typedef HANDLE		HKEY;
