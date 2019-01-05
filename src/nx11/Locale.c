//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------
#include "nxlib.h"
#include <stdio.h>
#include <stdlib.h>

// required for Xt
char *XSetLocaleModifiers(const char *modifier_list)
{
	DPRINTF("XSetLocaleModifiers called [%s]\n", modifier_list);
	return getenv("LANG");
}
