#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "nano-X.h"

static int bytes_received = 0;
static char *data = NULL;

int got_client_data(GR_EVENT *event)
{
	GR_EVENT_CLIENT_DATA *ev = &event->clientdata;

	fprintf(stderr, "Got client data packet with serial number %ld for "
			"window %d from window %d\n", ev->serial, ev->wid,
								ev->rid);
	if(!(data = realloc(data, bytes_received + ev->datalen))) {
		fprintf(stderr, "Out of memory\n");
		exit(7);
	}
	memcpy(data + bytes_received, ev->data, ev->datalen);
	free(ev->data);

	fprintf(stderr, "Got client data packet with serial number %ld for "
			"window %d from window %d\n", ev->serial, ev->wid,
								ev->rid);
	fprintf(stderr, "Already received %d bytes, this packet is %ld bytes "
			"long, and the total data length is %ld bytes so ",
					bytes_received, ev->datalen, ev->len);

	bytes_received += ev->datalen;
	if(bytes_received == ev->len) {
		fprintf(stderr, "we have received all of the data now.\n");
		fprintf(stderr, "The data in the packet is:\n%s\n", data);
		return 1;
	}
	else if(bytes_received < ev->len) {
		fprintf(stderr, "this is not the last data packet.\n");
		return 0;
	} else fprintf(stderr, "we have received too much data (shouldn't "
								"happen)\n");

	return 1;
}

int main(int argc, char *argv[])
{
	GR_CHAR *typelist, *p;
	GR_WINDOW_ID sid, wid;
	GR_EVENT event;
	int n = 0, mimetype = -1;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 1;
	}

	sid = GrGetSelectionOwner(&typelist);
	if(!sid) {
		fprintf(stderr, "Clipboard is empty\n");
		return 2;
	}

	if(!typelist) {
		fprintf(stderr, "GrGetSelectionOwner() returned an empty "
				"type list for window %d\n", sid);
		return 3;
	}

	fprintf(stderr, "Window %d owns the selection\n", sid);
	fprintf(stderr, "It claims to be able to supply data in the following "
			"types:\n%s\n", typelist);

	p = strtok(typelist, " ");
	do {
		if(!strncmp("text/plain", p, 10)) {
			mimetype = n;
			break;
		}
		n++;
	} while((p = strtok(NULL, " ")));

	if(mimetype == -1) {
		fprintf(stderr, "Type text/plain is not available\n");
		return 4;
	}

	free(typelist);

	fprintf(stderr, "Type text/plain is available- requesting data\n");

	wid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, 1, 1, 0, 0, 0);
	if(!wid) {
		fprintf(stderr, "Couldn't get a window\n");
		return 5;
	}

	GrSelectEvents(wid, GR_EVENT_MASK_CLIENT_DATA);

	GrRequestClientData(wid, sid, 0, mimetype);

	while(1) {
		GrGetNextEventTimeout(&event, 4000);
		switch(event.type) {
			case GR_EVENT_TYPE_CLIENT_DATA:
				if(got_client_data(&event))
					return 0;
				break;
			case GR_EVENT_TYPE_TIMEOUT:
				fprintf(stderr, "Timed out waiting for data\n");
				return 6;
			default:
				break;
		}
	}

	return 0;
}
