/*
 * Copyright (c) 2000 Victor Rogachev <rogach@sut.ru>
 *
 * Int10 function for PACIFIC C on MSDOS
 */

#include "device.h"
#include "vgaplan4.h"

/*
**	Uses int 10 for graphics
*/

FARADDR 
int10(int ax, int bx)
{
	union REGPACK reg;

	reg.x.ax = ax;        
	reg.x.bx = bx;        
	intr(0x10, &reg);

	return ((FARADDR) MK_FP(reg.x.es, reg.x.bp));
}
