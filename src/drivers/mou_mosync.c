/*
 * Mosync touchscreen mousedriver
 *
 * Copyright (c) 2010 DI (FH) Ludwig Ertl / CSP GmbH
 */

#include <ma.h>
#include "device.h"

extern MAEvent g_event;
extern SCREENDEVICE scrdev;

static int MOS_Open(MOUSEDEVICE *pmd)
{
	GdHideCursor(&scrdev);
	return 0;
}

static void MOS_Close(void)
{
}

static int MOS_GetButtonInfo(void)
{
	/* get "mouse" buttons supported */
	return MWBUTTON_L;
}

static void MOS_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = 3;
	*pthresh = 5;
}

static int MOS_Read(MWCOORD *px, MWCOORD *py, MWCOORD *pz, int *pb)
{
	switch (g_event.type)
	{
	case EVENT_TYPE_POINTER_PRESSED:
	case EVENT_TYPE_POINTER_DRAGGED:
		*pb = MWBUTTON_L;
		break;
	case EVENT_TYPE_POINTER_RELEASED:
		*pb = 0;
		break;
	default:
		return 0;
	}
	*px = g_event.point.x;
	*py = g_event.point.y;

	//*pz = event.pressure;

	if(!*pb)
		return 3;
	return 2;
}

MOUSEDEVICE mousedev = {
	MOS_Open,
	MOS_Close,
	MOS_GetButtonInfo,
	MOS_GetDefaultAccel,
	MOS_Read,
	NULL,
	MOUSE_RAW   /* Input filter flags */
};
