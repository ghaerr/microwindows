/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * sockets.c - functions to deal with sockets.
 */

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vncviewer.h>

/* fix bad MIPS sys headers...*/
#ifndef SOCK_STREAM
#define SOCK_STREAM	2	/* <asm/socket.h>*/
#endif

void PrintInHex(char *buf, int len);

Bool errorMessageFromReadExact = True;

/*
 * Read an exact number of bytes, and don't return until you've got them.
 */

Bool
ReadExact(int sock, char *buf, int n)
{
    int i = 0;
    int j;

    while (i < n) {
	j = read(sock, buf + i, (n - i));
	if (j <= 0) {
	    if (j < 0) {
		fprintf(stderr,programName);
		perror(": read");
	    } else {
		if (errorMessageFromReadExact) {
		    fprintf(stderr,"%s: read failed\n",programName);
		}
	    }
	    return False;
	}
	i += j;
    }
    if (debug)
	PrintInHex(buf,n);
    return True;
}


/*
 * Write an exact number of bytes, and don't return until you've sent them.
 */

Bool
WriteExact(int sock, char *buf, int n)
{
    int i = 0;
    int j;

    while (i < n) {
	j = write(sock, buf + i, (n - i));
	if (j <= 0) {
	    if (j < 0) {
		fprintf(stderr,programName);
		perror(": write");
	    } else {
		fprintf(stderr,"%s: write failed\n",programName);
	    }
	    return False;
	}
	i += j;
    }
    return True;
}


/*
 * ConnectToTcpAddr connects to the given TCP port.
 */

int
ConnectToTcpAddr(unsigned int host, int port)
{
    int sock;
    struct sockaddr_in addr;
    int one = 1;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = host;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	fprintf(stderr,programName);
	perror(": ConnectToTcpAddr: socket");
	return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	fprintf(stderr,programName);
	perror(": ConnectToTcpAddr: connect");
	close(sock);
	return -1;
    }

    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		   (char *)&one, sizeof(one)) < 0) {
	fprintf(stderr,programName);
	perror(": ConnectToTcpAddr: setsockopt");
	close(sock);
	return -1;
    }

    return sock;
}



/*
 * ListenAtTcpPort starts listening at the given TCP port.
 */

int
ListenAtTcpPort(int port)
{
    int sock;
    struct sockaddr_in addr;
    int one = 1;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	fprintf(stderr,programName);
	perror(": ListenAtTcpPort: socket");
	return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		   (const char *)&one, sizeof(one)) < 0) {
	fprintf(stderr,programName);
	perror(": ListenAtTcpPort: setsockopt");
	close(sock);
	return -1;
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	fprintf(stderr,programName);
	perror(": ListenAtTcpPort: bind");
	close(sock);
	return -1;
    }

    if (listen(sock, 5) < 0) {
	fprintf(stderr,programName);
	perror(": ListenAtTcpPort: listen");
	close(sock);
	return -1;
    }

    return sock;
}


/*
 * AcceptTcpConnection accepts a TCP connection.
 */

int
AcceptTcpConnection(int listenSock)
{
    int sock;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    int one = 1;

    sock = accept(listenSock, (struct sockaddr *) &addr, &addrlen);
    if (sock < 0) {
	fprintf(stderr,programName);
	perror(": AcceptTcpConnection: accept");
	return -1;
    }

    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		   (char *)&one, sizeof(one)) < 0) {
	fprintf(stderr,programName);
	perror(": AcceptTcpConnection: setsockopt");
	close(sock);
	return -1;
    }

    return sock;
}


/*
 * StringToIPAddr - convert a host string to an IP address.
 */

int
StringToIPAddr(const char *str, unsigned int *addr)
{
    struct hostent *hp;

    if ((*addr = inet_addr(str)) == -1)
    {
	if (!(hp = gethostbyname(str)))
	    return 0;

	*addr = *(unsigned int *)hp->h_addr;
    }

    return 1;
}


/*
 * Test if the other end of a socket is on the same machine.
 */

Bool
SameMachine(int sock)
{
    struct sockaddr_in peeraddr, myaddr;
    int addrlen = sizeof(struct sockaddr_in);

    getpeername(sock, (struct sockaddr *)&peeraddr, &addrlen);
    getsockname(sock, (struct sockaddr *)&myaddr, &addrlen);

    return (peeraddr.sin_addr.s_addr == myaddr.sin_addr.s_addr);
}


/*
 * Print out the contents of a packet for debugging.
 */

void
PrintInHex(char *buf, int len)
{
    int i, j;
    char c, str[17];

    str[16] = 0;

    fprintf(stderr,"ReadExact: ");

    for (i = 0; i < len; i++)
    {
	if ((i % 16 == 0) && (i != 0)) {
	    fprintf(stderr,"           ");
	}
	c = buf[i];
	str[i % 16] = (((c > 31) && (c < 127)) ? c : '.');
	fprintf(stderr,"%02x ",(unsigned char)c);
	if ((i % 4) == 3)
	    fprintf(stderr," ");
	if ((i % 16) == 15)
	{
	    fprintf(stderr,"%s\n",str);
	}
    }
    if ((i % 16) != 0)
    {
	for (j = i % 16; j < 16; j++)
	{
	    fprintf(stderr,"   ");
	    if ((j % 4) == 3) fprintf(stderr," ");
	}
	str[i % 16] = 0;
	fprintf(stderr,"%s\n",str);
    }

    fflush(stderr);
}
