#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nano-X.h"
/*
 * setselection - set current selection contents
 *
 * tests GrSetSelectionOwner, GrSendClientData
 */

int main(int argc, char *argv[])
{
	GR_WINDOW_ID wid;
	GR_EVENT event;
	FILE *fp;
	char *buf = NULL;
	int buf_size = 0;
	int data_len = 0;
	int ret = 0;
	GR_EVENT_CLIENT_DATA_REQ *req;
	GR_EVENT_SELECTION_CHANGED *sc;

	if(argc != 2) {
		fprintf(stderr, "Usage: setselection <text file>\n");
		return 1;
	}

	if(!(fp = fopen(argv[1], "r"))) {
		fprintf(stderr, "Couldn't open text file\n");
		return 2;
	}

	do {
		data_len += ret;
		buf_size = data_len + 65536;
		if(!(buf = realloc(buf, buf_size))) {
			fprintf(stderr, "Out of memory\n");
			return 3;
		}
	} while((ret = fread(buf + data_len, 1, 65536, fp)) > 0);
	if(ret < 0) {
		fprintf(stderr, "Failed to read text file sucessfully\n");
		return 2;
	}

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server\n");
		return 4;
	}

	wid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, 1, 1, 0, 0, 0);
	if(!wid) {
		fprintf(stderr, "Couldn't get a window\n");
		return 5;
	}

	GrSelectEvents(wid, GR_EVENT_MASK_CLIENT_DATA_REQ | GR_EVENT_MASK_SELECTION_CHANGED);

	GrSetSelectionOwner(wid, "nota/realtype text/plain non/existent something/else");

	while(1) {
		GrGetNextEvent(&event);
		switch(event.type) {
			case GR_EVENT_TYPE_CLIENT_DATA_REQ:
				req = &event.clientdatareq;
				fprintf(stderr, "Got request with serial number %d from window %d for mime type %d\n",
					req->serial, req->rid, req->mimetype);
				GrSendClientData(wid, req->rid, req->serial, data_len, data_len, buf);
				break;
			case GR_EVENT_TYPE_SELECTION_CHANGED:
				sc = &event.selectionchanged;
				if(sc->new_owner != wid) {
					fprintf(stderr, "Lost selection to window %d\n", sc->new_owner);
					return 0;
				}
			default:
				fprintf(stderr, "event %d recieved\n", event.type);
				break;
		}
	}

	fprintf(stderr, "setselection: normal exit\n");
	GrClose();
	return 0;
}
