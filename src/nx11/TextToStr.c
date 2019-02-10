//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------

#include "nxlib.h"
#include "X11/Xutil.h"
#include "X11/Xatom.h"
#include <string.h>
#include <stdlib.h>

// XTextPropertyToStringList - set list and count to contain data stored in
// null-separated STRING property.
Status XTextPropertyToStringList(XTextProperty *tp, char ***list_r, int *count_r)
{
	char **list;			/* return value */
	int nelements;			/* return value */
	register char *cp;		/* temp variable */
	char *start;			/* start of thing to copy */
	int i, j;			/* iterator variables */
	int datalen = (int) tp->nitems;	/* for convenience */

	// make sure we understand how to do it
	DPRINTF("XTextPropertyToStringList called..\n");
	if (tp->encoding != XA_STRING ||  tp->format != 8) return False;

	if (datalen == 0) {
		*list_r = NULL;
		*count_r = 0;
		return True;
	}

	// walk the list to figure out how many elements there are
	nelements = 1;			/* since null-separated */
	for (cp = (char*)tp->value, i = datalen; i>0; cp++, i--) {
		if (*cp == '\0') nelements++;
	}

	// allocate list and duplicate
	list = (char**)Xmalloc(nelements * sizeof(char*));
	if (!list) return False;

	start = (char*)Xmalloc((datalen + 1) * sizeof(char));	/* for <NUL> */
	if (!start) {
		Xfree((char*)list);
		return False;
	}

	// copy data
	memcpy(start, (char*)tp->value, tp->nitems);
	start[datalen] = '\0';

	// walk down list setting value
	for (cp = start, i = datalen + 1, j = 0; i>0; cp++, i--) {
		if (*cp == '\0') {
			list[j] = start;
			start = (cp + 1);
			j++;
		}
	}

	// append final null pointer and then return data
	*list_r = list;
	*count_r = nelements;
	return True;
}

void XFreeStringList(char **list)
{
	DPRINTF("XFreeStringList called..\n");
	if (list) {
		if (list[0]) Xfree(list[0]);
		Xfree((char*)list);
		list = NULL;
	}
}
