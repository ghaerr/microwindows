 /*C Source file for the Menu Bar widget
    * This file is part of `TinyWidgets', a widget set for the nano-X GUI which 
    * is  * a part of the Microwindows project (www.microwindows.org).
    * Copyright C 2000
    * Sunil Soman <sunil_soman@vsnl.com>
    * Amit Kulkarni <amms@vsnl.net>
    * Navin Thadani <navs@vsnl.net>
    *
    * This library is free software; you can redistribute it and/or modify it
    * under the terms of the GNU Lesser General Public License as published by 
    * the Free Software Foundation; either version 2.1 of the License, 
    * or (at your option)
    * any later version.
    *
    * This library is distributed in the hope that it will be useful, but 
    * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY    * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
    * License for more details.
    *
    * You should have received a copy of the GNU Lesser General Public License 
    * along with this library; if not, write to the Free Software Foundation, 
    * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
  */

#include "../include/tnWidgets.h"
/*Create Menu Bar widget*/
void
CreateMenuBar (TN_WIDGET * widget,
	       TN_WIDGET * parent,
	       int posx,
	       int posy, GR_SIZE height, GR_SIZE width, GR_COLOR bgcolor)
{
  if (height == 0)
    height = TN_MENUBAR_HEIGHT;
  if (width == 0)
    width = TN_MENUBAR_WIDTH;
  widget->WSpec.menubar.lastx = 2;
  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);

  GrSelectEvents (widget->wid, GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_EXPOSURE);
  return;
}

/*Event Handler for the Menu Bar*/
void
MenuBarEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_DOWN:
      GrSetFocus (event->button.subwid);
      if (widget->WSpec.menubar.CallBack[CLICKED].fp)
	      (*(widget->WSpec.menubar.CallBack[CLICKED].fp)) (widget,widget->WSpec.menubar.CallBack[CLICKED].dptr);
      break;
    
    case GR_EVENT_TYPE_EXPOSURE:
      DrawMenuBar (widget);
      break;
    }
}


/*Menu Bar draw routine*/
void
DrawMenuBar (TN_WIDGET * menubar)
{
  GR_WINDOW_INFO winfo, parentinfo;
						    
  GrGetWindowInfo (menubar->wid, &winfo);
      GrGetWindowInfo (winfo.parent, &parentinfo);
      GrResizeWindow (menubar->wid, parentinfo.width, winfo.height);
      GrGetWindowInfo (menubar->wid, &winfo);
  GrSetGCForeground (menubar->gc, GR_RGB (0, 0, 0));
  GrLine (menubar->wid, menubar->gc, 0, winfo.height - 1, winfo.width - 1,
	  winfo.height - 1);
  GrLine (menubar->wid, menubar->gc, winfo.width - 1, 0, winfo.width - 1,
	  winfo.height - 1);
  GrSetGCForeground (menubar->gc, GR_RGB (255, 255, 255));
  GrLine (menubar->wid, menubar->gc, 0, 0, winfo.width - 1, 0);
  GrLine (menubar->wid, menubar->gc, 0, 0, 0, winfo.height - 1);
  return;
}

void DestroyMenuBar(TN_WIDGET *widget)
{
	DeleteFromRegistry(widget);
	return;
}
	
