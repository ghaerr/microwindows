/* kate: space-indent off; indent-width 4; replace-tabs-save off; replace-tabs off; show-tabs on;  tab-width 4; */
/*
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2005 - Alexander Neundorf
//
// MODULE DESCRIPTION:
// This module defines the interface for input devices used by MicroWindows
// in an embedded system environment. Originally implemented for RTEMS, this
// implementation works for eCos.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//  1. Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//  3. The name of the author may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
//  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
//  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//
// MODIFICATION/HISTORY:
//
// mw_uid_ecos_qvfb.cxx
// 2005-10-17  Alexander Neundorf  <neundorf@kde.org>
//
//	* UID implementation for QVfb
//
/////////////////////////////////////////////////////////////////////////////
*/

#define _GNU_SOURCE 1
#include <assert.h>

#include "ecos_synth_qvfb.h"

#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/kernel/kapi.h>

#include <rtems/mw_uid.h>

#include <mwtypes.h>

struct QMouseData mouse_data;
int mouse_data_fill=0;

struct QVFbKeyData key_data;
int key_data_fill=0;

static void qvfb_read_mouse_data(int fd)
{
	char* buf=(char*)&mouse_data;
	int res=cyg_hal_sys_read(fd, buf+mouse_data_fill, sizeof(struct QMouseData)-mouse_data_fill);

	if (res<0)
		diag_printf("mouse read returned %d\n", res);
	else if (res==0)
		diag_printf("mouse read returned 0!\n");
	else
	{
		mouse_data_fill+=res;
		if (mouse_data_fill==sizeof(struct QMouseData))
		{
			struct MW_UID_MESSAGE msg;
			msg.type=MV_UID_ABS_POS;
			msg.m.pos.x=mouse_data.xp;
			msg.m.pos.y=mouse_data.yp;
			msg.m.pos.btns=0;
			if (mouse_data.buttons & 0x01)
				msg.m.pos.btns|=MV_BUTTON_LEFT;
			if (mouse_data.buttons & 0x2)
				msg.m.pos.btns|=MV_BUTTON_RIGHT;
			if (mouse_data.buttons & 0x04)
				msg.m.pos.btns|=MV_BUTTON_CENTER;

			uid_write_message( &msg );
			mouse_data_fill=0;
		}
	}
}

static void qvfb_read_keyboard_data(int fd)
{
	char* buf=(char*)&key_data;
	int res=cyg_hal_sys_read(fd, buf+key_data_fill, sizeof(struct QVFbKeyData)-key_data_fill);

	if (res<0)
		diag_printf("key read returned %d\n", res);
	else if (res==0)
		diag_printf("key read returned 0!\n");
	else
	{
		key_data_fill+=res;
		if (key_data_fill==sizeof(struct QVFbKeyData))
		{
			short keycode=key_data.unicode & 0xffff;
			diag_printf("u: 0x%x, mod: 0x%x, press: 0x%x repeat: %d\n", key_data.unicode, key_data.modifiers, (unsigned int)key_data.press, (unsigned int)key_data.repeat);
			struct MW_UID_MESSAGE msg;
			msg.type=MV_UID_KBD;
			msg.m.kbd.mode= (key_data.press==1)?1:2;
			key_data_fill=0;

			if (keycode!=0)
			{
				msg.m.kbd.code=keycode;
			}
			else
			{
				switch ((key_data.unicode >> 16) & 0xffff)
				{
//            case 0x1010:
//               msg.m.kbd.code=MWKEY_FIRST;
//               break;
					case 0x1012:
						msg.m.kbd.code=MWKEY_LEFT;
						break;
					case 0x1014:
						msg.m.kbd.code=MWKEY_RIGHT;
						break;
					case 0x1013:
						msg.m.kbd.code=MWKEY_UP;
						break;
					case 0x1015:
						msg.m.kbd.code=MWKEY_DOWN;
						break;
					case 0x1006:
						msg.m.kbd.code=MWKEY_INSERT;
						break;
					case 0x1007:
						msg.m.kbd.code=MWKEY_DELETE;
						break;
					case 0x1010:
						msg.m.kbd.code=MWKEY_HOME;
						break;
					case 0x1011:
						msg.m.kbd.code=MWKEY_END;
						break;
					case 0x1016:
						msg.m.kbd.code=MWKEY_PAGEUP;
						break;
					case 0x1017:
						msg.m.kbd.code=MWKEY_PAGEDOWN;
						break;
					case 0x1030:
						msg.m.kbd.code=MWKEY_F1;
						break;
					case 0x1031:
						msg.m.kbd.code=MWKEY_F2;
						break;
					case 0x1032:
						msg.m.kbd.code=MWKEY_F3;
						break;
					case 0x1033:
						msg.m.kbd.code=MWKEY_F4;
						break;
					case 0x1034:
						msg.m.kbd.code=MWKEY_F5;
						break;
					case 0x1035:
						msg.m.kbd.code=MWKEY_F6;
						break;
					case 0x1036:
						msg.m.kbd.code=MWKEY_F7;
						break;
					case 0x1037:
						msg.m.kbd.code=MWKEY_F8;
						break;
					case 0x1038:
						msg.m.kbd.code=MWKEY_F9;
						break;
					case 0x1039:
						msg.m.kbd.code=MWKEY_F10;
						break;
					case 0x103a:
						msg.m.kbd.code=MWKEY_F11;
						break;
					case 0x103b:
						msg.m.kbd.code=MWKEY_F12;
						break;
					case 0x1025:
						msg.m.kbd.code=MWKEY_NUMLOCK;
						break;
					case 0x1024:
						msg.m.kbd.code=MWKEY_CAPSLOCK;
						break;
					case 0x1026:
						msg.m.kbd.code=MWKEY_SCROLLOCK;
						break;
					case 0x1020:
						msg.m.kbd.code=MWKEY_LSHIFT;
						break;
					case 0x1021:
						msg.m.kbd.code=MWKEY_LCTRL;
						break;
					case 0x1023:
						msg.m.kbd.code=MWKEY_LALT;
						break;
					case 0xffff:
						msg.m.kbd.code=MWKEY_ALTGR;
						break;
						/* Numeric keypad*/
/*#define MWKEY_KP0		0xF80A
#define MWKEY_KP1		0xF80B
#define MWKEY_KP2		0xF80C
#define MWKEY_KP3		0xF80D
#define MWKEY_KP4		0xF80E
#define MWKEY_KP5		0xF80F
#define MWKEY_KP6		0xF810
#define MWKEY_KP7		0xF811
#define MWKEY_KP8		0xF812
#define MWKEY_KP9		0xF813
#define MWKEY_KP_PERIOD		0xF814
#define MWKEY_KP_DIVIDE		0xF815
#define MWKEY_KP_MULTIPLY	0xF816
#define MWKEY_KP_MINUS		0xF817
#define MWKEY_KP_PLUS		0xF818
#define MWKEY_KP_ENTER		0xF819
#define MWKEY_KP_EQUALS		0xF81A*/
					default:
						return; //no known key
				}
			}
			uid_write_message( &msg );
		}
	}
}

#define STACK_SIZE (16*1024)

struct QVfbMwuidInitializer
{
	QVfbMwuidInitializer();

	cyg_thread thread;
	cyg_handle_t handle;
	char stack[STACK_SIZE] __attribute__ ((aligned(16)));
};

QVfbMwuidInitializer g_qvfbMwuid;

externC void qvfb_run_mwuid_mainloop(cyg_addrword_t data)
{
	int keyboardFd=-1;
	int mouseFd=-1;

	keyboardFd=qvfb_open_keyboard();
	mouseFd=qvfb_open_mouse();

	diag_printf("***** keyboard: %d mouse: %d %p\n", keyboardFd, mouseFd, g_qvfbMwuid.stack);

	while (1)
	{
		struct cyg_hal_sys_fd_set readFds;
		struct cyg_hal_sys_timeval tv;
		int maxFd=-1;
		tv.hal_tv_sec=0;
		tv.hal_tv_usec=900*1000;

		CYG_HAL_SYS_FD_ZERO(&readFds);
		if (mouseFd>=0)
		{
			CYG_HAL_SYS_FD_SET(mouseFd, &readFds);
			maxFd=mouseFd;
		}

		if (keyboardFd>=0)
		{
			if (keyboardFd>maxFd)
				maxFd=keyboardFd;
			CYG_HAL_SYS_FD_SET(keyboardFd, &readFds);
		}

		int res=cyg_hal_sys__newselect(maxFd+1, &readFds, NULL, NULL, &tv);

		if (res<0)
		{
			if (res==-4) // ignore EINTR
			{
				cyg_thread_delay(1);
			}
			else
				diag_printf("cyg_hal_sys_select() error %d\n", res);
		}
		else if (res==0)
		{
         //timeout
		}
		else
		{
			if (CYG_HAL_SYS_FD_ISSET(mouseFd, &readFds))
			{
				qvfb_read_mouse_data(mouseFd);
			}
			if (CYG_HAL_SYS_FD_ISSET(keyboardFd, &readFds))
			{
				qvfb_read_keyboard_data(keyboardFd);
			}
		}
	}

	if (mouseFd>=0)
		cyg_hal_sys_close(mouseFd);

	if (keyboardFd>=0)
		cyg_hal_sys_close(keyboardFd);
}

QVfbMwuidInitializer::QVfbMwuidInitializer()
{
	uid_register_device(int(this), "mwuid_qvfb" ); // this does nothing right now

	cyg_thread_create(4, qvfb_run_mwuid_mainloop, (cyg_addrword_t) 1, "mwuid_qvfb",
			  (void *)  stack, STACK_SIZE, &handle, &thread);

	cyg_thread_resume(handle);
}
