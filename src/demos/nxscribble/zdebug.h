/***********************************************************************

zdebug.h - macros for debugging

Copyright (C) 1991 Dean Rubine

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License. See ../COPYING for
the full agreement.

**********************************************************************/

extern char _zdebug_flag[];

#define Z(f) if(_zdebug_flag[f] > 0)
#define ZZ(f) if(_zdebug_flag[f] >= 2)
#define ZZZ(f) if(_zdebug_flag[f] >= 3)
#define ZZZZ(f) if(_zdebug_flag[f] >= 4)

extern double kludge[];
