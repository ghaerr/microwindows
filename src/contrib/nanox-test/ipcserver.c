/*
 * Nano-X IPC server demonstration- start one copy of ipccserver first, then
 * run as many ipcclients as you want.
 *
 * Copyright (c) 2002 Alex Holden <alex@alexholden.net>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "nano-X.h"

static void handle_packet(GR_WINDOW_ID wid, GR_EVENT *event)
{
	GR_EVENT_CLIENT_DATA *pkt = (GR_EVENT_CLIENT_DATA *)event;

	if(pkt->len == 4 && !memcmp(pkt->data, "ping", 4)) {
		printf("Got ping packet from window %d with serial %lu, "
				"sending pong reply\n", pkt->rid, pkt->serial);
		GrSendClientData(wid, pkt->rid, pkt->serial,
			4, 4, "pong");
	} else printf("Got unknown packet from window %d\n", pkt->rid);
}

int main(int argc, char *argv[])
{
	GR_EVENT event;
	GR_WINDOW_ID wid;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	wid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, 1, 1, 0, 0, 0);
	if(!wid) {
		fprintf(stderr, "Couldn't get a window\n");
		GrClose();
		return 1;
	}

	GrSelectEvents(wid, GR_EVENT_MASK_CLIENT_DATA);

	GrChangeProperty(GR_ROOT_WINDOW_ID, "demo_ipc_server", (GR_PROP *)&wid,
			sizeof(wid));

	printf("Registered server window %d\n", wid);

	while(1) {
		GrGetNextEvent(&event);
		switch(event.type) {
			case GR_EVENT_TYPE_CLIENT_DATA:
				handle_packet(wid, &event);
				break;
			default:
				fprintf(stderr, "Got unknown event %d\n",
						event.type);
				break;
		}
	}

	GrClose();

	return 0;
}
