#include "windows.h"
#include "wintools.h"

BOOL WINAPI
PtInsideWindow(HWND hwnd,UINT x,UINT y)
{
	/* Determine whether or not the position ( x, y) is contained			*/
	/* within the control's client rectangle.							*/

	RECT	clientRect;
	POINT	buttonPoint;

	buttonPoint.x = x;
	buttonPoint.y = y;

	GetClientRect( hwnd, &clientRect);
	return PtInRect( &clientRect, buttonPoint);
}
