/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Nano-X Core Protocol Client Request Handling Routines
 */ 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "serv.h"
#include "nxproto.h"

#define SZREQBUF	2048	/* initial request buffer size*/

#ifndef __ECOS
static REQBUF	reqbuf;		/* request buffer*/
extern int 	nxSocket;
extern char *	nxSharedMem;
#endif

/* Allocate a request buffer of passed size and fill in header fields*/
void *
nxAllocReq(int type, long size, long extra)
{
	nxReq *	req;
	long	aligned_len;
        ACCESS_PER_THREAD_DATA();

	/* variable size requests must be hand-padded*/
	if(extra)
		assert((size & (long)(ALIGNSZ-1)) == 0);

	/* calculate aligned length of request buffer*/
	aligned_len = (size + extra + (long)(ALIGNSZ-1)) & ~(long)(ALIGNSZ-1);

	/* verify we're not greater than max request size*/
	assert(aligned_len <= MAXREQUESTSZ);

	/* flush buffer if required, and allocate larger one if required*/
	if(reqbuf.bufptr + aligned_len >= reqbuf.bufmax)
		nxFlushReq(aligned_len,1);

	/* fill in request header*/
	req = (nxReq *)reqbuf.bufptr;
	req->reqType = (BYTE8)type;
	req->hilength = (BYTE8)((size + extra) >> 16);
	req->length = (UINT16)(size + extra);
	reqbuf.bufptr += aligned_len;
	return req;
}

static void nxAllocReqbuffer(long newsize)
{
        ACCESS_PER_THREAD_DATA();

	if(newsize < (long)SZREQBUF)
		newsize = SZREQBUF;
	reqbuf.buffer = malloc(newsize);
	if(!reqbuf.buffer) {
		EPRINTF("nxFlushReq: Can't allocate initial request buffer\n");
		exit(1);
	}
	reqbuf.bufptr = reqbuf.buffer;
	reqbuf.bufmax = reqbuf.buffer + newsize;
}

void
nxAssignReqbuffer(char *buffer, long size)
{
        ACCESS_PER_THREAD_DATA();

	if ( reqbuf.buffer != 0 )
		free(reqbuf.buffer);
	reqbuf.buffer = buffer;
	reqbuf.bufptr = reqbuf.buffer;
	reqbuf.bufmax = reqbuf.buffer + size;
}

/* Write a block of data on the socket to the nano-X server */
void
nxWriteSocket(char *buf, int todo)
{
	int written;
        ACCESS_PER_THREAD_DATA();

	do {
		written = write(nxSocket, buf, todo);
		if ( written < 0 ) {
			if ( errno == EAGAIN || errno == EINTR )
				continue;
			EPRINTF("nxFlushReq: write failed: %m\n");
			exit(1);
		}
		buf += written;
		todo -= written;
	} while ( todo > 0 );
}

/* Flush request buffer if required, possibly reallocate buffer size*/
void
nxFlushReq(long newsize, int reply_needed)
{
        ACCESS_PER_THREAD_DATA();

	/* handle one-time initialization case*/
	if(reqbuf.buffer == NULL) {
		nxAllocReqbuffer(newsize);
		return;
	}

	/* flush buffer if required*/
	if(reqbuf.bufptr > reqbuf.buffer) {
		char *	buf = reqbuf.buffer;
		int	todo = reqbuf.bufptr - reqbuf.buffer;

#if HAVE_SHAREDMEM_SUPPORT
		if ( nxSharedMem != 0 ) {
			/* There is a shared memory segment used for the
			 * request buffer.  Make up a flush command and
			 * send it over the socket, to tell the server to
			 * process the shared memory segment.
			 * The 'reply_needed' argument should be non-zero
			 * when a confirmation is needed that all commands
			 * are flushed, so new ones can be filled into the
			 * request buffer.  NOTE:  This is *only* needed
			 * when explicitely flushing the request buffer, or
			 * when flushing it to make space for new commands.
			 * DO NOT REQUEST A REPLY when flushing the request
			 * buffer because the last command in the buffer
			 * will send a response:  This response would be
			 * queued up first and had to be drained before the
			 * response to the flush command itsel....
			 * So the GrReadBlock used to read replys to commands
			 * must not specify a nonzero 'reply_needed'.
			 * Not requesting a reply in this case is
			 * safe, since the command executed will wait for
			 * the reply *it* is waiting for, and thus make
			 * sure the request buffer is flushed before
			 * continuing.
			 *
			 * We have to make the protocol request by hand,
			 * as it has to be sent over the socket to wake
			 * up the Nano-X server.
			 */
			char c;
			nxShmCmdsFlushReq req;

			req.reqType = GrNumShmCmdsFlush;
			req.hilength = 0;
			req.length = sizeof(req);
			req.size = todo;
			req.reply = reply_needed;

			nxWriteSocket((char *)&req,sizeof(req));

			if ( reply_needed )
				while ( read(nxSocket, &c, 1) != 1 )
					;

			reqbuf.bufptr = reqbuf.buffer;

			if ( reqbuf.buffer + newsize > reqbuf.bufmax ) {
				/* Shared memory too small, critical */
				EPRINTF("nxFlushReq: shm region too small\n");
				exit(1);
			}
			return;
		}
#endif /* HAVE_SHAREDMEM_SUPPORT*/

		/* Standard Socket transfer */
		nxWriteSocket(buf,todo);
		reqbuf.bufptr = reqbuf.buffer;
	}

	/* allocate larger buffer for current request, if needed*/
	if(reqbuf.bufptr + newsize >= reqbuf.bufmax) {
		reqbuf.buffer = realloc(reqbuf.buffer, newsize);
		if(!reqbuf.buffer) {
		       EPRINTF("nxFlushReq: Can't reallocate request buffer\n");
			exit(1);
		}
		reqbuf.bufptr = reqbuf.buffer;
		reqbuf.bufmax = reqbuf.buffer + newsize;
	}
}

/* calc # bytes required for passed string according to encoding*/
int
nxCalcStringBytes(void *str, int count, int flags)
{
	int	nbytes;

	/* calc byte length of data*/
	if(flags & MWTF_UC16)
		nbytes = count * 2;
	else if(flags & MWTF_UC32)
		nbytes = count * 4;
	else
		nbytes = count;

	return nbytes;
}
