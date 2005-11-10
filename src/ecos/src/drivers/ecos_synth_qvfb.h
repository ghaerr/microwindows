/* kate: space-indent off; indent-width 4; replace-tabs-save off; replace-tabs off; show-tabs on;  tab-width 4; */
/*
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2005 - Alexander Neundorf
//
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with eCos; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//
// MODIFICATION/HISTORY:
//
// ecos_synth_qvfb.h
// 2005-11-10  Alexander Neundorf  <neundorf@kde.org>
//
//  * interface for accessing the Qt QVfb from eCos synthetic target
//
/////////////////////////////////////////////////////////////////////////////
*/

#ifndef ECOS_SYNTH_QVFB_H
#define ECOS_SYNTH_QVFB_H

#include <cyg/infra/cyg_type.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QT_VFB_MOUSE_PIPE   "/tmp/.qtvfb_mouse-%d"
#define QT_VFB_KEYBOARD_PIPE    "/tmp/.qtvfb_keyboard-%d"

struct QVFbHeader
{
	int width;
	int height;
	int depth;
	int linestep;
	int dataoffset;
	int update_x1;
	int update_y1;
	int update_x2;
	int update_y2;
	char dirty;
	int  numcols;
};

struct QVFbKeyData
{
	unsigned int unicode;
	unsigned int modifiers;
	char press;
	char repeat;
};

struct QMouseData
{
	/* QPoint */
	int xp;
	int yp;
	/* end of QPoint */
	int buttons;
};

/** Opens the keyboard pipe and returns the file descriptor.
 When done, the pipe has to be closed using cyg_hal_sys_close(fd);*/
int qvfb_open_keyboard(void);

/** Opens the mouse pipe and returns the file descriptor.
 When done, the pipe has to be closed using cyg_hal_sys_close(fd);*/
int qvfb_open_mouse(void);

#ifdef __cplusplus
}
#endif

#endif
