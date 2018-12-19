//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"
#include <string.h>

// required for Qt
Bool XkbSetPerClientControls(Display *dpy, unsigned change, unsigned *values)
{
	DPRINTF("XkbSetPerClientControls called ???\n");
	return 0;
}

// required for wine
Bool XkbSetDetectableAutoRepeat(Display *dpy, Bool detectable, Bool *supported)
{
	DPRINTF("XkbSetDetectableAutoRepeat called ???\n");
	return 0;
}

unsigned XkbKeysymToModifiers(Display *dpy, KeySym ks)
{
	//return XKeysymToModifiers(dpy, ks);
	DPRINTF("XkbKeysymToModifiers called [%lx]???\n", ks);
	return 0;
}

KeySym XkbKeycodeToKeysym(Display *dpy, KeyCode kc, unsigned int group, unsigned int level)
{
	DPRINTF("XkbKeycodeToKeysym called [%x]???\n", kc);
	return XKeycodeToKeysym(dpy, kc, 0);
}

Bool XkbUseExtension(Display *dpy, int *major, int *minor)
{
	DPRINTF("XkbUseExtension called...\n");
	return 0;
}

#if 0
struct _XKeytrans {
	struct _XKeytrans *next;/* next on list */
	char *string;		/* string to return when the time comes */
	int len;		/* length of string (since NULL is legit)*/
	KeySym key;		/* keysym rebound */
	unsigned int state;	/* modifier state */
	KeySym *modifiers;	/* modifier keysyms you want */
	int mlen;		/* length of modifier list */
};

int XkbLookupKeyBinding(Display *dpy, KeySym sym, unsigned int mods,
	char *buffer, int nbytes, int *extra_rtrn)
{
	struct _XKeytrans *p;

	if (extra_rtrn) *extra_rtrn = 0;
	for (p = dpy->key_bindings; p; p = p->next) {
		if (((mods & AllMods) == p->state) && (sym == p->key)) {
			int tmp = p->len;
			if (tmp > nbytes) {
				if (extra_rtrn) *extra_rtrn = (tmp-nbytes);
				tmp = nbytes;
			}
			memcpy(buffer, p->string, tmp);
			if (tmp < nbytes) buffer[tmp] = '\0';
			return tmp;
		}
	}
	return 0;
}
#endif

int XkbTranslateKeySym(Display *dpy, KeySym *sym, unsigned int mods,
	char *buf, int nbytes, int *extra)
{
	//char tmp[4];
	char *p;

	p = XKeysymToString(sym[0]);
	nbytes = strlen(p);
	memcpy(buf, p, nbytes);
	DPRINTF("XkbTranslateKeySym called [%lx/%s]...\n", *sym, p);
	if (extra) *extra = 0;
	/*if (!buf || !nbytes) {
		buf = tmp;
		nbytes = 4;
	}*/
	return nbytes;

#if 0
	XkbInfoPtr xkb;
	XkbKSToMBFunc cvtr;
	XPointer priv;
	char tmp[4];
	int n;

	xkb = dpy->xkb_info;
	if (!xkb->cvt.KSToMB) {
		_XkbGetConverters(_XkbGetCharset(), &xkb->cvt);
		_XkbGetConverters("ISO8859-1", &xkb->latin1cvt);
	}

	if (extra) *extra = 0;
	if (!buf || !nbytes) {
		buf = tmp;
		nbytes = 4;
	}

	/* see if symbol rebound, if so, return that string. */
	n = XkbLookupKeyBinding(dpy, *sym, mods, buf, nbytes, extra);
	if (n) return n;
	if (nbytes>0) buf[0] = '\0';

	if (xkb->cvt.KSToUpper && (mods&LockMask)) {
		*sym = (*xkb->cvt.KSToUpper)(*sym);
	}
	if (xkb->xlib_ctrls & XkbLC_ForceLatin1Lookup) {
		cvtr = xkb->latin1cvt.KSToMB;
		priv = xkb->latin1cvt.KSToMBPriv;
	} else {
		cvtr = xkb->cvt.KSToMB;
		priv = xkb->cvt.KSToMBPriv;
	}

	n = (*cvtr)(priv, *sym, buf, nbytes, extra);

	if ((!xkb->cvt.KSToUpper) && (mods&LockMask)) {
		int i;
		int change;
		char ch;
		for (i=change=0; i<n; i++) {
			ch = toupper(buf[i]);
			change = (change||(buf[i]!=ch));
			buf[i] = ch;
		}
		if (change) {
			if (n==1) *sym = (*xkb->cvt.MBToKS)(xkb->cvt.MBToKSPriv, buf, n, 0);
			else *sym = NoSymbol;
		}
	}

	if (mods&ControlMask) {
		if (n==1) {
			buf[0] = XkbToControl(buf[0]);
			if (nbytes>1) buf[1] = '\0';
			return 1;
		}
		if (nbytes>0) buf[0]= '\0';
		return 0;
	}
	return n;
#endif
}
