#include "windows.h"
#include "wintools.h"
/*
 * WIN Draw Library
 *	Fast fill rectangle
 */

/*
 * fast fill background (works with non-dithered colors only)
 */
void WINAPI
FastFillRect(HDC hdc,LPRECT lprect,COLORREF cr)
{
	COLORREF	crOld;

	crOld = SetBkColor( hdc, cr);
	ExtTextOut( hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED, lprect, NULL, 0, NULL);
	SetBkColor( hdc, crOld);
}
