#ifndef _WINBASE_H
#define _WINBASE_H

#define INVALID_HANDLE_VALUE     ((HANDLE)~(ULONG_PTR)0)

#define STD_INPUT_HANDLE (DWORD)-10	// The standard input device. Initially, this is the console input buffer, CONIN$.
#define STD_OUTPUT_HANDLE (DWORD)-11	// The standard output device. Initially, this is the active console screen buffer, CONOUT$.
#define STD_ERROR_HANDLE (DWORD)-12	// The standard error device. Initially, this is the active console screen buffer, CONOUT$.

#define INFINITE	0xFFFFFFFF

#define FILE_MAP_READ   4
#define FILE_MAP_WRITE  2

#define WAIT_OBJECT_0 0

typedef struct _OVERLAPPED {
  ULONG_PTR Internal;
  ULONG_PTR InternalHigh;
  union {
    struct {
      DWORD Offset;
      DWORD OffsetHigh;
    } DUMMYSTRUCTNAME;
    PVOID Pointer;
  } DUMMYUNIONNAME;
  HANDLE    hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES {
  DWORD  nLength;
  LPVOID lpSecurityDescriptor;
  BOOL   bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

extern HANDLE WINAPI GetStdHandle(DWORD nStdHandle);

#endif /* _WINBASE_H */
