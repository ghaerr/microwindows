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
// ecos_mw_uid.cxx
// 2005-10-17  Alexander Neundorf  <neundorf@kde.org>
//
//	* Simple implementation of the Uniform Input Device interface
//	* as used by the RTEMS Microwindows drivers. So less code for eCos
// 	* support has to be added.
//
/////////////////////////////////////////////////////////////////////////////
*/

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/error/errno.h>
#include <string.h>

#include <rtems/mw_uid.h>

/* The Uniform Input Device interface for eCos is implemented using
 an eCos message queue and an accompanying pool of messages to be sent
 over this queue. The queue is created automatically when the application
 starts by the constructor of MwUidQueue. That's why uid_open_queue() and
 uid_close_queue() can be empty functions. Also there is no real need for
 uid_register_device() and uid_unregister_device().
 Implemented mouse/keyboard drivers simply call uid_open_queue() and then
 write keyboard/mouse events using uid_write_message().
*/

struct MwUidQueue
{
	MwUidQueue();
	cyg_handle_t m_poolHandle;       // the handle for the pool
	cyg_mempool_fix m_pool;          // the pool for the messages
	MW_UID_MESSAGE m_poolBuffer[20]; // the memory area for the message pool

	cyg_handle_t m_mboxHandle;       // the handle for the message queue
	cyg_mbox m_mbox;                 // the message queue
};

/* create the pool of messages and the message queue */
MwUidQueue::MwUidQueue()
{
	cyg_mempool_fix_create(m_poolBuffer, sizeof(m_poolBuffer), sizeof(MW_UID_MESSAGE), &m_poolHandle, &m_pool);
	cyg_mbox_create(&m_mboxHandle, &m_mbox);
}

/* this will create the message queue when all ctors are executed during app startup */
MwUidQueue g_mwUidQueue;

extern "C"
{

/* creates the message queue that holds events from the input devices */
int uid_open_queue( const char *q_name, int flags, size_t max_msgs )
{
	return 0;
}

/* closes message queue (not really)*/
int uid_close_queue( void )
{
	return 0;
}

/* Reads a message from the queue. It waits up to the specified
 * timeout in mili-seconds.
 */
int uid_read_message( struct MW_UID_MESSAGE *m, unsigned long timeout )
{
	MW_UID_MESSAGE* msg=(MW_UID_MESSAGE*)cyg_mbox_timed_get(g_mwUidQueue.m_mboxHandle, cyg_current_time()+(timeout/10));
	if (msg==0)
		return ETIMEDOUT;

	memcpy(m, msg, sizeof(MW_UID_MESSAGE));
	cyg_mempool_fix_free(g_mwUidQueue.m_poolHandle, msg);
	return sizeof(MW_UID_MESSAGE);
}

/* Write a message to the queue.
 * This can block of the pool is empty or the queue is full.
 */
int uid_write_message( struct MW_UID_MESSAGE *m )
{
	MW_UID_MESSAGE* msg=(MW_UID_MESSAGE*)cyg_mempool_fix_alloc(g_mwUidQueue.m_poolHandle);
	memcpy(msg, m, sizeof(MW_UID_MESSAGE));
	cyg_mbox_put(g_mwUidQueue.m_mboxHandle, msg);
	return 0;
}


/* register device to insert data to the queue, nothing to do under eCos */
int uid_register_device( int fd, const char *q_name )
{
	diag_printf("reg %d\n", fd);
	return 0;
}

/* unregister device to stop adding messages to the queue, nothing to do under eCos */
int uid_unregister_device( int fd )
{return 0;}

/* set the keyboard, nothing to do under eCos */
int uid_set_kbd_mode( int fd, int mode, int *old_mode )
{
	return 0;
}


}

