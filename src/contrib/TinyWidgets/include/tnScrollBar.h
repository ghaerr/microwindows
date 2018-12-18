 /*
 * Header file for the ScrollBar widget
 * This file is part of `TinyWidgets', a widget set for the nano-X GUI which is  * a part of the Microwindows project (www.microwindows.org).
 * Copyright C 2000
 * Sunil Soman <sunil_soman@vsnl.com>
 * Amit Kulkarni <amms@vsnl.net>
 * Navin Thadani <navs@vsnl.net>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation; either version 2.1 of the License, 
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _TNSCROLLBAR_H_
#define _TNSCROLLBAR_H_
#define SCROLLBAR_CALLBACKS 1
#define SCROLLBAR_TIMEOUT 50

#define TN_SCROLLBAR_PAGESTEP 5
#define TN_SCROLLBAR_LINESTEP 1
#define TN_SCROLLBAR_MINVAL 0
#define TN_SCROLLBAR_MAXVAL 100
#define TN_SCROLLBAR_THUMBSIZE 10
#define TN_SCROLLBAR_VERTICAL_HEIGHT 160
#define TN_SCROLLBAR_VERTICAL_WIDTH 20
#define TN_SCROLLBAR_HORIZONTAL_HEIGHT 20
#define TN_SCROLLBAR_HORIZONTAL_WIDTH 160
#define TN_SCROLLBAR_COMPONENT_SIZE 20
#define TN_VERTICAL 0
#define TN_HORIZONTAL 1

#define TN_SCROLL_NOSCROLL	0
#define TN_SCROLL_LINEUP 	1
#define TN_SCROLL_LINEDOWN 	2
#define TN_SCROLL_PAGEUP 	3
#define TN_SCROLL_PAGEDOWN	4
#define TN_SCROLL_THUMBMOVE	5

#define TN_MIN_THUMBSIZE 	5

#include "tnBase.h"

typedef struct
{
  int orientation;
  GR_SIZE thumbsize;
  int minval;
  int maxval;
  int pagestep;
  int linestep;
  int thumbpos;
  int LastScrollEvent;
  GR_WINDOW_ID upleft;
  GR_WINDOW_ID downright;
  GR_WINDOW_ID thumb; 
  GR_BOOL st_upleft_down;	
  GR_BOOL st_downright_down;
  GR_BOOL st_thumb_down;	 
  GR_BOOL st_pageup;
  GR_TIMER_ID tid;
  
  CallBackStruct CallBack[SCROLLBAR_CALLBACKS];	/*ScrollBar CLICKED Callbacks*/
}
TN_STRUCT_SCROLLBAR;

void
CreateScrollBar (TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      GR_SIZE, GR_SIZE,int,int,int,int,int);
void ScrollBarEventHandler (GR_EVENT *, TN_WIDGET *);
void DrawScrollBar (TN_WIDGET *);
void DrawUpLeft(TN_WIDGET * );
void DrawDownRight(TN_WIDGET *);
void DrawThumb(TN_WIDGET *);
int tnGetLastScrollEvent(TN_WIDGET *);
int tnGetThumbPosition(TN_WIDGET *);
int tnSetScrollRange(TN_WIDGET *,int,int);
int tnSetThumbPosition(TN_WIDGET *,int);
void DestroyScrollBar(TN_WIDGET *);
int tnGetScrollBarOrientation(TN_WIDGET*, int*);
int tnGetScrollRange(TN_WIDGET *,int *,int *);
int tnGetScrollStepSizes(TN_WIDGET *,int *,int *);
int tnSetScrollStepSizes(TN_WIDGET *,int ,int );
int tnSetScrollBarOrientation(TN_WIDGET*,int);

#endif /*_TNSCROLLBAR_H_*/
