/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Nano-X Draw Library
 *	nxPaintNCArea - paint non-client area
 *	nxDraw3dShadow - draws a shadow with bottom-left and top-right missing
 *	nxDraw3dBox - draws a complete shadow
 *	nxDraw3dInset - draw a 2 line 3d inset
 *	nxDraw3dOutset - draw a 2 line 3d outset
 */
void		nxPaintNCArea(GR_DRAW_ID id, int w, int h, char *title,
			GR_BOOL active, GR_WM_PROPS props);

void		nxInflateRect(GR_RECT *prc, GR_SIZE dx, GR_SIZE dy);
void		nxDraw3dBox(GR_WINDOW_ID id,int x,int y,int w,int h,
			GR_COLOR crTop,GR_COLOR crBottom);
void		nxDraw3dInset(GR_DRAW_ID id,int x,int y,int w,int h);
void		nxDraw3dOutset(GR_WINDOW_ID id,int x,int y,int w,int h);
void		nxDraw3dPushDown(GR_DRAW_ID id, int x, int y, int w, int h);
void		nxDraw3dUpDownState(GR_DRAW_ID id, int x, int y, int w, int h,
			GR_BOOL fDown);
void		nxDraw3dUpFrame(GR_DRAW_ID id, int l, int t, int r, int b);
void		nxDraw3dShadow(GR_DRAW_ID id, int x, int y, int w, int h, GR_COLOR crTop, GR_COLOR crBottom);
