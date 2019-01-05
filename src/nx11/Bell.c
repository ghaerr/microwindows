#include "nxlib.h"

int
XBell(Display *display, int percent)
{ 
/* Don't bother getting complicated here */
	printf("\7");
	return 1;
}
