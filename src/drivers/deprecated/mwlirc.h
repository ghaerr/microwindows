/*
 * Client library for LIRC.
 *
 * Written by Koninklijke Philips Electronics N.V.
 *
 * Portions contributed by Koninklijke Philips Electronics N.V.
 * These portions are Copyright 2002-2003 Koninklijke Philips Electronics
 * N.V.  All Rights Reserved.  These portions are licensed under the
 * terms of the Mozilla Public License, version 1.1, or, at your
 * option, the GNU General Public License version 2.0.  Please see
 * the file "ChangeLog" for documentation regarding these
 * contributions.
 *
 * This is an independent implementation.  For license reasons,
 * it is *NOT* based on the client library included with LIRC.
 *
 */

#ifndef MWLIRC_H_INCLUDED
#define MWLIRC_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#define MWLIRC_ERROR_AGAIN    1
#define MWLIRC_ERROR_SOCKET   2
#define MWLIRC_ERROR_PROTOCOL 3
#define MWLIRC_ERROR_MEMORY   4

#define MWLIRC_BLOCK    0
#define MWLIRC_NONBLOCK 1

#define MWLIRC_KEY_NAME_LENGTH 28
#define MWLIRC_RC_NAME_LENGTH 16

	typedef struct mwlirc_keystroke_
	{
		int repeatcount;
		char name[MWLIRC_KEY_NAME_LENGTH];
		char rc[MWLIRC_RC_NAME_LENGTH];
	}
	mwlirc_keystroke;


	extern int mwlirc_fd;

	extern int mwlirc_init(int nonblocking);
	extern void mwlirc_close(void);
	extern int mwlirc_read_keystroke(mwlirc_keystroke * key);
	extern int mwlirc_read_keystroke_norepeat(mwlirc_keystroke * key);

#ifdef __cplusplus
}
#endif

#endif	/* MWLIRC_H_INCLUDED */

