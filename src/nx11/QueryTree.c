#include "nxlib.h"

Status
XQueryTree(Display * display, Window w, Window * root_return,
	   Window * parent_return,
	   Window ** children_return, unsigned int *nchildren_return)
{
	*root_return = GR_ROOT_WINDOW_ID;

	GrQueryTree(w, (GR_WINDOW_ID *) parent_return,
		    (GR_WINDOW_ID **) children_return, (GR_COUNT *)nchildren_return);
	return 1;
}
