/*
 * Copyright (c) 1999-2001, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002, 2003 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 1999 Alex Holden <alex@linuxhacker.org>
 * Copyright (c) 2000 Vidar Hokstad
 * Copyright (c) 2000 Morten Rolland <mortenro@screenmedia.no>
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Completely rewritten for speed by Greg Haerr
 *
 * This is the server side of the network interface, which accepts
 * connections from clients, receives functions from them, and dispatches
 * events to them.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#if HAVE_SHAREDMEM_SUPPORT
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#if ELKS
#include <linuxmt/na.h>
#elif __ECOS
#include <netinet/in.h>

#else
#ifndef TRIMEDIA
#include <sys/un.h>
#endif
#endif
#include "serv.h"
#include "nxproto.h"

/* fix bad MIPS sys headers...*/
#ifndef SOCK_STREAM
#define SOCK_STREAM	2	/* <asm/socket.h>*/
#endif

extern	int		un_sock;
extern	GR_CLIENT	*root_client;
extern	int		current_fd;

static int GsWriteType(int,short);

/*
 * Wrapper functions called after full packet read
 */


#if (!MW_FEATURE_TIMERS) || (!MW_FEATURE_IMAGES) || (!defined(HAVE_FILEIO))
static void
GrNotImplementedWrapper(void *r)
{
    EPRINTF("nano-X: Function %s() not implemented\n", curfunc);
}
#endif

static void 
GrOpenWrapper(void *r)
{
	nxOpenReq *req = r;

	/* store process id of client*/
	curclient->processid = req->pid;

	GrOpen();
}

static void
GrCloseWrapper(void *r)
{
	GrClose();
}

static void
GrGetScreenInfoWrapper(void *r)
{
	GR_SCREEN_INFO si;

	GrGetScreenInfo(&si);
	GsWriteType(current_fd,GrNumGetScreenInfo);
	GsWrite(current_fd, &si, sizeof(si));
}

static void
GrGetSysColorWrapper(void *r)
{
	nxGetSysColorReq *req = r;
	GR_COLOR color = GrGetSysColor(req->index);

	GsWriteType(current_fd, GrNumGetSysColor);
	GsWrite(current_fd, &color, sizeof(color));
}

static void
GrNewWindowWrapper(void *r)
{
	nxNewWindowReq *req = r;
	GR_WINDOW_ID	wid;

	wid = GrNewWindow(req->parentid, req->x, req->y, req->width,
		req->height, req->bordersize, req->backgroundcolor,
		req->bordercolor);

	GsWriteType(current_fd,GrNumNewWindow);
	GsWrite(current_fd, &wid, sizeof(wid));
}


static void
GrNewPixmapWrapper(void *r)
{
	nxNewPixmapReq *req = r;
	GR_WINDOW_ID	wid;

        /* FIXME: Add support for passing info about shared memory
	 * segment
	 */
	wid = GrNewPixmap(req->width, req->height, 0);

	GsWriteType(current_fd,GrNumNewPixmap);
	GsWrite(current_fd, &wid, sizeof(wid));
}

static void
GrNewInputWindowWrapper(void *r)
{
	nxNewInputWindowReq *req = r;
	GR_WINDOW_ID         wid;

	wid = GrNewInputWindow(req->parentid, req->x, req->y, req->width,
		req->height);
	GsWriteType(current_fd,GrNumNewInputWindow);
	GsWrite(current_fd, &wid, sizeof(wid));
}

static void
GrDestroyWindowWrapper(void *r)
{
	nxDestroyWindowReq *req = r;

	GrDestroyWindow(req->windowid);
}

static void
GrNewGCWrapper(void *r)
{
	GR_GC_ID gc = GrNewGC();

	GsWriteType(current_fd,GrNumNewGC);
	GsWrite(current_fd, &gc, sizeof(gc));
}

static void
GrCopyGCWrapper(void *r)
{
	nxCopyGCReq *req = r;
	GR_GC_ID     gcnew;

	gcnew = GrCopyGC(req->gcid);
	GsWriteType(current_fd,GrNumCopyGC);
	GsWrite(current_fd, &gcnew, sizeof(gcnew));
}

static void
GrGetGCInfoWrapper(void *r)
{
	nxGetGCInfoReq *req = r;
	GR_GC_INFO      gcinfo;

	GrGetGCInfo(req->gcid, &gcinfo);
	GsWriteType(current_fd,GrNumGetGCInfo);
	GsWrite(current_fd, &gcinfo, sizeof(gcinfo));
}

static void
GrDestroyGCWrapper(void *r)
{
	nxDestroyGCReq *req = r;

	GrDestroyGC(req->gcid);
}

static void
GrNewRegionWrapper(void *r)
{
	GR_REGION_ID region = GrNewRegion();

	GsWriteType(current_fd, GrNumNewRegion);
	GsWrite(current_fd, &region, sizeof(region));
}

static void
GrDestroyRegionWrapper(void *r)
{
	nxDestroyRegionReq *req = r;

	GrDestroyRegion(req->regionid);
}

static void
GrIntersectRegionWrapper(void *r)
{
	nxIntersectRegionReq *req = r;

	GrIntersectRegion(req->regionid, req->srcregionid1,
		req->srcregionid2);
}

static void
GrUnionRectWithRegionWrapper(void *r)
{
	nxUnionRectWithRegionReq *req = r;

	GrUnionRectWithRegion(req->regionid, &(req->rect));
}

static void
GrUnionRegionWrapper(void *r)
{
	nxUnionRegionReq *req = r;

	GrUnionRegion(req->regionid, req->srcregionid1, req->srcregionid2);
}

static void
GrSubtractRegionWrapper(void *r)
{
	nxSubtractRegionReq *req = r;

	GrSubtractRegion(req->regionid, req->srcregionid1, req->srcregionid2);
}

static void
GrXorRegionWrapper(void *r)
{
	nxXorRegionReq *req = r;

	GrXorRegion(req->regionid, req->srcregionid1, req->srcregionid2);
}

static void
GrPointInRegionWrapper(void *r)
{
	nxPointInRegionReq *req = r;
	GR_BOOL ret_value = GrPointInRegion(req->regionid, req->x, req->y);
		
	GsWriteType(current_fd, GrNumPointInRegion);
	GsWrite(current_fd, &ret_value, sizeof(ret_value));
}

static void
GrRectInRegionWrapper(void *r)
{
	nxRectInRegionReq *req = r;
	unsigned short ret_value;
	
	ret_value = (unsigned short)GrRectInRegion(req->regionid,
		req->x, req->y, req->w, req->h);
		
	GsWriteType(current_fd, GrNumRectInRegion);
	GsWrite(current_fd, &ret_value, sizeof(ret_value));
}

static void
GrEmptyRegionWrapper(void *r)
{
	nxEmptyRegionReq *req = r;
	GR_BOOL		  ret_value;
	
	ret_value = GrEmptyRegion(req->regionid);
	GsWriteType(current_fd, GrNumEmptyRegion);
	GsWrite(current_fd, &ret_value, sizeof(ret_value));
}

static void
GrEqualRegionWrapper(void *r)
{
	nxEqualRegionReq *req = r;
	GR_BOOL		  ret_value;
	
	ret_value = GrEqualRegion(req->region1, req->region2);
	GsWriteType(current_fd, GrNumEqualRegion);
	GsWrite(current_fd, &ret_value, sizeof(ret_value));
}

static void
GrOffsetRegionWrapper(void *r)
{
	nxOffsetRegionReq *req = r;
	
	GrOffsetRegion(req->region, req->dx, req->dy);
}

static void
GrSetGCRegionWrapper(void *r)
{
	nxSetGCRegionReq *req = r;

	GrSetGCRegion(req->gcid, req->regionid);
}

static void
GrSetGCClipOriginWrapper(void *r)
{
	nxSetGCClipOriginReq *req = r;
	GrSetGCClipOrigin(req->gcid, req->xoff, req->yoff);
}

static void
GrSetGCGraphicsExposureWrapper(void *r)
{
	nxSetGCGraphicsExposureReq *req = r;
	GrSetGCGraphicsExposure(req->gcid, req->exposure);
}

static void
GrGetRegionBoxWrapper(void *r)
{
	nxRectInRegionReq *req = r;
	GR_BOOL		   ret_value;
	GR_RECT		   ret_rect;

	ret_value = GrGetRegionBox(req->regionid, &ret_rect);
		
	GsWriteType(current_fd, GrNumGetRegionBox);
	GsWrite(current_fd, &ret_rect, sizeof(ret_rect));
	GsWriteType(current_fd, GrNumGetRegionBox);
	GsWrite(current_fd, &ret_value, sizeof(ret_value));
}

static void
GrNewPolygonRegionWrapper(void *r)
{
	GR_REGION_ID region;
	nxNewPolygonRegionReq *req = r;
	int count;
	
	/* FIXME: unportable method, depends on sizeof(int) in GR_POINT*/
	count = GetReqVarLen(req) / sizeof(GR_POINT);	
	region = GrNewPolygonRegion(req->mode, count,
		(GR_POINT *)GetReqData(req));
	
	GsWriteType(current_fd, GrNumNewPolygonRegion);
	GsWrite(current_fd, &region, sizeof(region));
}

static void
GrMapWindowWrapper(void *r)
{
	nxMapWindowReq *req = r;

	GrMapWindow(req->windowid);
}

static void
GrUnmapWindowWrapper(void *r)
{
	nxUnmapWindowReq *req = r;

	GrUnmapWindow(req->windowid);
}

static void
GrRaiseWindowWrapper(void *r)
{
	nxRaiseWindowReq *req = r;

	GrRaiseWindow(req->windowid);
}

static void
GrLowerWindowWrapper(void *r)
{
	nxLowerWindowReq *req = r;

	GrLowerWindow(req->windowid);
}

static void
GrMoveWindowWrapper(void *r)
{
	nxMoveWindowReq *req = r;

	GrMoveWindow(req->windowid, req->x, req->y);
}

static void
GrResizeWindowWrapper(void *r)
{
	nxResizeWindowReq *req = r;

	GrResizeWindow(req->windowid, req->width, req->height);
}

static void
GrReparentWindowWrapper(void *r)
{
	nxReparentWindowReq *req = r;

	GrReparentWindow(req->windowid, req->parentid, req->x, req->y);
}

static void
GrGetWindowInfoWrapper(void *r)
{
	nxGetWindowInfoReq *req = r;
	GR_WINDOW_INFO      wi;

	GrGetWindowInfo(req->windowid, &wi);
	GsWriteType(current_fd,GrNumGetWindowInfo);
	GsWrite(current_fd, &wi, sizeof(wi));
}

static void
GrGetFontInfoWrapper(void *r)
{
	nxGetFontInfoReq *req = r;
	GR_FONT_INFO      fi;

	GrGetFontInfo(req->fontid, &fi);
	GsWriteType(current_fd,GrNumGetFontInfo);
	GsWrite(current_fd, &fi, sizeof(fi));
}

static void
GrGetFocusWrapper(void *r)
{
	GR_WINDOW_ID wid = GrGetFocus();

	GsWriteType(current_fd, GrNumGetFocus);
	GsWrite(current_fd, &wid, sizeof(wid));
}

static void
GrSetFocusWrapper(void *r)
{
	nxSetFocusReq *req = r;

	GrSetFocus(req->windowid);
}

static void
GrSetWindowCursorWrapper(void *r)
{
	nxSetWindowCursorReq *req = r;

	GrSetWindowCursor(req->windowid, req->cursorid);
}

static void
GrClearAreaWrapper(void *r)
{
	nxClearAreaReq *req = r;

	GrClearArea(req->windowid, req->x, req->y, req->width,
		req->height, req->exposeflag);
}

static void
GrSelectEventsWrapper(void *r)
{
	nxSelectEventsReq *req = r;

	GrSelectEvents(req->windowid, req->eventmask);
}

static void
GrGetNextEventWrapper(void *r)
{
#if 1
	/* tell main loop to call Finish routine on event*/
	curclient->waiting_for_event = TRUE;
#else
	GR_EVENT evt;

	/* first check if any event ready*/
	GrCheckNextEvent(&evt);
	if(evt.type == GR_EVENT_TYPE_NONE) {
		/* tell main loop to call Finish routine on event*/
		curclient->waiting_for_event = TRUE;
		return;
	}

	GsWriteType(current_fd, GrNumGetNextEvent);
	GsWrite(current_fd, &evt, sizeof(evt));
	if(evt.type == GR_EVENT_TYPE_CLIENT_DATA) {
		GsWrite(fd, evt.data, evt.datalen);
		free(evt.data);
	}
#endif
}

/* Complete the GrGetNextEvent call from client.
 * The client is still waiting on a read at this point.
 */
void
GrGetNextEventWrapperFinish(int fd)
{
	GR_EVENT evt;
	GR_EVENT_CLIENT_DATA *cde;

	/* get the event and pass it to client*/
	/* this will never be GR_EVENT_TYPE_NONE*/
	GrCheckNextEvent(&evt);

	GsWriteType(fd,GrNumGetNextEvent);
	GsWrite(fd, &evt, sizeof(evt));
	if(evt.type == GR_EVENT_TYPE_CLIENT_DATA) {
		cde = (GR_EVENT_CLIENT_DATA *)&evt;
		if(cde->data) {
			GsWrite(fd, cde->data, cde->datalen);
			free(cde->data);
		} cde->datalen = 0;
	}
}

static void
GrCheckNextEventWrapper(void *r)
{
	GR_EVENT evt;
	GR_EVENT_CLIENT_DATA *cde;

	GrCheckNextEvent(&evt);

	GsWriteType(current_fd,GrNumGetNextEvent);
	GsWrite(current_fd, &evt, sizeof(evt));
	if(evt.type == GR_EVENT_TYPE_CLIENT_DATA) {
		cde = (GR_EVENT_CLIENT_DATA *)&evt;
		if(cde->data) {
			GsWrite(current_fd, cde->data, cde->datalen);
			free(cde->data);
		} cde->datalen = 0;
	}
}

static void
GrPeekEventWrapper(void *r)
{
	GR_EVENT evt;
	GR_CHAR	 ret;
	GR_EVENT_CLIENT_DATA *cde;

	ret = GrPeekEvent(&evt);

	GsWriteType(current_fd,GrNumPeekEvent);
	GsWrite(current_fd, &evt, sizeof(evt));
	if(evt.type == GR_EVENT_TYPE_CLIENT_DATA) {
		cde = (GR_EVENT_CLIENT_DATA *)&evt;
		if(cde->data) {
			GsWrite(current_fd, cde->data, cde->datalen);
			free(cde->data);
		} cde->datalen = 0;
	}
	GsWrite(current_fd, &ret, sizeof(GR_CHAR));

	/* EXPERIMENTAL CODE for GTK+ select wait*/
	if (ret == 0) {
		/* tell main loop to call Finish routine on event*/
		curclient->waiting_for_event = TRUE;
	}
}

static void
GrLineWrapper(void *r)
{
	nxLineReq *req = r;

	GrLine(req->drawid, req->gcid, req->x1, req->y1, req->x2, req->y2);
}

static void
GrPointWrapper(void *r)
{
	nxPointReq *req = r;

	GrPoint(req->drawid, req->gcid, req->x, req->y);

}

static void
GrPointsWrapper(void *r)
{
	nxPointsReq *req = r;
	int        count;

	count = GetReqVarLen(req) / sizeof(GR_POINT);
	GrPoints(req->drawid, req->gcid, count, (GR_POINT *)GetReqData(req));
}

static void
GrRectWrapper(void *r)
{
	nxRectReq *req = r;

	GrRect(req->drawid, req->gcid, req->x, req->y, req->width, req->height);
}

static void
GrFillRectWrapper(void *r)
{
	nxFillRectReq *req = r;

	GrFillRect(req->drawid, req->gcid, req->x, req->y, req->width,
		req->height);
}

static void
GrPolyWrapper(void *r)
{
	nxPolyReq *req = r;
	int        count;

	count = GetReqVarLen(req) / sizeof(GR_POINT);
	GrPoly(req->drawid, req->gcid, count, (GR_POINT *)GetReqData(req));
}

/* FIXME: fails with pointtable size > 64k if sizeof(int) == 2*/
static void
GrFillPolyWrapper(void *r)
{
	nxPolyReq *req = r;
	int        count;

	count = GetReqVarLen(req) / sizeof(GR_POINT);
	GrFillPoly(req->drawid, req->gcid, count, (GR_POINT *)GetReqData(req));
}

static void
GrEllipseWrapper(void *r)
{
	nxEllipseReq *req = r;

	GrEllipse(req->drawid, req->gcid, req->x, req->y, req->rx, req->ry);
}

static void
GrFillEllipseWrapper(void *r)
{
	nxFillEllipseReq *req = r;

	GrFillEllipse(req->drawid, req->gcid, req->x, req->y, req->rx, req->ry);
}

static void
GrArcWrapper(void *r)
{
	nxArcReq *req = r;

	GrArc(req->drawid, req->gcid, req->x, req->y, req->rx, req->ry,
		req->ax, req->ay, req->bx, req->by, req->type);
}

static void
GrArcAngleWrapper(void *r)
{
	nxArcAngleReq *req = r;

	GrArcAngle(req->drawid, req->gcid, req->x, req->y, req->rx, req->ry,
		req->angle1, req->angle2, req->type);
}

static void
GrSetGCForegroundWrapper(void *r)
{
	nxSetGCForegroundReq *req = r;

	GrSetGCForeground(req->gcid, req->color);
}

static void
GrSetGCBackgroundWrapper(void *r)
{
	nxSetGCBackgroundReq *req = r;

	GrSetGCBackground(req->gcid, req->color);
}

static void
GrSetGCForegroundPixelValWrapper(void *r)
{
	nxSetGCForegroundPixelValReq *req = r;

	GrSetGCForegroundPixelVal(req->gcid, req->pixelval);
}

static void
GrSetGCBackgroundPixelValWrapper(void *r)
{
	nxSetGCBackgroundPixelValReq *req = r;

	GrSetGCBackgroundPixelVal(req->gcid, req->pixelval);
}

static void
GrSetGCUseBackgroundWrapper(void *r)
{
	nxSetGCUseBackgroundReq *req = r;

	GrSetGCUseBackground(req->gcid, req->flag);
}

static void
GrSetGCModeWrapper(void *r)
{
	nxSetGCModeReq *req = r;

	GrSetGCMode(req->gcid, req->mode);
}

static void
GrSetGCLineAttributesWrapper(void *r)
{
	nxSetGCLineAttributesReq *req = r;

	GrSetGCLineAttributes(req->gcid, req->linestyle);
}

static void
GrSetGCDashWrapper(void *r)
{
	nxSetGCDashReq *req = r;
	char *buffer = ALLOCA(req->count);

	memcpy((void *) buffer, GetReqData(req), req->count);
	GrSetGCDash(req->gcid, buffer, req->count);
	FREEA(buffer);
}


static void
GrSetGCFillModeWrapper(void *r)
{
	nxSetGCFillModeReq *req = r;

	GrSetGCFillMode(req->gcid, req->fillmode);
}

static void
GrSetGCStippleWrapper(void *r)
{
	nxSetGCStippleReq *req = r;
	GR_BITMAP *buffer;
	
	buffer = ALLOCA(GR_BITMAP_SIZE(req->width, req->height) *
		sizeof(GR_BITMAP));

	memcpy((void *) buffer, GetReqData(req),
	       GR_BITMAP_SIZE(req->width, req->height) * sizeof(GR_BITMAP));

	GrSetGCStipple(req->gcid, (GR_BITMAP *)buffer, req->width, req->height);
	FREEA(buffer);
}

static void
GrSetGCTileWrapper(void *r)
{
	nxSetGCTileReq *req = r;

	GrSetGCTile(req->gcid, req->pixmap, req->width, req->height);
}

static void
GrSetGCTSOffsetWrapper(void *r)
{
	nxSetGCTSOffsetReq *req = r;

	GrSetGCTSOffset(req->gcid, req->xoffset, req->yoffset);
}

static void
GrCreateFontWrapper(void *r)
{
	nxCreateFontReq *req = r;
	GR_FONT_ID 	fontid;

	fontid = GrCreateFont(GetReqData(req), req->height, NULL);

	GsWriteType(current_fd,GrNumCreateFont);
	GsWrite(current_fd, &fontid, sizeof(fontid));
}

static void
GrCreateLogFontWrapper(void *r)
{
	nxCreateLogFontReq *req = r;
	GR_FONT_ID fontid;

	fontid = GrCreateFont(NULL, 0, &req->lf);

	GsWriteType(current_fd, GrNumCreateLogFont);
	GsWrite(current_fd, &fontid, sizeof(fontid));
}

static void
GrSetFontSizeWrapper(void *r)
{
	nxSetFontSizeReq *req = r;

 	GrSetFontSize(req->fontid, req->fontsize);
}

static void
GrSetFontRotationWrapper(void *r)
{
	nxSetFontRotationReq *req = r;

 	GrSetFontRotation(req->fontid, req->tenthdegrees);
}

static void
GrSetFontAttrWrapper(void *r)
{
	nxSetFontAttrReq *req = r;

 	GrSetFontAttr(req->fontid, req->setflags, req->clrflags);
}

static void
GrDestroyFontWrapper(void *r)
{
	nxDestroyFontReq *req = r;

	GrDestroyFont(req->fontid);
}

static void
GrSetGCFontWrapper(void *r)
{
	nxSetGCFontReq *req = r;

	GrSetGCFont(req->gcid, req->fontid);
}

static void
GrGetGCTextSizeWrapper(void *r)
{
	nxGetGCTextSizeReq *req = r;
	GR_SIZE             retwidth, retheight, retbase;

	GrGetGCTextSize(req->gcid, GetReqData(req), req->charcount,
		req->flags, &retwidth, &retheight, &retbase);

 	GsWriteType(current_fd,GrNumGetGCTextSize);
	GsWrite(current_fd, &retwidth, sizeof(retwidth));
	GsWrite(current_fd, &retheight, sizeof(retheight));
	GsWrite(current_fd, &retbase, sizeof(retbase));
}

/* FIXME: fails with size > 64k if sizeof(int) == 2*/
static void
GrReadAreaWrapper(void *r)
{
	nxReadAreaReq *req = r;
	int            size;
	GR_PIXELVAL *   area;

	/* FIXME: optimize for smaller pixelvals*/
	size = req->width * req->height * sizeof(GR_PIXELVAL);

	if(!(area = malloc(size))) {
		/*GsPutCh(current_fd, GrRetENoMem);*/ /* FIXME*/
		/*return;*/
	}

	GrReadArea(req->drawid, req->x, req->y, req->width, req->height, area);
	GsWriteType(current_fd,GrNumReadArea);
	GsWrite(current_fd, area, size);
	free(area);
}

/* FIXME: fails with size > 64k if sizeof(int) == 2*/
static void
GrAreaWrapper(void *r)
{
	nxAreaReq *req = r;

	GrArea(req->drawid, req->gcid, req->x, req->y, req->width, req->height,
		GetReqData(req), req->pixtype);
}

/* FIXME: fails with bitmapsize > 64k if sizeof(int) == 2*/
static void
GrBitmapWrapper(void *r)
{
	nxBitmapReq *req = r;

	GrBitmap(req->drawid, req->gcid, req->x, req->y, req->width,
		req->height, GetReqData(req));
}

static void
GrDrawImageBitsWrapper(void *r)
{
	nxDrawImageBitsReq *req = r;
	char *		    addr;
	int		    imagesize;
	GR_IMAGE_HDR	    hdr;

	hdr.width = req->width;
	hdr.height = req->height;
	hdr.planes = req->planes;
	hdr.bpp = req->bpp;
	hdr.pitch = req->pitch;
	hdr.bytesperpixel = req->bytesperpixel;
	hdr.compression = req->compression;
	hdr.palsize = req->palsize;
	hdr.transcolor = req->transcolor;
	addr = GetReqData(req);
	hdr.imagebits = (MWUCHAR *)addr;
	imagesize = hdr.pitch * hdr.height;
	hdr.palette = (MWPALENTRY *)(addr + imagesize);
	GrDrawImageBits(req->drawid, req->gcid, req->x, req->y, &hdr);
}

static void
GrCopyAreaWrapper(void *r)
{
	nxCopyAreaReq *req = r;

	GrCopyArea(req->drawid, req->gcid, req->x, req->y, req->width, req->height,
		req->srcid, req->srcx, req->srcy, req->op);
}

static void
GrTextWrapper(void *r)
{
	nxTextReq *req = r;

	GrText(req->drawid, req->gcid, req->x, req->y, GetReqData(req),
		req->count, req->flags);
}

static void
GrNewCursorWrapper(void *r)
{
	nxNewCursorReq *req = r;
	int		varlen;
	char *          bitmap;
	GR_CURSOR_ID	cursorid;

	varlen = GetReqVarLen(req);
	bitmap = GetReqData(req);

	cursorid = GrNewCursor(req->width, req->height,
		req->hotx, req->hoty, req->fgcolor, req->bgcolor,
		(GR_BITMAP *)bitmap, (GR_BITMAP *)(bitmap+varlen/2));

	GsWriteType(current_fd, GrNumNewCursor);
	GsWrite(current_fd, &cursorid, sizeof(cursorid));
}

static void
GrMoveCursorWrapper(void *r)
{
	nxMoveCursorReq *req = r;

	GrMoveCursor(req->x, req->y);
}

static void
GrGetSystemPaletteWrapper(void *r)
{
	GR_PALETTE             pal;

	GrGetSystemPalette(&pal);
	GsWriteType(current_fd,GrNumGetSystemPalette);
	GsWrite(current_fd, &pal, sizeof(pal));
}

static void
GrSetSystemPaletteWrapper(void *r)
{
	nxSetSystemPaletteReq *req = r;
	GR_PALETTE	       pal;

	pal.count = req->count;
	memcpy(pal.palette, req->palette, sizeof(pal.palette));
	GrSetSystemPalette(req->first, &pal);
}

static void
GrFindColorWrapper(void *r)
{
	nxFindColorReq *req = r;
	GR_PIXELVAL	pixel;

	GrFindColor(req->color, &pixel);
	GsWriteType(current_fd,GrNumFindColor);
	GsWrite(current_fd, &pixel, sizeof(pixel));
}

static void
GrInjectEventWrapper(void *r)
{
	nxInjectEventReq *req = r;

	switch(req->event_type) {
	case GR_INJECT_EVENT_POINTER:
		GrInjectPointerEvent(req->event.pointer.x,
				     req->event.pointer.y,
				     req->event.pointer.button,
				     req->event.pointer.visible);
		break;

	case GR_INJECT_EVENT_KEYBOARD:
		GrInjectKeyboardEvent(req->event.keyboard.wid,
				      req->event.keyboard.keyvalue,
				      req->event.keyboard.modifier,
				      req->event.keyboard.scancode,
				      req->event.keyboard.pressed);
		break;
	}
}

static void
GrSetWMPropertiesWrapper(void *r)
{
	nxSetWMPropertiesReq *req = r;
	GR_WM_PROPERTIES *props = GetReqData(req);

	if(GetReqVarLen(req) - sizeof(GR_WM_PROPERTIES)) 
		props->title = (GR_CHAR *)props + sizeof(GR_WM_PROPERTIES);
	else
		props->title = NULL;
	GrSetWMProperties(req->windowid, props);
}

static void
GrGetWMPropertiesWrapper(void *r)
{
	nxGetWMPropertiesReq *req = r;
	UINT16 textlen;
	GR_WM_PROPERTIES props;
	
	GrGetWMProperties(req->windowid, &props);

	if(props.title)
		textlen = strlen((const char *)props.title) + 1;
	else textlen = 0;

	GsWriteType(current_fd,GrNumGetWMProperties);
	GsWrite(current_fd, &props, sizeof(props));
	GsWrite(current_fd, &textlen, sizeof(textlen));
	if(textlen)
		GsWrite(current_fd, props.title, textlen);
}

static void
GrCloseWindowWrapper(void *r)
{
	nxCloseWindowReq *req = r;

	GrCloseWindow(req->windowid);
}

static void
GrKillWindowWrapper(void *r)
{
	nxKillWindowReq *req = r;

	GrKillWindow(req->windowid);
}

#if MW_FEATURE_IMAGES && defined(HAVE_FILEIO)
static void
GrDrawImageFromFileWrapper(void *r)
{
	nxDrawImageFromFileReq *req = r;

	GrDrawImageFromFile(req->drawid, req->gcid, req->x, req->y, req->width,
		req->height, GetReqData(req), req->flags);
}

static void
GrLoadImageFromFileWrapper(void *r)
{
	nxLoadImageFromFileReq *req = r;
	GR_IMAGE_ID		id;

	id = GrLoadImageFromFile(GetReqData(req), req->flags);
	GsWriteType(current_fd, GrNumLoadImageFromFile);
	GsWrite(current_fd, &id, sizeof(id));
}
#else /* if ! (MW_FEATURE_IMAGES && defined(HAVE_FILEIO)) */
#define GrDrawImageFromFileWrapper GrNotImplementedWrapper
#define GrLoadImageFromFileWrapper GrNotImplementedWrapper
#endif

#if MW_FEATURE_IMAGES
static void
GrDrawImageToFitWrapper(void *r)
{
	nxDrawImageToFitReq *req = r;

	GrDrawImageToFit(req->drawid, req->gcid, req->x, req->y, req->width,
		req->height, req->imageid);
}

static void
GrFreeImageWrapper(void *r)
{
	nxFreeImageReq *req = r;

	GrFreeImage(req->id);
}

static void
GrGetImageInfoWrapper(void *r)
{
	nxGetImageInfoReq *req = r;
	GR_IMAGE_INFO	   info;

	GrGetImageInfo(req->id, &info);
	GsWriteType(current_fd, GrNumGetImageInfo);
	GsWrite(current_fd, &info, sizeof(info));
}
#else /* if ! MW_FEATURE_IMAGES */
#define GrDrawImageToFitWrapper GrNotImplementedWrapper
#define GrFreeImageWrapper GrNotImplementedWrapper
#define GrGetImageInfoWrapper GrNotImplementedWrapper
#endif

static void
GrSetScreenSaverTimeoutWrapper(void *r)
{
	nxSetScreenSaverTimeoutReq *req = r;

	GrSetScreenSaverTimeout(req->timeout);
}

static void
GrSetSelectionOwnerWrapper(void *r)
{
	nxSetSelectionOwnerReq *req = r;

	GrSetSelectionOwner(req->wid, GetReqData(req));
}

static void
GrGetSelectionOwnerWrapper(void *r)
{
	GR_CHAR *typelist;
	GR_WINDOW_ID wid;
	unsigned short len;

	wid = GrGetSelectionOwner(&typelist);
	GsWriteType(current_fd, GrNumGetSelectionOwner);
	GsWrite(current_fd, &wid, sizeof(wid));

	if(wid) {
		len = strlen((const char *)typelist) + 1;
		GsWrite(current_fd, &len, sizeof(len));
		GsWrite(current_fd, typelist, len);
	}
}

static void
GrRequestClientDataWrapper(void *r)
{
	nxRequestClientDataReq *req = r;

	GrRequestClientData(req->wid, req->rid, req->serial, req->mimetype);
}

static void
GrSendClientDataWrapper(void *r)
{
	nxSendClientDataReq *req = r;

	GrSendClientData(req->wid, req->did, req->serial, req->len,
		GetReqVarLen(req), GetReqData(req));
}

static void
GrBellWrapper(void *r)
{
	GrBell();
}

static void
GrSetBackgroundPixmapWrapper(void *r)
{
	nxSetBackgroundPixmapReq *req = r;

	GrSetBackgroundPixmap(req->wid, req->pixmap, req->flags);
}

static void
GrDestroyCursorWrapper(void *r)
{
	nxDestroyCursorReq *req = r;

	GrDestroyCursor(req->cursorid);
}

static void
GrQueryTreeWrapper(void *r)
{
	nxQueryTreeReq *req = r;
	GR_WINDOW_ID	parentid;
	GR_WINDOW_ID *	children;
	GR_COUNT 	nchildren;

	GrQueryTree(req->windowid, &parentid, &children, &nchildren);

	GsWriteType(current_fd, GrNumQueryTree);
	GsWrite(current_fd, &parentid, sizeof(parentid));
	GsWrite(current_fd, &nchildren, sizeof(nchildren));
	if (nchildren) {
		GsWrite(current_fd, children, nchildren * sizeof(GR_WINDOW_ID));
		free(children);
	}
}

#if MW_FEATURE_TIMERS
static void
GrCreateTimerWrapper(void *r)
{
    nxCreateTimerReq *req = r;
    GR_TIMER_ID  timerid;

    timerid = GrCreateTimer(req->wid, req->period);

    GsWriteType(current_fd, GrNumCreateTimer);
    GsWrite(current_fd, &timerid, sizeof (timerid));
}
#else /* if ! MW_FEATURE_TIMERS */
#define GrCreateTimerWrapper GrNotImplementedWrapper
#endif

typedef struct image_list {
 void *data;
 int id;
 int size;
 int offset;

 struct image_list *next;
} imagelist_t;

static imagelist_t *imageListHead = 0;
static imagelist_t *imageListTail = 0;
static int imageListId = 0;

static imagelist_t *findImageBuffer(int buffer_id)
{
	imagelist_t *buffer = 0;

	for(buffer = imageListHead; buffer; buffer = buffer->next)
		if (buffer->id == buffer_id)
			break;

	return buffer;
}

static void freeImageBuffer(imagelist_t *buffer)
{
 imagelist_t *prev = 0;
 imagelist_t *ptr = imageListHead;

 while(ptr) {
   if (ptr == buffer) {
     if (prev) 
	prev->next = buffer->next;
     else 
	imageListHead = buffer->next;
     
     if (!imageListHead) imageListTail = 0;
     
     free(buffer->data);
     free(buffer);
     return;
   }
   
   prev = ptr;
   ptr = ptr->next;
 }  
}

static void
GrImageBufferAllocWrapper(void *r)
{
 nxImageBufferAllocReq *req = r;
 
 /* Add a new buffer to the end of the list */

 if (!imageListTail) {
   imageListHead = imageListTail = (imagelist_t *) malloc(sizeof(imagelist_t));
 }
 else {
   imageListTail->next = (imagelist_t *) malloc(sizeof(imagelist_t));
   imageListTail = imageListTail->next;
 }

 imageListTail->id = ++imageListId;
 imageListTail->data = (void *) malloc(req->size);
 imageListTail->size = req->size;
 imageListTail->offset = 0;

 imageListTail->next = 0;

 GsWriteType(current_fd,GrNumImageBufferAlloc);
 GsWrite(current_fd, &imageListTail->id, sizeof(int));
}

static void
GrImageBufferSendWrapper(void *r)
{
 int csize;
 imagelist_t *buffer;
 nxImageBufferSendReq *req = r;

 buffer = findImageBuffer(req->buffer_id);

 if (!buffer) return;

 if (buffer->offset + req->size >= buffer->size) 
   csize = buffer->size - buffer->offset;
 else
   csize = req->size;

 if (!csize) return;

 memcpy((void *) (((char *)buffer->data) + buffer->offset), 
	 GetReqData(req), csize);
 
 buffer->offset += csize;
}

#if MW_FEATURE_IMAGES
static void
GrLoadImageFromBufferWrapper(void *r)
{
 GR_IMAGE_ID		id;
 imagelist_t *buffer;
 nxLoadImageFromBufferReq *req = r;
 
 buffer = findImageBuffer(req->buffer);
 
 if (!buffer) return;

 id = GrLoadImageFromBuffer(buffer->data, buffer->size, req->flags);

 GsWriteType(current_fd, GrNumLoadImageFromBuffer);
 GsWrite(current_fd, &id, sizeof(id));

 freeImageBuffer(buffer);
}

static void
GrDrawImageFromBufferWrapper(void *r)
{
 imagelist_t *buffer;
 nxDrawImageFromBufferReq *req = r;
 
 buffer = findImageBuffer(req->buffer);
 
 if (!buffer) return;

 GrDrawImageFromBuffer(req->drawid, req->gcid, req->x, req->y, req->width, 
			req->height, buffer->data, buffer->size, 
			req->flags);
 
 freeImageBuffer(buffer);
}
#else /* if ! MW_FEATURE_IMAGES */
#define GrLoadImageFromBufferWrapper GrNotImplementedWrapper
#define GrDrawImageFromBufferWrapper GrNotImplementedWrapper
#endif


#if MW_FEATURE_TIMERS
static void
GrDestroyTimerWrapper(void *r)
{
    nxDestroyTimerReq *req = r;
    
    GrDestroyTimer(req->timerid);
}
#else /* if ! MW_FEATURE_TIMERS */
#define GrDestroyTimerWrapper GrNotImplementedWrapper
#endif

static void
GrSetPortraitModeWrapper(void *r)
{
    nxSetPortraitModeReq *req = r;
    
    GrSetPortraitMode(req->portraitmode);
}

static void
GrQueryPointerWrapper(void *r)
{
	GR_WINDOW_ID	mwin;
	GR_COORD	x, y;
	GR_BUTTON	bmask;

	GrQueryPointer(&mwin, &x, &y, &bmask);

	GsWriteType(current_fd, GrNumQueryPointer);

	GsWrite(current_fd, &mwin, sizeof(mwin));
	GsWrite(current_fd, &x, sizeof(x));
	GsWrite(current_fd, &y, sizeof(y));
	GsWrite(current_fd, &bmask, sizeof(bmask));
}

/*
 * This function makes the Nano-X server set up a shared memory segment
 * that the client can use when feeding the Nano-X server with requests.
 * There is a corresponding GrShmCmdsFlush function that will make the
 * server execute the batched commands.
 */
#define SHMKEY_BASE 1000000
#define SHMKEY_MAX 256

static void
GrReqShmCmdsWrapper(void *r)
{
#if HAVE_SHAREDMEM_SUPPORT
	nxReqShmCmdsReq *req = r;
	int 		key, shmid;
	char 		*tmp;

	if ( curclient->shm_cmds != 0 )
		goto bad;

	for ( key=SHMKEY_BASE; key < SHMKEY_BASE+SHMKEY_MAX; key++ ) {
		shmid = shmget(key,req->size,IPC_CREAT|IPC_EXCL|0666);
		if ( shmid == -1 ) {
			if ( errno != EEXIST )
				goto bad;
		} else {
			tmp = shmat(shmid,0,0);
			if ( tmp == (char *)-1 )
				goto bad;
			curclient->shm_cmds = tmp;
			curclient->shm_cmds_shmid = shmid;
			curclient->shm_cmds_size = req->size;
			goto finish;
		}
	}

 bad:
	key = 0;

 finish:
	DPRINTF("Shm: Request key granted=%d\n",key);
	GsWrite(current_fd, &key, sizeof(key));
#else
	/* return no shared memory support*/
	int key = 0;
	GsWrite(current_fd, &key, sizeof(key));
#endif /* HAVE_SHAREDMEM_SUPPORT*/
}

static void 
GrGetFontListWrapper(void *r)
{
	MWFONTLIST **list;
	int num;
	int i, ttlen, mwlen;

	GrGetFontList(&list, &num);

	GsWriteType(current_fd, GrNumGetFontList);

	/* the number of strings comming in */
	GsWrite(current_fd, &num, sizeof(int));

	if(num != -1) {
		for(i = 0; i < num; i++) {
			ttlen = strlen(list[i]->ttname) + 1;
			mwlen = strlen(list[i]->mwname) + 1;

			GsWrite(current_fd, &ttlen, sizeof(int));
			GsWrite(current_fd, list[i]->ttname, ttlen * sizeof(char));

			GsWrite(current_fd, &mwlen, sizeof(int));
			GsWrite(current_fd, list[i]->mwname, mwlen * sizeof(char));
		}
		
		GrFreeFontList(&list, num);
	}
}

static void
GrNewBitmapRegionWrapper(void *r)
{
	GR_REGION_ID region;
	nxNewBitmapRegionReq *req = r;

	region = GrNewBitmapRegion(GetReqData(req), req->width, req->height);

	GsWriteType(current_fd, GrNumNewBitmapRegion);
	GsWrite(current_fd, &region, sizeof(region));
}

static void
GrSetWindowRegionWrapper(void *r)
{
	nxSetWindowRegionReq *req = r;

	GrSetWindowRegion(req->wid, req->rid, req->type);
}
 
static void
GrStretchAreaWrapper(void *r)
{
	nxStretchAreaReq *req = r;

	GrStretchArea(req->drawid, req->gcid,
		      req->dx1, req->dy1,
		      req->dx2, req->dy2,
		      req->srcid,
		      req->sx1, req->sy1,
		      req->sx2, req->sy2,
		      req->op);
}

/* handle both GrGrabKey and GrUngrabKey*/
static void
GrGrabKeyWrapper(void *r)
{
        nxGrabKeyReq *req = r;

	if (req->type != GR_GRAB_MAX + 1) {   /* GrGrabKey */
		int ret = GrGrabKey(req->wid, req->key, req->type);
		GsWriteType(current_fd, GrNumGrabKey);
		GsWrite(current_fd, &ret, sizeof(ret));
	} else
		GrUngrabKey(req->wid, req->key);
}

static void
GrSetTransformWrapper(void *r) 
{
	nxSetTransformReq *req = r;
	GR_TRANSFORM trans;

	if (req->mode) {
		trans.a = req->trans_a;
		trans.b = req->trans_b;
		trans.c = req->trans_c;
		trans.d = req->trans_d;
		trans.e = req->trans_e;
		trans.f = req->trans_f;
		trans.s = req->trans_s;
		GrSetTransform(&trans);
	} else
		GrSetTransform(NULL);
}

static void
GrCreateFontFromBufferWrapper(void *r)
{
#if HAVE_FREETYPE_2_SUPPORT
	imagelist_t *buffer;
	nxCreateFontFromBufferReq *req = r;
	GR_FONT_ID result;
	
	buffer = findImageBuffer(req->buffer_id);
	if (!buffer) {
		result = 0;
	} else {
		result = GrCreateFontFromBuffer(buffer->data, buffer->size,
			(const char *)req->format, req->height);
		
		freeImageBuffer(buffer);
	}
	
	GsWriteType(current_fd, GrNumCreateFontFromBuffer);
	GsWrite(current_fd, &result, sizeof(result));
#endif /*HAVE_FREETYPE_2_SUPPORT*/
}

static void
GrCopyFontWrapper(void *r)
{
#if HAVE_FREETYPE_2_SUPPORT
	nxCopyFontReq *req = r;
	GR_FONT_ID result = GrCopyFont(req->fontid, req->height);

	GsWriteType(current_fd, GrNumCopyFont);
	GsWrite(current_fd, &result, sizeof(result));
#endif /*HAVE_FREETYPE_2_SUPPORT*/
}


void GrShmCmdsFlushWrapper(void *r);

/*
 * Handler functions, ordered by reqType
 */
struct GrFunction {
	void		(*func)(void *);
	GR_FUNC_NAME 	name;
} GrFunctions[] = {
	/*   0 */ {GrOpenWrapper, "GrOpen"},
	/*   1 */ {GrCloseWrapper, "GrClose"},
	/*   2 */ {GrGetScreenInfoWrapper, "GrGetScreenInfo"},
	/*   3 */ {GrNewWindowWrapper, "GrNewWindow"},
	/*   4 */ {GrNewInputWindowWrapper, "GrNewInputWindow"},
	/*   5 */ {GrDestroyWindowWrapper, "GrDestroyWindow"},
	/*   6 */ {GrNewGCWrapper, "GrNewGC"},
	/*   7 */ {GrCopyGCWrapper, "GrCopyGC"},
	/*   8 */ {GrGetGCInfoWrapper, "GrGetGCInfo"},
	/*   9 */ {GrDestroyGCWrapper, "GrDestroyGC"},
	/*  10 */ {GrMapWindowWrapper, "GrMapWindow"},
	/*  11 */ {GrUnmapWindowWrapper, "GrUnmapWindow"},
	/*  12 */ {GrRaiseWindowWrapper, "GrRaiseWindow"},
	/*  13 */ {GrLowerWindowWrapper, "GrLowerWindow"},
	/*  14 */ {GrMoveWindowWrapper, "GrMoveWindow"},
	/*  15 */ {GrResizeWindowWrapper, "GrResizeWindow"},
	/*  16 */ {GrGetWindowInfoWrapper, "GrGetWindowInfo"},
	/*  17 */ {GrGetFontInfoWrapper, "GrGetFontInfo"},
	/*  18 */ {GrSetFocusWrapper, "GrSetFocus"},
	/*  19 */ {GrSetWindowCursorWrapper, "GrSetWindowCursor"},
	/*  20 */ {GrClearAreaWrapper, "GrClearAreaWrapper"},
	/*  21 */ {GrSelectEventsWrapper, "GrSelectEvents"},
	/*  22 */ {GrGetNextEventWrapper, "GrGetNextEvent"},
	/*  23 */ {GrCheckNextEventWrapper, "GrCheckNextEvent"},
	/*  24 */ {GrPeekEventWrapper, "GrPeekEvent"},
	/*  25 */ {GrLineWrapper, "GrLine"},
	/*  26 */ {GrPointWrapper, "GrPoint"},
	/*  27 */ {GrRectWrapper, "GrRect"},
	/*  28 */ {GrFillRectWrapper, "GrFillRect"},
	/*  29 */ {GrPolyWrapper, "GrPoly"},
	/*  30 */ {GrFillPolyWrapper, "GrFillPoly"},
	/*  31 */ {GrEllipseWrapper, "GrEllipse"},
	/*  32 */ {GrFillEllipseWrapper, "GrFillEllipse"},
	/*  33 */ {GrSetGCForegroundWrapper, "GrSetGCForeground"},
	/*  34 */ {GrSetGCBackgroundWrapper, "GrSetGCBackGround"},
	/*  35 */ {GrSetGCUseBackgroundWrapper, "GrSetGCUseBackGround"},
	/*  36 */ {GrSetGCModeWrapper, "GrSetGCMode"},
	/*  37 */ {GrSetGCFontWrapper, "GrSetGCFont"},
	/*  38 */ {GrGetGCTextSizeWrapper, "GrGetGCTextSize"},
	/*  39 */ {GrReadAreaWrapper, "GrReadArea"},
	/*  40 */ {GrAreaWrapper, "GrArea"},
	/*  41 */ {GrBitmapWrapper, "GrBitmap"},
	/*  42 */ {GrTextWrapper, "GrText"},
	/*  43 */ {GrNewCursorWrapper, "GrNewCursor"},
	/*  44 */ {GrMoveCursorWrapper, "GrMoveCursor"},
	/*  45 */ {GrGetSystemPaletteWrapper, "GrGetSystemPalette"},
	/*  46 */ {GrFindColorWrapper, "GrFindColor"},
	/*  47 */ {GrReparentWindowWrapper, "GrReparentWindow"},
	/*  48 */ {GrDrawImageFromFileWrapper, "GrDrawImageFromFile"},
	/*  49 */ {GrLoadImageFromFileWrapper, "GrLoadImageFromFile"},
	/*  50 */ {GrNewPixmapWrapper, "GrNewPixmap"},
	/*  51 */ {GrCopyAreaWrapper, "GrCopyArea"},
	/*  52 */ {GrSetFontSizeWrapper, "GrSetFontSize"},
	/*  53 */ {GrCreateFontWrapper, "GrCreateFont"},
	/*  54 */ {GrDestroyFontWrapper, "GrDestroyFont"},
	/*  55 */ {GrReqShmCmdsWrapper, "GrReqShmCmds"},
	/*  56 */ {GrShmCmdsFlushWrapper, "GrShmCmdsFlush"},
	/*  57 */ {GrSetFontRotationWrapper, "GrSetFontRotation"},
	/*  58 */ {GrSetFontAttrWrapper, "GrSetFontAttr"},
	/*  59 */ {GrSetSystemPaletteWrapper, "GrSetSystemPalette"},
	/*  60 */ {GrInjectEventWrapper, "GrInjectEvent"},
	/*  61 */ {GrNewRegionWrapper, "GrNewRegion"},
	/*  62 */ {GrDestroyRegionWrapper, "GrDestroyRegion"},
	/*  63 */ {GrUnionRectWithRegionWrapper, "GrUnionRectWithRegion"},
	/*  64 */ {GrUnionRegionWrapper, "GrUnionRegion"},
	/*  65 */ {GrIntersectRegionWrapper, "GrIntersectRegion"},
	/*  66 */ {GrSetGCRegionWrapper, "GrSetGCRegion"},
	/*  67 */ {GrSubtractRegionWrapper, "GrSubtractRegion"},
	/*  68 */ {GrXorRegionWrapper, "GrXorRegion"},
	/*  69 */ {GrPointInRegionWrapper, "GrPointInRegion"},
	/*  70 */ {GrRectInRegionWrapper, "GrRectInRegion"},	
	/*  71 */ {GrEmptyRegionWrapper, "GrEmptyRegion"},	
	/*  72 */ {GrEqualRegionWrapper, "GrEqualRegion"},	
	/*  73 */ {GrOffsetRegionWrapper, "GrOffsetRegion"},	
	/*  74 */ {GrGetRegionBoxWrapper, "GrGetRegionBox"},	
	/*  75 */ {GrNewPolygonRegionWrapper, "GrNewPolygonRegion"},	
	/*  76 */ {GrArcWrapper, "GrArc"},
	/*  77 */ {GrArcAngleWrapper, "GrArcAngle"},
	/*  78 */ {GrSetWMPropertiesWrapper, "GrSetWMProperties"},
	/*  79 */ {GrGetWMPropertiesWrapper, "GrGetWMProperties"},
	/*  80 */ {GrCloseWindowWrapper, "GrCloseWindow"},
	/*  81 */ {GrKillWindowWrapper, "GrKillWindow"},
	/*  82 */ {GrDrawImageToFitWrapper, "GrDrawImageToFit"},
	/*  83 */ {GrFreeImageWrapper, "GrFreeImage"},
	/*  84 */ {GrGetImageInfoWrapper, "GrGetImageInfo"},
	/*  85 */ {GrDrawImageBitsWrapper, "GrDrawImageBits"},
 	/*  86 */ {GrPointsWrapper, "GrPoints"},
 	/*  87 */ {GrGetFocusWrapper, "GrGetFocus"},
 	/*  88 */ {GrGetSysColorWrapper, "GrGetSysColor"},
	/*  89 */ {GrSetScreenSaverTimeoutWrapper, "GrSetScreenSaverTimeout"},
	/*  90 */ {GrSetSelectionOwnerWrapper, "GrSetSelectionOwner"},
	/*  91 */ {GrGetSelectionOwnerWrapper, "GrGetSelectionOwner"},
	/*  92 */ {GrRequestClientDataWrapper, "GrRequestClientData"},
	/*  93 */ {GrSendClientDataWrapper, "GrSendClientData"},
	/*  94 */ {GrBellWrapper, "GrBell"},
	/*  95 */ {GrSetBackgroundPixmapWrapper, "GrSetBackgroundPixmap"},
	/*  96 */ {GrDestroyCursorWrapper, "GrDestroyCursor"},
	/*  97 */ {GrQueryTreeWrapper, "GrQueryTree"},
	/*  98 */ {GrCreateTimerWrapper, "GrCreateTimer"},
	/*  99 */ {GrDestroyTimerWrapper, "GrDestroyTimer"},
	/* 100 */ {GrSetPortraitModeWrapper, "GrSetPortraitMode"},
	/* 101 */ {GrImageBufferAllocWrapper, "GrImageBufferAlloc"},
	/* 102 */ {GrImageBufferSendWrapper, "GrImageBufferSend"},
	/* 103 */ {GrLoadImageFromBufferWrapper, "GrLoadImageFromBuffer"},
	/* 104 */ {GrDrawImageFromBufferWrapper, "GrDrawImageFromBuffer"},
	/* 105 */ {GrGetFontListWrapper, "GrGetFontList"},
	/* 106 */ {GrSetGCClipOriginWrapper, "GrSetGCClipOrigin"},
	/* 107 */ {GrSetGCGraphicsExposureWrapper, "GrSetGCGraphicsExposure"},
	/* 108 */ {GrQueryPointerWrapper, "GrQueryPointer"},
	/* 109 */ {GrSetGCLineAttributesWrapper, "GretGCLineAttributes"},
	/* 110 */ {GrSetGCDashWrapper, "GrSetGCDash"},
	/* 111 */ {GrSetGCFillModeWrapper, "GrSetGCFillMode"},
	/* 112 */ {GrSetGCStippleWrapper, "GrSetGCStipple"},
	/* 113 */ {GrSetGCTSOffsetWrapper, "GrSetGCTSOffset"},
	/* 114 */ {GrSetGCTileWrapper, "GrSetGCTile" },
	/* 115 */ {GrNewBitmapRegionWrapper, "GrNewBitmapRegion"},
	/* 116 */ {GrSetWindowRegionWrapper, "GrSetWindowRegion"},
	/* 117 */ {GrSetGCForegroundPixelValWrapper, "GrSetGCForegroundPixelVal"},
	/* 118 */ {GrSetGCBackgroundPixelValWrapper, "GrSetGCBackgroundPixelVal"},
	/* 119 */ {GrCreateLogFontWrapper, "GrCreateLogFont"},
	/* 120 */ {GrStretchAreaWrapper, "GrStretchArea"},
	/* 121 */ {GrGrabKeyWrapper, "GrGrabKey" },
	/* 122 */ {GrSetTransformWrapper, "GrSetTransform" },
	/* 123 */ {GrCreateFontFromBufferWrapper, "GrCreateFontFromBuffer"},
	/* 124 */ {GrCopyFontWrapper, "GrCopyFont"},
};

void
GrShmCmdsFlushWrapper(void *r)
{
	nxShmCmdsFlushReq *req = r;
	unsigned char 	reply;
#if HAVE_SHAREDMEM_SUPPORT
	nxReq 		*pr;
	int 		length;
	unsigned char 	*do_req, *do_req_last;

	if ( current_shm_cmds == 0 || current_shm_cmds_size < req->size ) {
		/* No or short shm present serverside, bug or mischief */
		EPRINTF("nano-X: Ill behaved client assumes shm ok\n");
		if ( req->reply ) {
			reply = 0;
			GsWrite(current_fd, &reply, 1);
		}
		return;
	}

	do_req = current_shm_cmds;
	do_req_last = current_shm_cmds + req->size;

	while ( do_req < do_req_last ) {
		pr = (nxReq *)do_req;
		length = GetReqAlignedLen(pr);
		if ( pr->reqType < GrTotalNumCalls ) {
			GrFunctions[pr->reqType].func(pr);
		} else {
			EPRINTF("nano-X: Error bad shm function!\n");
		}
		do_req += length;
	}

	if ( req->reply ) {
		reply = 1;
		GsWrite(current_fd, &reply, 1);
	}
#else
	/* no shared memory support*/
	if ( req->reply ) {
		reply = 0;
		GsWrite(current_fd, &reply, 1);
	}
#endif /* HAVE_SHAREDMEM_SUPPORT*/
}

/*
 * This function is used to bind to the named socket which is used to
 * accept connections from the clients.
 */
int 
GsOpenSocket(void)
{
#if ELKS
	struct sockaddr_na sckt;
#ifndef SUN_LEN
#define SUN_LEN(ptr)	(sizeof(sckt))
#endif
#elif __ECOS
	struct sockaddr_in sckt;
#ifndef SUN_LEN
#define SUN_LEN(ptr)	(sizeof(sckt))
#endif
#else
	struct sockaddr_un sckt;
#ifndef SUN_LEN
#define SUN_LEN(ptr)	((size_t) (((struct sockaddr_un *) 0)->sun_path) \
		      		+ strlen ((ptr)->sun_path))
#endif
#endif /* ELKS */

#if ELKS
	if((un_sock = socket(AF_NANO, SOCK_STREAM, 0)) == -1)
		return -1;

	sckt.sun_family = AF_NANO;
	sckt.sun_no = GR_NUMB_SOCKET;
#elif __ECOS
	/* Create the socket */
	if((un_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	    return -1;

	/* Bind to any/all local IP addresses */
	memset( &sckt, '\0', sizeof(sckt) );
	sckt.sin_family = AF_INET;
	sckt.sin_len = sizeof(sckt);
	sckt.sin_port = htons(6600);
	sckt.sin_addr.s_addr = INADDR_ANY;
#else
	if (access(GR_NAMED_SOCKET, F_OK) == 0) {
		/* FIXME: should try connecting to see if server is active */
		if(unlink(GR_NAMED_SOCKET))
			return -1;
	}

	/* Create the socket: */
	if((un_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	/* Bind a name to the socket: */
	sckt.sun_family = AF_UNIX;
	strncpy(sckt.sun_path, GR_NAMED_SOCKET, sizeof(sckt.sun_path));
#endif /* ELKS */
	if(bind(un_sock, (struct sockaddr *) &sckt, SUN_LEN(&sckt)) < 0)
		return -1;

	/* Start listening on the socket: */
	if(listen(un_sock, 5) == -1)
		return -1;
	return 1;
}

void
GsCloseSocket(void)
{
	if(un_sock != -1)
		close(un_sock);
	un_sock = -1;
	unlink(GR_NAMED_SOCKET);
}

/*
 * This function is used to accept a connnection from a client.
 */
void
GsAcceptClient(void)
{
	int i;
#if ELKS
	struct sockaddr_na sckt;
#elif __ECOS
	struct sockaddr_in sckt;
#else
	struct sockaddr_un sckt;
#endif
	socklen_t size = sizeof(sckt);

	if((i = accept(un_sock, (struct sockaddr *) &sckt, &size)) == -1) {
		EPRINTF("nano-X: Error accept failed (%d)\n", errno);
		return;
	}
	GsAcceptClientFd(i);
}

/*
 * This function accepts a client ID, and searches through the
 * linked list of clients, returning a pointer to the relevant
 * structure or NULL if not found.
 */
GR_CLIENT *
GsFindClient(int fd)
{
	GR_CLIENT *client;

	client = root_client;

	while(client) {
		if(client->id == fd)
			return(client);
		client = client->next;
	}
	
	return 0;
}

/*
 * Destroy windows and eventclient structures used by client.
 * Called by GsDropClient after a client has exited to clean
 * up resources.
 */
void
GsDestroyClientResources(GR_CLIENT * client)
{
	GR_WINDOW     * wp, *nwp;
	GR_PIXMAP     * pp, *npp;
	GR_GC 	      * gp, *ngp;
	GR_REGION     * rp, *nrp;
	GR_FONT       * fp, *nfp;
	GR_CURSOR     *	cp, *ncp;
	GR_EVENT_CLIENT *ecp, *necp;
	GR_EVENT_CLIENT *pecp = NULL;
	GR_EVENT_LIST	*evp;
	GR_GRABBED_KEY	*kp, *nkp;
#if MW_FEATURE_IMAGES
	GR_IMAGE      * ip, *nip;
#endif
#if MW_FEATURE_TIMERS
	GR_TIMER      * tp, *ntp;
#endif

DPRINTF("Destroy client %d resources\n", client->id);
	/* search window list, destroy windows owned by client*/
	for(wp=listwp; wp; wp=nwp) {
		nwp = wp->next;
		/*
		 * Remove eventclient structures for this client
		 */
		ecp = wp->eventclients;
		while (ecp) {
			necp = ecp->next;
			if (ecp->client == client) {
DPRINTF( "  Destroy window %d eventclient mask %08lx\n", wp->id, ecp->eventmask);
				if (ecp == wp->eventclients)
					wp->eventclients = ecp->next;
				else
					pecp->next = ecp->next;
				free(ecp);
			} else
				pecp = ecp;
			ecp = necp;
		}
		if (wp->owner == client) {
DPRINTF("  Destroy window %d\n", wp->id);
			GrDestroyWindow(wp->id);
		}
	}

	/* search pixmap list, destroy pixmaps owned by client*/
	for(pp=listpp; pp; pp=npp) {
		npp = pp->next;
		if (pp->owner == client) {
DPRINTF("  Destroy pixmap %d\n", pp->id);
			GrDestroyWindow(pp->id);
		}
	}

	/* free gc's owned by client*/
	for(gp=listgcp; gp; gp=ngp) {
		ngp = gp->next;
		if (gp->owner == client) {
DPRINTF("  Destroy gc %d\n", gp->id);
			GrDestroyGC(gp->id);
		}
	}

	/* free fonts owned by client*/
	for(fp=listfontp; fp; fp=nfp) {
		nfp = fp->next;
		if (fp->owner == client) {
DPRINTF("  Destroy font %d\n", fp->id);
			GrDestroyFont(fp->id);
		}
	}

	/* free regions owned by client*/
	for(rp=listregionp; rp; rp=nrp) {
		nrp = rp->next;
		if (rp->owner == client) {
DPRINTF("  Destroy region %d\n", rp->id);
			GrDestroyRegion(rp->id);
		}
	}

#if MW_FEATURE_IMAGES
	/* free images owned by client*/
	for(ip=listimagep; ip; ip=nip) {
		nip = ip->next;
		if (ip->owner == client) {
DPRINTF("  Destroy image %d\n", ip->id);
			GrFreeImage(ip->id);
		}
	}
#endif

#if MW_FEATURE_TIMERS
	/* free timers owned by client*/
	for(tp=list_timer; tp; tp=ntp) {
		ntp = tp->next;
		if (tp->owner == client) {
DPRINTF("  Destroy timer %d\n", tp->id);
			GrDestroyTimer(tp->id);
		}
	}
#endif

	/* free cursors owned by client*/
	for(cp=listcursorp; cp; cp=ncp) {
		ncp = cp->next;
		if (cp->owner == client) {
DPRINTF("  Destroy cursor %d\n", cp->id);
			GrDestroyCursor(cp->id);
		}
	}

	/* Free key grabs associated with client*/
	for (kp=list_grabbed_keys; kp; kp = nkp) {
		nkp = kp->next;
		if (kp->owner == curclient) {
DPRINTF("  Destroy grabkey %d,%d\n", kp->wid, kp->key);
			GrUngrabKey(kp->wid, kp->key);
		}
	}

	/* Free events associated with client*/
	evp = client->eventhead;
	while (evp) {
DPRINTF("  Destroy event %d\n", evp->event.type);
		client->eventhead = evp->next;
		evp->next = eventfree;
		eventfree = evp;
		evp = client->eventhead;
	}
}

/*
 * Display window, pixmap, gc, font, region lists
 */
static void
GsPrintResources(void)
{
	GR_WINDOW *wp;
	GR_PIXMAP *pp;
	GR_GC *gp;
	GR_REGION *rp;
	GR_FONT *fp;
#if MW_FEATURE_IMAGES
	GR_IMAGE *ip;
#endif
#if MW_FEATURE_TIMERS
	GR_TIMER *tp;
#endif

	/* window list*/
	DPRINTF("Window list:\n");
	for(wp=listwp; wp; wp=wp->next) {
		DPRINTF("%d(%d),", wp->id, wp->owner? wp->owner->id: 0);
	}
	DPRINTF("\nPixmap list:\n");
	for(pp=listpp; pp; pp=pp->next) {
		DPRINTF("%d(%d),", pp->id, pp->owner->id);
	}
	DPRINTF("\nGC list:\n");
	for(gp=listgcp; gp; gp=gp->next) {
		DPRINTF("%d(%d),", gp->id, gp->owner->id);
	}
	DPRINTF("\nFont list:\n");
	for(fp=listfontp; fp; fp=fp->next) {
		DPRINTF("%d(%d),", fp->id, fp->owner->id);
	}
	DPRINTF("\nRegion list:\n");
	for(rp=listregionp; rp; rp=rp->next) {
		DPRINTF("%d(%d),", rp->id, rp->owner->id);
	}
#if MW_FEATURE_IMAGES
	DPRINTF("\nImage list:\n");
	for(ip=listimagep; ip; ip=ip->next) {
		DPRINTF("%d(%d),", ip->id, ip->owner->id);
	}
#endif
#if MW_FEATURE_TIMERS
	DPRINTF("\nTimer list:\n");
	for(tp=list_timer; tp; tp=tp->next) {
		DPRINTF("%d(%d),", tp->id, tp->owner->id);
	}
#endif
	DPRINTF("\n");
}

/*
 * This is used to drop a client when it is detected that the connection to it
 * has been lost.
 */
void
GsDropClient(int fd)
{
	GR_CLIENT *client;

	if((client = GsFindClient(fd))) { /* If it exists */
		close(fd);	/* Close the socket */

		GsDestroyClientResources(client);
		if(client == root_client)
			root_client = client->next;
		/* Link the prev to the next */
		if(client->prev) client->prev->next = client->next;

		/* Link the next to the prev */
		if(client->next) client->next->prev = client->prev;

#if HAVE_SHAREDMEM_SUPPORT
		if ( client->shm_cmds != 0 ) {
			/* Free shared memory */
			shmctl(client->shm_cmds_shmid,IPC_RMID,0);
			shmdt(client->shm_cmds);
		}
#endif
GsPrintResources();
		free(client);	/* Free the structure */

		clipwp = NULL;	/* reset clip window*/
		--connectcount;
	} else EPRINTF("nano-X: trying to drop non-existent client %d.\n", fd);
}

/*
 * This is a wrapper to read() which handles error conditions, and
 * returns 0 for both error conditions and no data.
 */
int
#if ELKS
GsRead(int fd, char *buf, int c)
#else
GsRead(int fd, void *buf, int c)
#endif
{
	int e, n;

	n = 0;

	while(n < c) {
		e = read(fd, ((char *)buf) + n, c - n);
		if(e <= 0) {
			if (e == 0)
				EPRINTF("nano-X: client closed socket: %d\n", fd);
			else EPRINTF("nano-X: GsRead failed %d %d: %d\r\n",
			       e, n, errno);
			GsClose(fd);
			return -1;
		}
		n += e;
	}

	return 0;
}

/*
 * This is a wrapper to write().
 */
int GsWrite(int fd, void *buf, int c)
{
	int e, n;

	n = 0;

	while(n < c) {
		e = write(fd, ((char *) buf + n), (c - n));
		if(e <= 0) {
			GsClose(fd);
			return -1;
		}
		n += e;
	}

	return 0;
}

int GsWriteType(int fd, short type)
{
	return GsWrite(fd,&type,sizeof(type));
}

/*
 * This function is used to parse and dispatch requests from the clients.
 * Note that the maximum request size is allocated from the stack
 * in this function.
 */
void
GsHandleClient(int fd)
{
	nxReq *	req;
	long	len;
	char	buf[MAXREQUESTSZ];

	current_fd = fd;
#if HAVE_SHAREDMEM_SUPPORT
	current_shm_cmds = curclient->shm_cmds;
	current_shm_cmds_size = curclient->shm_cmds_size;
#endif
	/* read request header*/
	if(GsRead(fd, buf, sizeof(nxReq)))
		return;

	len = GetReqAlignedLen((nxReq *)&buf[0]);
	if(len > sizeof(nxReq)) {
		if(len > MAXREQUESTSZ) {
			EPRINTF("nano-X: GsHandleClient request too large: %ld > %d\n",
				len, MAXREQUESTSZ);
			exit(1);
		}
		/* read additional request data*/
		if(GsRead(fd, &buf[sizeof(nxReq)], len-sizeof(nxReq)))
			return;
	}
	req = (nxReq *)&buf[0];

	if(req->reqType < GrTotalNumCalls) {
		curfunc = GrFunctions[req->reqType].name;
		/*DPRINTF("HandleClient %s\n", curfunc);*/
		GrFunctions[req->reqType].func(req);
	} else {
		EPRINTF("nano-X: GsHandleClient bad function\n");
	}
}
