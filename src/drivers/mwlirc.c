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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "mwlirc.h"
#include "mwsystem.h"

/**
 * The file descriptor for the LIRC socket.
 */
int mwlirc_fd = -1;


#define MWLIRC_BUF_SIZE_MIN 100

static char *mwlirc_buf = NULL;
static int mwlirc_buf_size = 0;
static int mwlirc_buf_used = 0;
static int mwlirc_buf_line_len = 0;

#define MWLIRC_EXPECT_BEGIN         -1
#define MWLIRC_EXPECT_COMMAND       -2
#define MWLIRC_EXPECT_SUCCESS_ERROR -3
#define MWLIRC_EXPECT_DATA          -4
#define MWLIRC_EXPECT_DATACOUNT     -5
#define MWLIRC_EXPECT_END            0

static int mwlirc_packet_state = MWLIRC_EXPECT_BEGIN;


#ifdef TRIMEDIA
static char * my_strchr(char * buf, char ch)
{
	while ((*buf != '\0') & (*buf != ch)) buf++;
	return (*buf ? buf : NULL);
}
#undef strchr
#define strchr my_strchr
#endif

/**
 * Initialize LIRC.
 *
 * @return the file descriptor, or -1 on error.
 */
int
mwlirc_init(int nonblocking)
{
	int fd;

	if (mwlirc_fd != -1) {
		/* Already initialized - fail. */
		return -1;
	}

	mwlirc_packet_state = MWLIRC_EXPECT_BEGIN;
	mwlirc_buf_used = 0;
	mwlirc_buf_line_len = 0;

	mwlirc_buf = malloc(MWLIRC_BUF_SIZE_MIN);
	if (mwlirc_buf == NULL) {
		mwlirc_buf_size = 0;
		return -1;
	}

	mwlirc_buf_size = MWLIRC_BUF_SIZE_MIN;
	mwlirc_buf[0] = '\0';

	{
		struct sockaddr_un addr;

		fd = socket(PF_UNIX, SOCK_STREAM, 0);
		if (fd == -1) {
			free(mwlirc_buf);
			mwlirc_buf = NULL;
			mwlirc_buf_size = 0;
			return -1;
		}

		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, "/dev/lircd");

		if (0 != connect(fd, (struct sockaddr *) &addr, sizeof(addr))) {
			free(mwlirc_buf);
			mwlirc_buf = NULL;
			mwlirc_buf_size = 0;
			close(fd);
			return -1;
		}

		if (nonblocking) {
			fcntl(fd, F_SETFL, O_NONBLOCK);
		}
	}

	mwlirc_fd = fd;
	return fd;
}


/**
 * Close LIRC.
 */
void
mwlirc_close(void)
{
	if (mwlirc_fd != -1) {
		close(mwlirc_fd);
		mwlirc_fd = -1;
	}

	if (mwlirc_buf != NULL) {
		free(mwlirc_buf);
		mwlirc_buf = NULL;
	}

	mwlirc_buf_size = 0;
	mwlirc_buf_used = 0;
	mwlirc_buf_line_len = 0;
	mwlirc_packet_state = MWLIRC_EXPECT_BEGIN;
}


/**
 * Read a line.
 *
 * The line is returned in mwlirc_buf.
 *
 * @return 0 on success, else a MWLIRC_ERROR_* error code.
 */
static int
mwlirc_read_line(void)
{
	char *eol;
	int n;

	if (mwlirc_buf_line_len != 0) {
		mwlirc_buf_used -= mwlirc_buf_line_len;
		memmove(mwlirc_buf, mwlirc_buf + mwlirc_buf_line_len,
			mwlirc_buf_used + 1);
		mwlirc_buf_line_len = 0;
	}

	while (NULL == (eol = strchr(mwlirc_buf, '\n'))) {
		int space = mwlirc_buf_size - mwlirc_buf_used - 1;
		
		/*DPRINTF("mwlirc_read_line(): No newline in '%s'\n", mwlirc_buf);*/
		
		/*EPRINTF("mwlirc_read_line(): At top of loop\n");*/
		if (space < 30) {
			char *p =
				GdRealloc(mwlirc_buf, mwlirc_buf_size, mwlirc_buf_size * 2);
			if (p == NULL) {
				return MWLIRC_ERROR_MEMORY;
			}
			mwlirc_buf = p;
			mwlirc_buf_size = mwlirc_buf_size * 2;
		}
		do {
			/*DPRINTF("mwlirc_read_line(): Doing read()\n");*/
			n = read(mwlirc_fd,
				 mwlirc_buf + mwlirc_buf_used, space);
		} while ((n < 0) && (errno == EINTR));
		if (n < 0) {
			if (errno == EAGAIN) {
				/*DPRINTF("mwlirc_read_line(): Would block\n");*/
				return MWLIRC_ERROR_AGAIN;
			}
			EPRINTF("mwlirc_read_line(): Error in read\n");
			return MWLIRC_ERROR_SOCKET;
		}
		if (n == 0) {
			EPRINTF("mwlirc_read_line(): EOF\n");
			return MWLIRC_ERROR_SOCKET;
		}
		mwlirc_buf_used += n;
		mwlirc_buf[mwlirc_buf_used] = '\0';
		while (mwlirc_buf_used != (n = strlen(mwlirc_buf))) {
			EPRINTF("mwlirc_read_line(): Error: Embedded NUL in input string!  Skipping past it.  (Context: '%s' NUL '%s') (buf_used=%d != strlen()=%d)\n",
				mwlirc_buf, mwlirc_buf + n + 1, mwlirc_buf_used, n);
			mwlirc_buf_used -= (n + 1);
			memmove(mwlirc_buf, mwlirc_buf + n + 1,
				mwlirc_buf_used + 1);
		}
		/*DPRINTF("mwlirc_read_line(): Got '%s'\n", mwlirc_buf);*/
	}
	*eol = '\0';
	mwlirc_buf_line_len = eol - mwlirc_buf + 1;

	/*DPRINTF("mwlirc_read_line(): Returning string '%s'\n", mwlirc_buf);*/

	return 0;
}


/**
 * Get a line containing a keystroke.  Anything else is silently ignored.
 *
 * The line is returned in mwlirc_buf.
 *
 * @return 0 on success, else a MWLIRC_ERROR_* error code.
 */
static int
mwlirc_read_key(void)
{
	int err;

	for (;;) {
		/*DPRINTF("mwlirc_read_key(): At top of loop\n");*/
		err = mwlirc_read_line();
		/*DPRINTF("mwlirc_read_key(): Got line, err=%d\n", err);*/
		if (err) {
			return err;
		}
		switch (mwlirc_packet_state) {
		case MWLIRC_EXPECT_BEGIN:
			if (0 == strcmp(mwlirc_buf, "BEGIN")) {
				mwlirc_packet_state =
					MWLIRC_EXPECT_COMMAND;
				break;
			}
			/*DPRINTF("mwlirc_read_key(): Got line OK, returning it\n");*/
			return 0;

		case MWLIRC_EXPECT_COMMAND:
			/* Accept anything */
			/* Ignore actual command */
			mwlirc_packet_state = MWLIRC_EXPECT_SUCCESS_ERROR;
			break;

		case MWLIRC_EXPECT_SUCCESS_ERROR:
			if ((0 == strcmp(mwlirc_buf, "SUCCESS"))
			    || (0 == strcmp(mwlirc_buf, "ERROR"))) {
				mwlirc_packet_state = MWLIRC_EXPECT_DATA;
				break;
			}
			/* Fallthrough - SUCCESS/ERROR is optional */
		case MWLIRC_EXPECT_DATA:
			if (0 == strcmp(mwlirc_buf, "DATA")) {
				mwlirc_packet_state =
					MWLIRC_EXPECT_DATACOUNT;
				break;
			}
			/* Fallthrough - DATA is optional */
		case MWLIRC_EXPECT_END:
			if (0 == strcmp(mwlirc_buf, "END")) {
				mwlirc_packet_state = MWLIRC_EXPECT_BEGIN;
				break;
			}
			mwlirc_packet_state = MWLIRC_EXPECT_BEGIN;
			return MWLIRC_ERROR_PROTOCOL;

		case MWLIRC_EXPECT_DATACOUNT:
			mwlirc_packet_state = atoi(mwlirc_buf);
			if (mwlirc_packet_state <= 0) {
				mwlirc_packet_state = MWLIRC_EXPECT_BEGIN;
				return MWLIRC_ERROR_PROTOCOL;
			}
			break;
		default:
			assert(mwlirc_packet_state > 0);	/* must be a valid state! */
			assert(MWLIRC_EXPECT_END == 0);	/* else mwlirc_packet_state==1 fails! */
			/* Ignore actual data */
			mwlirc_packet_state--;
			break;
		}
	}
}

/**
 * Read a keystroke.
 *
 * @param key Used to store the keystroke.
 * @return 0 on success, else a MWLIRC_ERROR_* error code.
 */
int
mwlirc_read_keystroke(mwlirc_keystroke * key)
{
	char *repeatcountstr;
	char *keyname;
	char *remote;
	char *p;
	int repeatcount;

	int err = mwlirc_read_key();
	/*DPRINTF("mwlirc_read_keystroke(): got line, err=%d\n", err);*/
	if (err) {
		return err;
	}

	repeatcountstr = strchr(mwlirc_buf, ' ');
	if (NULL == repeatcountstr) {
		return MWLIRC_ERROR_PROTOCOL;
	}
	*repeatcountstr++ = '\0';

	keyname = strchr(repeatcountstr, ' ');
	if (NULL == keyname) {
		return MWLIRC_ERROR_PROTOCOL;
	}
	*keyname++ = '\0';

	remote = strchr(keyname, ' ');
	if (NULL == remote) {
		return MWLIRC_ERROR_PROTOCOL;
	}
	*remote++ = '\0';

	if ((*keyname == '\0')
	    || (*repeatcountstr == '\0')) {
		return MWLIRC_ERROR_PROTOCOL;
	}

	p = repeatcountstr;
	repeatcount = strtol(repeatcountstr, &p, 16);
	if (*p != '\0') {
		/* Couldn't convert entire string. */
		return MWLIRC_ERROR_PROTOCOL;
	}
	if (repeatcount < 0) {
		return MWLIRC_ERROR_PROTOCOL;
	}

	key->repeatcount = repeatcount;
	strncpy(key->name, keyname, MWLIRC_KEY_NAME_LENGTH - 1);
	strncpy(key->rc, remote, MWLIRC_RC_NAME_LENGTH - 1);
	key->name[MWLIRC_KEY_NAME_LENGTH - 1] = '\0';
	key->rc[MWLIRC_RC_NAME_LENGTH - 1] = '\0';

	/*DPRINTF("mwlirc_read_keystroke(): got key: remote=%s, key=%s, repeat=%d\n", remote, keyname, repeatcount);*/

	return 0;
}


/**
 * Read a keystroke.  Repeated keystrokes are ignored
 *
 * @param key Used to store the keystroke.
 * @return 0 on success, else a MWLIRC_ERROR_* error code.
 */
int
mwlirc_read_keystroke_norepeat(mwlirc_keystroke * key)
{
	do {
		int err = mwlirc_read_keystroke(key);
		if (err) {
			return err;
		}
	}
	while (key->repeatcount != 0);

	return 0;
}

