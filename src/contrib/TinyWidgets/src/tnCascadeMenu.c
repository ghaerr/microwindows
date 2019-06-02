 /*C Source file for the Cascade Menu widget
  * This file is part of `TinyWidgets', a widget set for the nano-X GUI which is  * a part of the Microwindows project (www.microwindows.org).
  * Copyright C 2000
  * Sunil Soman <sunil_soman@vsnl.com>
  * Amit Kulkarni <amms@vsnl.net>
  * Navin Thadani <navs@vsnl.net>
  *
  * This library is free software; you can redistribute it and/or modify it
  * under the terms of the GNU Lesser General Public License as published by the  * Free Software Foundation; either version 2.1 of the License, 
  * or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful, but WITHOUT
  * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License  * for more details.
  *
  * You should have received a copy of the GNU Lesser General Public License 
  * along with this library;if not, write to the Free Software Foundation, Inc.,
  * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
  */

#include "../include/tnWidgets.h"
/*Create Cascade Menu widget*/
void
CreateCascadeMenu (TN_WIDGET * widget,
		 TN_WIDGET * parent,
		 int posx,
		 int posy,
		 char *caption,
		 GR_SIZE height,
		 GR_SIZE width,
		 char *fontname,
		 GR_SIZE fontsize, GR_COLOR bgcolor, GR_COLOR fgcolor,GR_BOOL exclusive)
{
  GR_FONT_ID font;
  GR_WINDOW_INFO winfo;
  GR_WINDOW_ID parentwid,appwindowwid;
  TN_WIDGET *temp;
  GR_SIZE captionwidth, captionheight, base;
  GR_COORD containerx,containery; //,lasty;
  if(strcmp(fontname,""))
	  font=GrCreateFont (fontname, fontsize, NULL);
  else
	  font=TN_DEFAULT_FONT_NO;
  GrSetGCFont (widget->gc, font);
  if (height == 0)
    height = TN_CASCADEMENU_HEIGHT;
  if (width == 0)
    width = TN_CASCADEMENU_WIDTH;
  if (caption)
    strcpy ((widget->WSpec.cascademenu.caption), caption);
  widget->WSpec.cascademenu.FGColor = fgcolor;
  widget->WSpec.cascademenu.selected = GR_FALSE;
  widget->WSpec.cascademenu.exclusive=exclusive;
  widget->WSpec.cascademenu.lasty = 1;
  posx+=1;
  if(parent->type==TN_POPUPMENU)
  {
	  posy=parent->WSpec.popupmenu.lasty;
	  //lasty=parent->WSpec.popupmenu.lasty;
	  parent->WSpec.popupmenu.lasty+=height;
	  parentwid=parent->WSpec.popupmenu.container;
  }
  else
  {
	            posy=parent->WSpec.cascademenu.lasty;
	            //lasty=parent->WSpec.cascademenu.lasty;
		    parent->WSpec.cascademenu.lasty+=height;
		    parentwid=parent->WSpec.cascademenu.container;
  }
			      
  widget->WSpec.cascademenu.popupmenuwid=parent->wid;  
  if (width == TN_CASCADEMENU_WIDTH)
    {
      GrGetGCTextSize (widget->gc, widget->WSpec.cascademenu.caption, -1,
		       GR_TFASCII, &captionwidth, &captionheight, &base);
      width = captionwidth + 12;
    }

  /*Resize parent container*/
    GrGetWindowInfo(parentwid,&winfo);
    GrResizeWindow(parentwid,winfo.width,winfo.height+height+1);
    width=winfo.width-2;
  widget->wid =
    GrNewWindow (parentwid, posx, posy, width, height, 0, bgcolor, 0);
  /*Find posx posxy for the container*/
    GrGetWindowInfo(widget->wid,&winfo);
	containerx = winfo.width - 4 - 1;			// frame width is 2*2
	containery = winfo.height + 2 - 1;

  /*Find parent for the container & create container window for menu items */
   temp=widget;

  while(1)
  {
	  temp=GetFromRegistry(temp->WSpec.cascademenu.popupmenuwid);
  	GrGetWindowInfo(temp->wid,&winfo);			// this code may need to move up one line or down two lines
  	containerx += winfo.width;
  	containery += winfo.height;
	  if(temp->type!=TN_CASCADEMENU)
	  	break;
  }

  GrGetWindowInfo(temp->wid,&winfo);
  //containerx += winfo.width;
  //containery += winfo.height;
  GrGetWindowInfo(winfo.parent,&winfo);
  appwindowwid=winfo.parent;
  GrGetWindowInfo(winfo.parent,&winfo);
  //containerx -= winfo.x;
  //containery -= winfo.height;
  //containerx-=winfo.x;			// original code
  //containery-=winfo.y;
  widget->WSpec.cascademenu.container =
	  GrNewWindow (appwindowwid, containerx,containery, CONTAINER_WIDTH, CONTAINER_HEIGHT, 0, bgcolor, 0);
  
  GrSelectEvents (widget->wid, GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_EXPOSURE);
  GrSetGCUseBackground (widget->gc, GR_FALSE);
  return;
}

/*Event Handler for the Cascade Menu*/
void
CascadeMenuEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  GR_WINDOW_INFO winfo;
  GR_WINDOW_ID siblingwid, parentwid;
  TN_WIDGET *sibling,*popupmenu; //*parent
  
  GrGetWindowInfo(widget->wid,&winfo);
  //parent=GetFromRegistry(winfo.parent);
      
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_DOWN:
      if(widget->enabled==GR_FALSE) break;
      widget->WSpec.cascademenu.selected ^= GR_TRUE;
      GrClearWindow (widget->wid, GR_FALSE);
      DrawCascadeMenu (widget);

      if (widget->WSpec.cascademenu.selected == GR_TRUE)
	{
	  GrMapWindow (widget->WSpec.cascademenu.container);
	  GrGetWindowInfo (widget->wid, &winfo);
	  parentwid = winfo.parent;
	  GrGetWindowInfo (parentwid, &winfo);
	  siblingwid = winfo.child;
	  while (siblingwid != 0)
	    {
	      sibling = GetFromRegistry (siblingwid);
	      if (sibling != widget)
		{
			if(sibling->type==TN_CASCADEMENU)
			{
		      		sibling->WSpec.cascademenu.selected = GR_FALSE;
				GrUnmapWindow (sibling->WSpec.cascademenu.container);
		      		GrClearWindow (sibling->wid, GR_FALSE);
			      	DrawCascadeMenu (sibling);
			}
			else
			{
				/*its a menu item*/
				sibling->WSpec.menuitem.selected=GR_FALSE;
				GrClearWindow (sibling->wid, GR_FALSE);
				DrawMenuItem(sibling);
			}
		}
	      GrGetWindowInfo (siblingwid, &winfo);
	      siblingwid = winfo.sibling;
	    }
	}
      else
	GrUnmapWindow (widget->WSpec.cascademenu.container);
      UnmapForwardContainers(widget->WSpec.cascademenu.container);
      break;

    case GR_EVENT_TYPE_EXPOSURE:
      DrawCascadeMenu (widget);
      if(widget->enabled==GR_FALSE) break;
      popupmenu=GetFromRegistry(widget->WSpec.cascademenu.popupmenuwid);
      GrClearWindow(widget->WSpec.cascademenu.popupmenuwid,GR_FALSE);
      DrawPopUpMenu(popupmenu);
      break;
    }
}


/*Cascade Menu draw routine*/
void
DrawCascadeMenu(TN_WIDGET *cascademenu)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth, captionheight, base;
  GR_SIZE captionx, captiony;
  GR_POINT arrow[3];
  if(cascademenu->visible==GR_FALSE) return;
  GrGetWindowInfo (cascademenu->wid, &winfo);

  /*Draw Cascade Menu caption */

  GrSetGCForeground (cascademenu->gc, cascademenu->WSpec.cascademenu.FGColor);
  GrGetGCTextSize (cascademenu->gc, cascademenu->WSpec.cascademenu.caption, -1,
		   GR_TFASCII, &captionwidth, &captionheight, &base);
  captiony = (winfo.height - captionheight) / 2;

  captionx = 0;		/*if window height or width < */
  if (captiony < 0)
    captiony = 0;		/*caption height or width - reset */

  if (cascademenu->WSpec.cascademenu.selected)
    {
      GrSetGCForeground (cascademenu->gc, GR_RGB (100,100, 255));
      GrFillRect (cascademenu->wid, cascademenu->gc, 0, 0, winfo.width - 1,
		  winfo.height - 1);
      GrSetGCForeground (cascademenu->gc, GR_RGB (255, 255, 255));
    }
  else
    GrSetGCForeground (cascademenu->gc, cascademenu->WSpec.cascademenu.FGColor);
  if(cascademenu->enabled==GR_FALSE)
	  GrSetGCForeground (cascademenu->gc, GR_RGB(120,120,120));
  else
	  GrSetGCForeground (cascademenu->gc, cascademenu->WSpec.cascademenu.FGColor);
			  
  GrText (cascademenu->wid, cascademenu->gc, captionx, captiony,
	  cascademenu->WSpec.cascademenu.caption, -1, GR_TFASCII | GR_TFTOP);
  /*Draw arrow*/
  GrSetGCForeground(cascademenu->gc,GR_RGB(255,255,255));
  arrow[0].x=captionwidth+12;
  arrow[0].y=winfo.height/2;
  arrow[1].x=captionwidth+5;
  arrow[1].y=arrow[0].y+4;
  arrow[2].x=captionwidth+5;
  arrow[2].y=arrow[0].y-4;
  GrFillPoly(cascademenu->wid,cascademenu->gc,3,arrow);
 /*Draw border for the container*/ 
  GrGetWindowInfo(cascademenu->WSpec.cascademenu.container,&winfo);
  GrSetGCForeground (cascademenu->gc, GR_RGB (0, 0, 0));
  GrLine (cascademenu->WSpec.cascademenu.container, cascademenu->gc, 0, winfo.height - 1, winfo.width - 1, winfo.height - 1);
  GrLine (cascademenu->WSpec.cascademenu.container, cascademenu->gc, winfo.width - 1, 0, winfo.width - 1, winfo.height - 1);
  GrSetGCForeground (cascademenu->gc, GR_RGB (255, 255, 255));
  GrLine (cascademenu->WSpec.cascademenu.container, cascademenu->gc, 0, 0, winfo.width - 1, 0);
  GrLine (cascademenu->WSpec.cascademenu.container, cascademenu->gc, 0, 0, 0, winfo.height - 1);
	    
  return;
}

void DestroyCascadeMenu(TN_WIDGET *widget)
{
	GR_WINDOW_INFO winfo;
	GR_WINDOW_ID siblingwid;
	TN_WIDGET *sibling;

	GrGetWindowInfo(widget->WSpec.cascademenu.container,&winfo);
	siblingwid=winfo.child;
	while(siblingwid!=0)
	{
		sibling=GetFromRegistry(siblingwid);
		if(sibling->type==TN_CASCADEMENU)
			DestroyCascadeMenu(sibling);
		else
			DestroyMenuItem(sibling);
		sibling=NULL;
		GrGetWindowInfo(siblingwid,&winfo);
		siblingwid=winfo.sibling;
	}
	GrDestroyWindow(widget->WSpec.cascademenu.container);
	DeleteFromRegistry(widget);
	return;
}
