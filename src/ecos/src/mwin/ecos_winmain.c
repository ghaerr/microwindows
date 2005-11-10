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
// ecos_winmain.c
// 2005-11-10  Alexander Neundorf  <neundorf@kde.org>
//
//  * two functions to invoke WinMain()
//
/////////////////////////////////////////////////////////////////////////////
*/

#include <cyg/kernel/kapi.h>

#include <pkgconf/system.h>

extern void invoke_WinMain(int argc, char** argv);

#define STACK_SIZE (16*1024)

struct ecos_winmain_data
{
	cyg_thread thread;
	cyg_handle_t handle;
	char stack[STACK_SIZE] __attribute__ ((aligned(16)));
	int argc;
	char** argv;
};

static void winmain_mainloop(cyg_addrword_t data)
{
	struct ecos_winmain_data* winmain_data=(struct ecos_winmain_data*)data;
	invoke_WinMain(winmain_data->argc, winmain_data->argv);
}

void start_WinMain_thread(int priority, int argc, char** argv)
{
	static struct ecos_winmain_data winmain_data;
	winmain_data.argc=argc;
	winmain_data.argv=argv;

	cyg_thread_create(priority, winmain_mainloop, (cyg_addrword_t) &winmain_data,
			  "winmain", (void *)  winmain_data.stack, STACK_SIZE,
			  &winmain_data.handle, &winmain_data.thread);

	cyg_thread_resume(winmain_data.handle);
}

void start_WinMain(int argc, char** argv)
{
	invoke_WinMain(argc, argv);
}


#ifndef CYGPKG_LIBC_STARTUP

#include <cyg/infra/diag.h>


void exit(int status)
{
	diag_printf("exit(%d) was called !\n", status);
}

#endif

