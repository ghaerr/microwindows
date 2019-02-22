//---------------------------------------------------------
//	2009,2013 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"

// required for libXrender.so
unsigned long XNextRequest(Display *display)
{
	DPRINTF("XNextRequest called...\n");
	return ++display->request;
}
#if 0 /*defined in Nextevent.c */
// required for gdk
long XExtendedMaxRequestSize(Display *display)
{
	DPRINTF("XExtendedMaxRequestSize called...\n");
	return 0;
}
#endif
/*
 * WARNING: This implementation's pre-conditions and post-conditions
 * must remain compatible with the old macro-based implementations of
 * GetReq, GetReqExtra, GetResReq, and GetEmptyReq. The portions of the
 * Display structure affected by those macros are part of libX11's
 * ABI.
 */
void *_XGetRequest(Display *dpy, /*CARD8*/char type, size_t len)
{
	DPRINTF("_XGetRequest called...type:%x len:%ld\n", type, (long)len);
	return (void*)dpy->request;

/*	xReq *req;

	WORD64ALIGN

	if (dpy->bufptr + len > dpy->bufmax)
		_XFlush(dpy);

	if (len % 4)
		EPRINTF("Xlib: request %d length %zd not a multiple of 4.\n", type, len);

	dpy->last_req = dpy->bufptr;

	req = (xReq*)dpy->bufptr;
	req->reqType = type;
	req->length = len / 4;
	dpy->bufptr += len;
	dpy->request++;
	return req;*/
}
