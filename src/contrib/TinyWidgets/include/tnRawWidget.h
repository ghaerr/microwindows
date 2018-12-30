 /*Header file for the Raw widget
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

#ifndef _TNRAWWIDGET_H_
#define _TNRAWWIDGET_H_

/*
 * These are the events that have been taken upward from the nano-X architecture
 * and the user is able to register callbacks for them in the raw widget
 */  

#define TN_SYSTEM_EVENT_EXPOSURE 		0
#define TN_SYSTEM_EVENT_BUTTON_UP		1
#define TN_SYSTEM_EVENT_BUTTON_DOWN		2
#define TN_SYSTEM_EVENT_MOUSE_ENTER		3
#define TN_SYSTEM_EVENT_MOUSE_EXIT		4
#define TN_SYSTEM_EVENT_MOUSE_MOTION		5
#define TN_SYSTEM_EVENT_MOUSE_POSITION 		6
#define TN_SYSTEM_EVENT_KEY_DOWN		7
#define TN_SYSTEM_EVENT_KEY_UP			8
#define TN_SYSTEM_EVENT_FOCUS_IN		9
#define TN_SYSTEM_EVENT_FOCUS_OUT		10
#define TN_SYSTEM_EVENT_UPDATE			11
#define TN_SYSTEM_EVENT_CHILD_UPDATE		12
#define TN_SYSTEM_EVENT_CLOSE_REQ		13
#define TN_SYSTEM_EVENT_TIMER			14
#define RAWWIDGET_CALLBACKS			15	/* +1 from line above*/

#include "tnBase.h"
#define TN_RAWWIDGET_HEIGHT 30
#define TN_RAWWIDGET_WIDTH 60

typedef struct
{
  CallBackStruct CallBack[RAWWIDGET_CALLBACKS];	
  GR_EVENT *lastevent;	  
}
TN_STRUCT_RAWWIDGET;

void
CreateRawWidget (TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      GR_SIZE, GR_SIZE,char *, GR_SIZE, GR_COLOR, GR_COLOR);
void RawWidgetEventHandler (GR_EVENT *, TN_WIDGET *);
void DestroyRawWidget(TN_WIDGET *);
void InvokeCallBack(TN_WIDGET *,int);
GR_EVENT *tnGetLastEvent(TN_WIDGET *);
#endif /*_TNRAWWIDGET_H_*/
