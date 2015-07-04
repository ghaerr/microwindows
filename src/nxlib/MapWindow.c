#include "nxlib.h"

int
XMapWindow(Display *dpy, Window w)
{
	GrMapWindow(w);
	return 1;
}

int
XMapSubwindows(Display *dpy, Window w)
{
	GR_WINDOW_ID	parent;
	GR_WINDOW_ID	*child, *cp;
	int		i, nchild;

	GrQueryTree(w, &parent, &child, &nchild);

	cp = child;
	for (i=0; i<nchild; ++i)
		GrMapWindow(*cp++);

	free(child);
	return 1;
}
