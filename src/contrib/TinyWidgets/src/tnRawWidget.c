 /*C Source file for the Raw widget
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

#include "../include/tnWidgets.h"
#include<string.h>
/*Create Raw widget*/
void
CreateRawWidget (
		 TN_WIDGET * widget,
	      	 TN_WIDGET * parent,
	      	 int posx,
	      	 int posy,
	      	 GR_SIZE height,
	      	 GR_SIZE width,
	         char *fontname,
	     	 GR_SIZE fontsize, 
		 GR_COLOR bgcolor, 
		 GR_COLOR fgcolor
		 )
{
  int i;
  GR_FONT_ID font ;
  if(strcmp(fontname,""))
	  font= GrCreateFont (fontname, fontsize, NULL);
  else
	  font= TN_DEFAULT_FONT_NO;

  GrSetGCFont (widget->gc, font);
  if (height == 0) 
      height = TN_RAWWIDGET_HEIGHT;
  if(width==0)
      width = TN_RAWWIDGET_WIDTH;
  
  widget->WSpec.button.FGColor = fgcolor;

  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, GR_RGB(255,255,255), 0);

  for (i = 0; i < RAWWIDGET_CALLBACKS; i++)
    widget->WSpec.rawwidget.CallBack[i].fp = NULL;
  widget->WSpec.rawwidget.lastevent=NULL;
  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_MOUSE_EXIT |
		  GR_EVENT_MASK_MOUSE_ENTER |
		  GR_EVENT_MASK_MOUSE_POSITION |
		  GR_EVENT_MASK_MOUSE_MOTION |
		  GR_EVENT_MASK_BUTTON_UP | 
		  GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_KEY_DOWN |
		  GR_EVENT_MASK_KEY_UP |
		  GR_EVENT_MASK_FOCUS_IN |
		  GR_EVENT_MASK_FOCUS_OUT |
		  GR_EVENT_MASK_UPDATE |
		  GR_EVENT_MASK_CHLD_UPDATE |
		  GR_EVENT_MASK_CLOSE_REQ |
		  GR_EVENT_MASK_TIMER |
		  GR_EVENT_MASK_EXPOSURE 
		  );
  return;
}

/*Event Handler for the Raw Widget*/
void
RawWidgetEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
	widget->WSpec.rawwidget.lastevent=event;
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_DOWN:	
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_BUTTON_DOWN);
	    break;
    case GR_EVENT_TYPE_BUTTON_UP:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_BUTTON_UP);
	    break;
    case GR_EVENT_TYPE_EXPOSURE:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_EXPOSURE);
	    break;
    case GR_EVENT_TYPE_MOUSE_EXIT:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_MOUSE_EXIT);
       	    break;
    case GR_EVENT_TYPE_MOUSE_ENTER: 
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_MOUSE_ENTER);
	    break;
    case GR_EVENT_TYPE_MOUSE_POSITION:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_MOUSE_POSITION);
	    break;
    case GR_EVENT_TYPE_MOUSE_MOTION:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_MOUSE_MOTION);
	    break;
    case GR_EVENT_TYPE_KEY_DOWN:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_KEY_DOWN);
	    break;
    case GR_EVENT_TYPE_KEY_UP:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_KEY_UP);
	    break;
    case GR_EVENT_TYPE_FOCUS_IN:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_FOCUS_IN);
	    break;
    case GR_EVENT_TYPE_FOCUS_OUT:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_FOCUS_OUT);
	    break;
    case GR_EVENT_TYPE_UPDATE:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_UPDATE);
	    break;
    case GR_EVENT_TYPE_CHLD_UPDATE:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_CHILD_UPDATE);
	    break;
    case GR_EVENT_TYPE_TIMER:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_TIMER);
		break;
    case GR_EVENT_TYPE_CLOSE_REQ:
	    InvokeCallBack(widget,TN_SYSTEM_EVENT_CLOSE_REQ);
	    break;
    }
}

void InvokeCallBack(TN_WIDGET *widget,int callback_event)
{
	CallBackStruct *cb = &widget->WSpec.rawwidget.CallBack[callback_event];
	/* Call callback function if it exists with widget and data_pointer params*/
	if (cb->fp)
		cb->fp(widget, cb->dptr);
	return;
}

void DestroyRawWidget(TN_WIDGET *widget)
{
	DeleteFromRegistry(widget);
	return;
}

GR_EVENT *tnGetLastEvent(TN_WIDGET *widget)
{
	
	return ( (widget->type==TN_RAWWIDGET)?widget->WSpec.rawwidget.lastevent:NULL);
}
