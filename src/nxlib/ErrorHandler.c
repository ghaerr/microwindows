#include "nxlib.h"

static XIOErrorHandler _ioerrorfunc = 0;
static XErrorHandler _errorfunc = 0;

void
_nxIOErrorHandler(GR_EVENT * event)
{
	/* Fixme:  Determine and send the display value here */
	DPRINTF("XIOErrorHandler called\n");
	if (_ioerrorfunc)
		_ioerrorfunc(0);

	/* Fixme:  Do some default stuff here? */
}

XIOErrorHandler
XSetIOErrorHandler(XIOErrorHandler handle)
{
	XIOErrorHandler prev = _ioerrorfunc;

	GrSetErrorHandler(_nxIOErrorHandler);
	_ioerrorfunc = handle;
	return prev;
}

void
_nxErrorHandler(GR_EVENT * event)
{
	/* Fixme:  Determine and send the display value here */
	DPRINTF("XErrorHandler called\n");
	if (_errorfunc)
		_errorfunc(0,0);

	/* Fixme:  Do some default stuff here? */
}

XErrorHandler
XSetErrorHandler(XErrorHandler handle)
{
	XErrorHandler prev = _errorfunc;

	GrSetErrorHandler(_nxErrorHandler);
	_errorfunc = handle;
	return prev;
}