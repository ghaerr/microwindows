#include "windows.h"
#include "wintools.h"
/*
 * WIN Draw Library
 */

void WINAPI
InsetR(LPRECT lprc,int h,int v)
{
	lprc->top += v;
	lprc->left += h;
	lprc->right -= 2*h-1;
	lprc->bottom -= 2*v-1;
}
