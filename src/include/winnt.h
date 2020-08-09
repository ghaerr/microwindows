#ifndef _WINNT_H
#define _WINNT_H

#ifndef _GUID_DEFINED /* also defined in basetyps.h */
#define _GUID_DEFINED
typedef struct _GUID {
	unsigned long  Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[8];
} GUID, *REFGUID, *LPGUID;
#define SYSTEM_LUID { QuadPart:999 }
#endif /* _GUID_DEFINED */

#define PAGE_READWRITE 4

#include <winerror.h>

#endif
