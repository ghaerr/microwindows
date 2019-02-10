//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"
#include <string.h>
#include <stdlib.h>

// from /usr/include/X11/extensions/shapestr.h
#define SHAPENAME "SHAPE"
#define SHAPE_MAJOR_VERSION	1	/* current version numbers */
#define SHAPE_MINOR_VERSION	1

typedef struct _EXT_Manage {
	char *name;
	char ver;
	int event;
	int error;
} EXT_Manage;
EXT_Manage exmanage[] = {
	{ SHAPENAME, SHAPE_MAJOR_VERSION, LASTEvent+1, 0, },
	{ NULL, 0, 0, 0 }
};

Bool XQueryExtension(Display *dpy, _Xconst char *name,
	int *major_opcode, int *first_event, int *first_error)
{
	EXT_Manage *p = exmanage;
	DPRINTF("XQueryExtension called [%s]\n", name);

	while (p->name) {
		if (!strcmp(p->name, name)) {
			if (major_opcode) *major_opcode = p->ver;
			if (first_event) *first_event = p->event;
			if (first_error) *first_error = p->error;
			return 1;
		}
		p++;
	}

	return 0;
}

// This codes run from extutil.c in libXext
// required for Qt
XExtCodes *XInitExtension(Display *dpy, _Xconst char *name)
{
	DPRINTF("XInitExtension called [%s]\n", name);
	return 0;
}

static int ext = 0;
XExtCodes *XAddExtension(Display *dpy)
{
	XExtCodes *codes = calloc(1, sizeof(XExtCodes));
	if (codes) {
		//codes->major_opcode = ext;
		codes->extension = ext++;
	}
	DPRINTF("XAddExtension called [#%d]\n", codes ? codes->extension : -1);
	return codes;
	/*DPRINTF("XAddExtension called\n");
	return 0;*/
}

int (*XESetCloseDisplay(Display *dpy, int extension, int (*proc)()))()
{
	DPRINTF("XESetCloseDisplay called [#%d]\n", extension);
	return NULL;
}

// from libXi
int /*XDeviceInfo **/XListInputDevices(Display *dpy, int *ndevices)
{
	return 0;
}
int XFreeDeviceList()
{
	return 0;
}

int /*XExtDisplayInfo **/XextFindDisplay(/*XExtensionInfo *extinfo, Display *dpy*/)
{
	return 0;
}
#if 0
int XextAddDisplay()
{
	return NULL;
}

int XRenderFreePicture()
{
	return NULL;
}
int XRenderFreeGlyphSet()
{
	return NULL;
}
int XRenderCompositeText32()
{
	return NULL;
}
int XRenderCompositeText16()
{
	return NULL;
}
int XRenderCompositeText8()
{
	return NULL;
}
#endif

Bool XSyncQueryExtension(Display *dpy, int *event_base, int *error_base)
{
	DPRINTF("XSyncQueryExtension called\n");
	//*event_base = *error_base = 0; //segfault
	return 0;
}
Bool XShmQueryExtension(Display *dpy, int *event_base, int *error_base)
{
	DPRINTF("XShmQueryExtension called\n");
	return 0;
}
// required for qt4
Bool XkbQueryExtension(Display *dpy, int *event_base, int *error_base)
{
	DPRINTF("XkbQueryExtension called\n");
	return 0;
}

// required for OpenGL
XExtData **XEHeadOfExtensionList(XEDataObject object)
{
	return *(XExtData ***)&object;
}

int XAddToExtensionList(XExtData **structure, XExtData *ext_data)
{
	ext_data->next = *structure;
	*structure = ext_data;
	return 1;
}

XExtData *XFindOnExtensionList(XExtData **structure, int number)
{
	XExtData *ext;
	ext = *structure;
	while (ext && (ext->number != number)) ext = ext->next;
	return ext;
}
