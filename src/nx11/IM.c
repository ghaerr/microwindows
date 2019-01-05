//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include <string.h>
#include "nxlib.h"
#include "X11/Xlcint.h"		/* this file is missing in some X11 distributions*/

// moved from FontInfo.c to compile with missing Xlcint.h
XFontSetExtents *XExtentsOfFontSet(XFontSet font_set)
{
	//DPRINTF("XExtentsOfFontSet called...\n");
	if (!font_set) return NULL;
	return &font_set->core.font_set_extents;
}

#if 0
typedef enum {
	CREATE_IC = 1,
	SET_ICVAL = 2,
	GET_ICVAL = 3
} XICOp_t;

char *_SetICValueData(XIC ic, XIMArg *values, XICOp_t mode)
{
	XIMArg *p;
	char *return_name = NULL;

	for (p = values; p != NULL && p->name != NULL; p++) {
		if (!strcmp(p->name, XNInputStyle)) {
			if (mode == CREATE_IC)
				ic->core.input_style = (XIMStyle)p->value;
		} else if (!strcmp(p->name, XNClientWindow)) {
			ic->core.client_window = (Window)p->value;
		} else if (!strcmp(p->name, XNFocusWindow)) {
			ic->core.focus_window = (Window)p->value;
		} else if (!strcmp(p->name, XNPreeditAttributes)
			|| !strcmp(p->name, XNStatusAttributes)) {
			return_name = _SetICValueData(ic, (XIMArg*)p->value, mode);
			if (return_name) break;
		} else {
			return_name = p->name;
			break;
		}
	}
	return return_name;
}

char *_GetICValueData(XIC ic, XIMArg *values, XICOp_t mode)
{
	XIMArg *p;
	char *return_name = NULL;

	for (p = values; p->name != NULL; p++) {
		if (!strcmp(p->name, XNInputStyle)) {
			*((XIMStyle *)(p->value)) = ic->core.input_style;
		} else if (!strcmp(p->name, XNClientWindow)) {
			*((Window *)(p->value)) = ic->core.client_window;
		} else if (!strcmp(p->name, XNFocusWindow)) {
			*((Window *)(p->value)) = ic->core.focus_window;
		} else if (!strcmp(p->name, XNFilterEvents)) {
			*((unsigned long *)(p->value))= ic->core.filter_events;
		} else if (!strcmp(p->name, XNPreeditAttributes)
			|| !strcmp(p->name, XNStatusAttributes)) {
			return_name = _GetICValueData(ic, (XIMArg*)p->value, mode);
			if (return_name) break;
		} else {
			return_name = p->name;
			break;
		}
	}
	return return_name;
}

char *_SetICValues(XIC ic, XIMArg *args)
{
	char *ret;
	if (!ic) return args->name;
	ret = _SetICValueData(ic, args, SET_ICVAL);
	return ret;
}

char *_GetICValues(XIC ic, XIMArg *args)
{
	char *ret;
	if (!ic) return args->name;
	ret = _GetICValueData(ic, args, GET_ICVAL);
	return ret;
}

int _MbLookupString(XIC xic, XKeyEvent *ev, char *buffer, int bytes,
	KeySym *keysym, Status *status)
{
	XComposeStatus NotSupportedYet;
	int length;

	length = XLookupString(ev, buffer, bytes, keysym, &NotSupportedYet);
	if (keysym && *keysym == NoSymbol) {
		*status = XLookupNone;
	} else if (length > 0) {
		*status = XLookupBoth;
	} else {
		*status = XLookupKeySym;
	}
	return length;
}

int _WcLookupString(XIC xic, XKeyEvent *ev, wchar_t * buffer, int wlen,
	KeySym *keysym, Status *status)
{
	XComposeStatus NotSupportedYet;
	int length;
	/* In single-byte, mb_len = wc_len */
	char *mb_buf = (char *)Xmalloc(wlen);

	length = XLookupString(ev, mb_buf, wlen, keysym, &NotSupportedYet);
	if (keysym && *keysym == NoSymbol) {
		*status = XLookupNone;
	} else if (length > 0) {
		*status = XLookupBoth;
	} else {
		*status = XLookupKeySym;
	}
	mbstowcs(buffer, mb_buf, length);
	XFree(mb_buf);
	return length;
}

static _Xconst XICMethodsRec local_ic_methods = {
//	_DestroyIC, 		/* destroy */
//	_SetFocus,		/* set_focus */
//	_UnsetFocus,		/* unset_focus */
	NULL,	 		/* destroy */
	NULL,			/* set_focus */
	NULL,			/* unset_focus */
	_SetICValues,		/* set_values */
	_GetICValues,		/* get_values */
//	_MbReset,		/* mb_reset */
//	_WcReset,		/* wc_reset */
	NULL,			/* mb_reset */
	NULL,			/* wc_reset */
	NULL,			/* utf8_reset */		/* ??? */
	_MbLookupString,	/* mb_lookup_string */
	_WcLookupString,	/* wc_lookup_string */
	NULL			/* utf8_lookup_string */	/* ??? */
};

Status _CloseIM(XIM im)
{
	DPRINTF("_CloseIM called [%x]\n", (int)im);
	/*StaticXIM im = (StaticXIM)xim;
	_XlcCloseConverter(im->private->ctom_conv);
	_XlcCloseConverter(im->private->ctow_conv);
	XFree(im->private);*/
	XFree(im->core.im_name);
	if (im->core.res_name) XFree(im->core.res_name);
	if (im->core.res_class) XFree(im->core.res_class);
	return 1; /*bugID 4163122*/
}

char *_SetIMValues(XIM xim, XIMArg *arg)
{
	DPRINTF("_SetIMValues called [%s]\n", arg->name);
	return arg->name;	/* evil */
}

char *_GetIMValues(XIM xim, XIMArg *values)
{
	XIMArg *p;
	XIMStyles *styles;

	DPRINTF("_GetIMValues called [%s]\n", values->name);
	for (p = values; p->name != NULL; p++) {
		if (strcmp(p->name, XNQueryInputStyle) == 0) {
			styles = (XIMStyles*)Xmalloc(sizeof(XIMStyles));
			*(XIMStyles**)p->value = styles;
			styles->count_styles = 1;
			styles->supported_styles =
				(XIMStyle*)Xmalloc(styles->count_styles * sizeof(XIMStyle));
			styles->supported_styles[0] = (XIMPreeditNone | XIMStatusNone);
		} else {
			break;
		}
	}
	return p->name;
}

XIC _CreateIC(XIM im, XIMArg *arg)
{
	XIC ic;

	DPRINTF("_CreateIC called [%x]\n", (int)im);
	if (!(ic = Xcalloc(1, sizeof(XICRec)))) return NULL;

	ic->methods = (XICMethods)&local_ic_methods;
	ic->core.im = im;
	ic->core.filter_events = KeyPressMask;

	if ((_SetICValueData(ic, arg, CREATE_IC) != NULL)
		|| (!(ic->core.input_style))) {
		XFree(ic);
		return 0;
	}

	return ic;
}

static _Xconst XIMMethodsRec local_im_methods = {
	_CloseIM,	/* close */
	_SetIMValues,	/* set_values */
	_GetIMValues, 	/* get_values */
	_CreateIC,	/* create_ic */
	NULL,		/* ctstombs */
	NULL		/* ctstowcs */
};
#endif

// XrmDatabase rdb??
XIM XOpenIM(Display *dpy,
	struct _XrmHashBucketRec *rdb, char *res_name, char *res_class)
{
	XIM im;
	//XIMStaticXIMRec *local_impart;

	DPRINTF("XOpenIM called <name:%s, class:%s>\n", res_name, res_class);
	//DPRINTF("XOpenIM called...\n");
	//if (!(im = Xmalloc(sizeof(XIM)))) return 0;
	if (!(im = Xcalloc(1, sizeof(XIM)))) return 0;
	//if (!(local_impart = Xcalloc(1, sizeof(XIMRec)))) return 0;
	//im->private = local_impart;
	//im->methods        = (XIMMethods)&local_im_methods;
	//im->core.display   = dpy;

#if 0
	// @im=kimera
	strcpy(im->core.im_name, "kimera");

	//im->private = local_impart;
	im->methods        = (XIMMethods)&local_im_methods;
	//im->core.lcd       = lcd;
	im->core.ic_chain  = (XIC)NULL;
	im->core.display   = dpy;
	im->core.rdb       = rdb;
	im->core.res_name  = NULL;
	im->core.res_class = NULL;

/*	local_impart->ctom_conv = ctom_conv;
	local_impart->ctow_conv = ctow_conv;*/
	/*local_impart->ctom_conv = NULL;
	local_impart->ctow_conv = NULL;*/

	if (res_name && *res_name) {
		im->core.res_name  = (char*)Xmalloc(strlen(res_name)+1);
		strcpy(im->core.res_name, res_name);
	}
	if (res_class && *res_class) {
		im->core.res_class = (char*)Xmalloc(strlen(res_class)+1);
		strcpy(im->core.res_class, res_class);
	}
#endif

	return im;
}

Status XCloseIM(XIM im)
{
	DPRINTF("XCloseIM called.. %x\n", (int)im);
	/*StaticXIM im = (StaticXIM)xim;
	_XlcCloseConverter(im->private->ctom_conv);
	_XlcCloseConverter(im->private->ctow_conv);
	XFree(im->private);
	XFree(im->core.im_name);
	if (im->core.res_name) XFree(im->core.res_name);
	if (im->core.res_class) XFree(im->core.res_class);*/
	if (im) Xfree(im);
	return 1; /*bugID 4163122*/
}

// Return the Display associated with the input method.
Display *XDisplayOfIM(XIM im)
{
	DPRINTF("XDisplayOfIM called.. %x\n", (int)im);
	return im->core.display;
}

// Return the Locale associated with the input method.
char *XLocaleOfIM(XIM im)
{
	DPRINTF("XLocaleOfIM called.. %x\n", (int)im);
	return im ? im->core.lcd->core->name : 0;
}


#if 0
void _XIMCountNestedList(XIMArg *args, int *total_count)
{
	for (; args->name; args++) {
		if (!strcmp(args->name, XNVaNestedList))
			_XIMCountNestedList((XIMArg *)args->value, total_count);
		else
			++(*total_count);
	}
}

void _XIMCountVaList(va_list var, int *total_count)
{
	char *attr;
	*total_count = 0;

	for (attr = va_arg(var, char*); attr; attr = va_arg(var, char*)) {
		if (!strcmp(attr, XNVaNestedList)) {
			_XIMCountNestedList(va_arg(var, XIMArg*), total_count);
		} else {
			(void)va_arg(var, XIMArg*);
			++(*total_count);
		}
	}
}

int _XIMNestedListToNestedList(
	XIMArg *nlist,   /* This is the new list */
	XIMArg *list)    /* The original list */
{
	XIMArg *ptr = list;

	while (ptr->name) {
		if (!strcmp(ptr->name, XNVaNestedList)) {
			nlist += _XIMNestedListToNestedList(nlist, (XIMArg *)ptr->value);
		} else {
			nlist->name = ptr->name;
			nlist->value = ptr->value;
			ptr++;
			nlist++;
		}
	}
	return ptr - list;
}

void _XIMVaToNestedList(va_list var, int max_count, XIMArg **args_return)
{
	XIMArg *args;
	char   *attr;

	if (max_count <= 0) {
		*args_return = (XIMArg *)NULL;
		return;
	}

	args = (XIMArg *)Xmalloc((unsigned)(max_count + 1) * sizeof(XIMArg));
	*args_return = args;
	if (!args) return;

	for (attr = va_arg(var, char*); attr; attr = va_arg(var, char*)) {
		if (!strcmp(attr, XNVaNestedList)) {
			args += _XIMNestedListToNestedList(args, va_arg(var, XIMArg*));
		} else {
			args->name = attr;
			args->value = va_arg(var, XPointer);
			args++;
		}
	}
	args->name = (char*)NULL;
}

char *XSetIMValues(XIM im, ...)
{
	DPRINTF("XSetIMValues called\n");
//	return 0;

	va_list var;
	int     total_count;
	XIMArg *args;
	char   *ret;

	// so count the stuff dangling here
	va_start(var, im);
	_XIMCountVaList(var, &total_count);
	va_end(var);

	// now package it up so we can send it along
	va_start(var, im);
	_XIMVaToNestedList(var, total_count, &args);
	va_end(var);

	ret = (*im->methods->set_values) (im, args);
	if (args) Xfree((char *)args);
	return ret;
}

char *XGetIMValues(XIM im, ...)
{
	va_list var;
	int     total_count;
	XIMArg *args;
	char   *ret;

	// so count the stuff dangling here
	va_start(var, im);
	_XIMCountVaList(var, &total_count);
	va_end(var);

	// now package it up so we can send it along
	va_start(var, im);
	_XIMVaToNestedList(var, total_count, &args);
	va_end(var);

	ret = (*im->methods->get_values) (im, args);
	if (args) Xfree((char *)args);
	return ret;
}

// Create an input context within the input method,
// and return a pointer to the input context.
XIC XCreateIC(XIM im, ...)
{
	va_list var;
	int     total_count;
	XIMArg *args;
	XIC     ic;
	DPRINTF("XCreateIC called..\n");

	// so count the stuff dangling here
	va_start(var, im);
	_XIMCountVaList(var, &total_count);
	va_end(var);

	// now package it up so we can send it along
	va_start(var, im);
	_XIMVaToNestedList(var, total_count, &args);
	va_end(var);

	ic = (XIC) (*im->methods->create_ic) (im, args);
	if (args) Xfree((char *)args);
	if (ic) {
		ic->core.next = im->core.ic_chain;
		im->core.ic_chain = ic;
	}
	return ic;
}
#endif
XIC XCreateIC(XIM im, ...)
{
	XIC ic;
	DPRINTF("XCreateIC called...\n");
	if (!(ic = Xcalloc(1, sizeof(XICRec)))) return 0;
	return ic;
}

int XmbLookupString(XIC ic, XKeyPressedEvent *e,
	char *buff, int len, KeySym *ks, Status *status)
{
	DPRINTF("XmbLookupString called..\n");
	*status = XLookupBoth;
	return XLookupString(e, buff, len, ks, (XComposeStatus *)status);

	/*if (ic->core.im)
		return (*ic->methods->mb_lookup_string) (ic, e, buff, len, ks, status);
	return XLookupNone;*/
}
