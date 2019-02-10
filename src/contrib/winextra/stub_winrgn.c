/* stub_winrgn.c*/
#include "windows.h"
#include "winextra.h"
#include "device.h"			/* for GdUnionRectWithRegion*/

int WINAPI GetClipBox(HDC    hdc, LPRECT lprect)
{
	printf("GetClipBox BAD IMPLEMENTED");
	lprect->top = 0;
	lprect->left = 0;
	lprect->right = 800;
	lprect->bottom = 800;
	
	return SIMPLEREGION;
}

int WINAPI GetClipRgn(HDC  hdc, HRGN hrgn)
{
	printf("GetClipRgn IMPLEMENT ME");
	return 0;
}

int WINAPI GetGraphicsMode(HDC hdc)
{
	return hdc->GraphicsMode;
}

int WINAPI IntersectClipRect(HDC hdc, int left, int top, int right, int bottom)
{
	printf("IntersectClipRect IMPLEMENT ME");
	return 0;
}

int WINAPI SetGraphicsMode(HDC hdc, int iMode)
{
	int ret = hdc->GraphicsMode;
	hdc->GraphicsMode = iMode;

	return ret;
}

BOOL WINAPI GetWorldTransform(HDC     hdc, LPXFORM lpxf)
{
	if (!lpxf) return FALSE;
	*lpxf = hdc->xformWorld2Wnd;
	
	return TRUE;

}

BOOL WINAPI SetWorldTransform(HDC         hdc, const XFORM *lpxf)
{
	if (!lpxf) return FALSE;
	/* The transform must conform to (eM11 * eM22 != eM12 * eM21) requirement */
	if (lpxf->eM11 * lpxf->eM22 == lpxf->eM12 * lpxf->eM21) return FALSE;

	hdc->xformWorld2Wnd = *lpxf;
	//DC_UpdateXforms(dc);

	return TRUE;//for now just pretend OK!
}

BOOL WINAPI ModifyWorldTransform(HDC         hdc, const XFORM *lpxf, DWORD       mode) {

	printf("ModifyWorldTransform BAD IMPLEMENTED");
	switch (mode)
	{
	case MWT_IDENTITY:
		hdc->xformWorld2Wnd.eM11 = 1.0f;
		hdc->xformWorld2Wnd.eM12 = 0.0f;
		hdc->xformWorld2Wnd.eM21 = 0.0f;
		hdc->xformWorld2Wnd.eM22 = 1.0f;
		hdc->xformWorld2Wnd.eDx = 0.0f;
		hdc->xformWorld2Wnd.eDy = 0.0f;
		break;
	case MWT_LEFTMULTIPLY:
		//CombineTransform(&hdc->xformWorld2Wnd, lpxf, &hdc->xformWorld2Wnd);
		break;
	case MWT_RIGHTMULTIPLY:
		//CombineTransform(&hdc->xformWorld2Wnd, &hdc->xformWorld2Wnd, lpxf);
		break;
	default:
		return FALSE;
	}

	//DC_UpdateXforms(dc);
	return TRUE;
}

int WINAPI	SetMapMode(HDC hdc, int iMode) {
	printf("SetMapMode IMPLEMENT ME");
	return 0;
}
int WINAPI GetObjectW(HANDLE h, int    c, LPVOID pv) {
	printf("GetObjectW IMPLEMENT ME");
	return 0;
}

HRGN WINAPI
ExtCreateRegion(const XFORM* lpXform, DWORD dwCount, const RGNDATA* rgndata)
{
    HRGN hrgn = CreateRectRgn(0, 0, 0, 0);
    MWRGNOBJ *obj = (MWRGNOBJ *)hrgn;
    RECT *pCurRect, *pEndRect;

    if(!hrgn)
    {
        DPRINTF("Can't create a region!\n");
		return 0;
    }
    if(lpXform)
        DPRINTF("Xform not implemented - ignoring\n");
    
    if(rgndata->rdh.iType != RDH_RECTANGLES)
    {
        DPRINTF("Type not RDH_RECTANGLES\n");
		DeleteObject( hrgn );
		return 0;
    }

    pEndRect = (RECT *)rgndata->Buffer + rgndata->rdh.nCount;
    for(pCurRect = (RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
        GdUnionRectWithRegion( pCurRect, obj->rgn );

    return hrgn;
}
