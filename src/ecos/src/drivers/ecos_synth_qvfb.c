/* kate: space-indent off; indent-width 4; replace-tabs-save off; replace-tabs off; show-tabs on;  tab-width 4; */
/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Screen Driver for Linux kernel framebuffers
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 *
 * Modified for eCos by
 *   Alexander Neundorf <neundorf@kde.org>
 *   Gary Thomas <gthomas@redhat.com>
 *   Richard Panton <richard.panton@3glab.org>
 *
 * Note: modify select_fb_driver() to add new framebuffer subdrivers

 ftok is taken from FreeBSD:

 * Copyright (c) 1994 SigmaSoft, Th. Lockert <tholo@sigmasoft.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _GNU_SOURCE 1
#include <assert.h>

#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

#include "ecos_synth_qvfb.h"

#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>

// code for accessing the Qt virtual framebuffer

struct QVFbHeader* qvfb_header=NULL;

char* qvfb_connect()
{
	int sid=0;
	char pipeName[256];
	int displayId=0;
	cyg_uint32 key;

	if (qvfb_header)
		return (char*) qvfb_header;

	snprintf(pipeName, 256, QT_VFB_MOUSE_PIPE, displayId);
	key=cyg_hal_sys_ftok(pipeName, 'b');

	sid=cyg_hal_sys_shmget(key, 0, 0);
	if (sid<0)
		return NULL;
	char* qvfb_buf=(char*)cyg_hal_sys_shmat(sid, 0, 0);
	if (!qvfb_buf)
		return NULL;

	qvfb_header=(struct QVFbHeader*)qvfb_buf;
	return qvfb_buf+qvfb_header->dataoffset;
}

void qvfb_update(void)
{
	if (!qvfb_header)
		return;
	qvfb_header->update_x1=0;
	qvfb_header->update_y1=0;
	qvfb_header->update_x2=qvfb_header->width;
	qvfb_header->update_y2=qvfb_header->height;
	qvfb_header->dirty=0xff;
}

void qvfb_update_rect(int x1, int y1, int x2, int y2)
{
	if (!qvfb_header)
		return;
	qvfb_header->update_x1=x1;
	qvfb_header->update_y1=y1;
	qvfb_header->update_x2=x2;
	qvfb_header->update_y2=y2;
	qvfb_header->dirty=0xff;
}

void qvfb_disconnect(void)
{
	if (!qvfb_header)
		return;
	cyg_hal_sys_shmdt(qvfb_header);
	qvfb_header=NULL;
}

int qvfb_width(void)
{
	if (!qvfb_header)
		return -1;
	return qvfb_header->width;
}

int qvfb_height(void)
{
	if (!qvfb_header)
		return -1;
	return qvfb_header->height;
}

int qvfb_depth(void)
{
	if (!qvfb_header)
		return -1;
	return qvfb_header->depth;
}

int qvfb_linestep(void)
{
	if (!qvfb_header)
		return -1;
	return qvfb_header->linestep;
}

int qvfb_open_keyboard()
{
	char pipeName[256];
	int displayId=0;

	snprintf(pipeName, 256, QT_VFB_KEYBOARD_PIPE, displayId);
	int fd=cyg_hal_sys_open(pipeName, CYG_HAL_SYS_O_RDONLY, 0);
//	diag_printf("keyboard fd: %d\n", fd);
	return fd;
}

int qvfb_open_mouse()
{
	char pipeName[256];
	int displayId=0;

	snprintf(pipeName, 256, QT_VFB_MOUSE_PIPE, displayId);
	int fd=cyg_hal_sys_open(pipeName, CYG_HAL_SYS_O_RDONLY, 0);
//	diag_printf("mouse fd: %d\n", fd);
	return fd;
}



