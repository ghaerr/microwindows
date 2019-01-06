/*
 * Nano-X IPC Client demonstration- start one copy of ipccserver first, then
 * run as many ipcclients as you want.
 *
 * Copyright (c) 2002 Alex Holden <alex@alexholden.net>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "nano-X.h"

static void send_ping(GR_WINDOW_ID wid, GR_WINDOW_ID serverid,
		GR_SERIALNO serial)
{
	printf("Sending ping from %d to %d with serial %lu\n", wid, serverid,
			serial);
	GrSendClientData(wid, serverid, serial, 4, 4, "ping");
}

static void handle_packet(GR_EVENT *event)
{
	GR_EVENT_CLIENT_DATA *pkt = (GR_EVENT_CLIENT_DATA *)event;

	if(pkt->len == 4 && !memcmp(pkt->data, "pong", 4)) {
		printf("Got pong from %d to %d with serial %lu\n", pkt->rid,
			pkt->wid, pkt->serial);
	} else printf("Got unknown packet from %d\n", pkt->rid);
}

int main(int argc, char *argv[])
{
	GR_EVENT event;
	GR_PROP *data;
	GR_SERIALNO count = 0;
	GR_WINDOW_ID wid, serverid;

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

	GrSelectEvents(wid, GR_EVENT_MASK_CLIENT_DATA | GR_EVENT_MASK_TIMER);

	if(GrGetWindowProperty(GR_ROOT_WINDOW_ID, "demo_ipc_server", &data)
			!= sizeof(serverid)) {
		if(!data) {
			fprintf(stderr, "Couldn't find server ID\n");
			GrClose();
			return 1;
		} else {
			fprintf(stderr, "Server ID property is wrong size\n");
			free(data);
			GrClose();
			return 1;
		}
	}

	memcpy(&serverid, data, sizeof(serverid));
	free(data);

	printf("Found server at window %d\n", serverid);

	GrCreateTimer(wid, 1000, GR_TRUE);

	while(1) {
		GrGetNextEvent(&event);
		switch(event.type) {
			case GR_EVENT_TYPE_TIMER:
				send_ping(wid, serverid, count++);
				break;
			case GR_EVENT_TYPE_CLIENT_DATA:
				handle_packet(&event);
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
