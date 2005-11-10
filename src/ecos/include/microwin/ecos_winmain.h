/* kate: space-indent off; indent-width 8; replace-tabs-save off; replace-tabs off; show-tabs on;  tab-width 8; */
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
// ecos_winmain.h
// 2005-11-10  Alexander Neundorf  <neundorf@kde.org>
//
//  * two functions to invoke WinMain()
//
/////////////////////////////////////////////////////////////////////////////
*/

#ifndef ECOS_WINMAIN_H
#define ECOS_WINMAIN_H

/* starts WinMain() with the given argc and argv from a newly created thread */
void start_WinMain_thread(int priority, int argc, char** argv);

/* starts WinMain() with the given argc and argv from the current thread */
void start_WinMain(int argc, char** argv);


#endif
