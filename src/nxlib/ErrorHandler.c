#include "nxlib.h"

static XIOErrorHandler _errorfunc = 0;

void
_nxErrorHandler(GR_EVENT * event)
{
	/* Fixme:  Determine and send the display value here */
	DPRINTF("XIOErrorHandler called\n");
	if (_errorfunc)
		_errorfunc(0);

	/* Fixme:  Do some default stuff here? */
}

XIOErrorHandler
XSetIOErrorHandler(XIOErrorHandler handle)
{
	XIOErrorHandler prev = _errorfunc;

	GrSetErrorHandler(_nxErrorHandler);
	_errorfunc = handle;
	return prev;
}
