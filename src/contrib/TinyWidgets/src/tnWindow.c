 /*C Source file for the Window widget
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
#include<string.h>
#include "../include/tnWidgets.h"
void
CreateWindow (TN_WIDGET *widget,
	      TN_WIDGET *parent,
	      int posx,
	      int posy,
	      char *caption, 
	      GR_SIZE height, 
	      GR_SIZE width, 
	      GR_COLOR bgcolor)	
{
  int i;
  GR_WM_PROPERTIES props;
  
  props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
  props.props = GR_WM_PROPS_APPWINDOW|GR_WM_PROPS_NOAUTOMOVE;
  props.title=caption?caption:"Window";
  if(height==0)
	  height=TN_WINDOW_HEIGHT;
  if(width==0)
	  width=TN_WINDOW_WIDTH;
  widget->wid=GrNewWindow(parent->wid,posx,posy,width,height,0,bgcolor,0);
  widget->WSpec.window.clickedx=-1;
  widget->WSpec.window.clickedy=-1;
  GrSetWMProperties (widget->wid, &props);/*Set the WM props*/
  for (i = 0; i < WINDOW_CALLBACKS; i++)
    widget->WSpec.window.CallBack[i].fp = NULL; /*Reset callback(s)*/
  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ 
		  | GR_EVENT_MASK_BUTTON_DOWN|GR_EVENT_MASK_KEY_DOWN);
  return;
}

void 
WindowEventHandler(GR_EVENT* event,TN_WIDGET *widget)
{
TN_WIDGET *child;
	switch(event->type)
	{
		case GR_EVENT_TYPE_BUTTON_DOWN:
				/*Set focus to Window on button click*/
				child=GetFromRegistry(event->button.subwid);
				if(child != NULL && child->type!=TN_CASCADEMENU)
					GrSetFocus(event->button.subwid);
				widget->WSpec.window.clickedx=event->button.x;
				widget->WSpec.window.clickedy=event->button.y;
				if (widget->WSpec.window.CallBack[CLICKED].fp)
				  (*(widget->WSpec.window.CallBack[CLICKED].fp)) (widget, widget->WSpec.window.CallBack[CLICKED].dptr);
			break;
		case GR_EVENT_TYPE_EXPOSURE:
			if(widget->visible==GR_FALSE)
				GrUnmapWindow(widget->wid);
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
			if (widget->WSpec.window.CallBack[CLOSED].fp)
				(*(widget->WSpec.window.CallBack[CLOSED].fp)) (widget, widget->WSpec.window.CallBack[CLOSED].dptr);
//			tnDestroyWidget(widget);
			break;
	}
	return;
}

void DestroyWindow(TN_WIDGET *widget)
{
	DeleteFromRegistry(widget);
	return;
}

void tnGetClickedPos(TN_WIDGET *widget,int *x,int *y)
{
	*x=widget->WSpec.window.clickedx;
	*y=widget->WSpec.window.clickedy;
	return;
}

int tnGetWindowTitle(TN_WIDGET *widget, char *caption)
{
	GR_WM_PROPERTIES props;
	if(widget->type!=TN_WINDOW)
		return -1;
	GrGetWMProperties(widget->wid,&props);
	strcpy(caption,props.title);
	return 1;
}
					
int tnSetWindowTitle(TN_WIDGET *widget,char *caption)
{
	GR_WM_PROPERTIES props;
	if(widget->type!=TN_WINDOW)
		return -1;
	GrGetWMProperties(widget->wid,&props);
	props.title=caption;
	GrSetWMProperties(widget->wid,&props);
	return 1;
}
