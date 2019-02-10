#include "nxlib.h"

int
XBell(Display *display, int percent)
{ 
	GrBell();
	return 1;
}
