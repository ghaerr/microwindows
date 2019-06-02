 /*C Source file for the PopUp Menu widget
    * This file is part of `TinyWidgets', a widget set for the nano-X GUI which 
    * is a part of the Microwindows project (www.microwindows.org).
    * Copyright C 2000
    * Sunil Soman <sunil_soman@vsnl.com>
    * Amit Kulkarni <amms@vsnl.net>
    * Navin Thadani <navs@vsnl.net>
    *
    * This library is free software; you can redistribute it and/or modify it
    * under the terms of the GNU Lesser General Public License as published by 
    * the Free Software Foundation; either version 2.1 of the License, 
    * or (at your option) any later version.
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
/*Create PopUp Menu widget*/
void
CreatePopUpMenu (TN_WIDGET * widget,
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
  GR_SIZE captionwidth, captionheight, base;
  if(strcmp(fontname,""))
	  font= GrCreateFont (fontname, fontsize, NULL);
  else
	  font=TN_DEFAULT_FONT_NO;

  GrSetGCFont (widget->gc, font);
  if (height == 0)
    height = TN_POPUPMENU_HEIGHT;
  if (width == 0)
    width = TN_POPUPMENU_WIDTH;
  if (caption)
    strcpy ((widget->WSpec.popupmenu.caption), caption);
  widget->WSpec.popupmenu.FGColor = fgcolor;
  widget->WSpec.popupmenu.selected = GR_FALSE;
  widget->WSpec.popupmenu.exclusive=exclusive;
	  
  widget->WSpec.popupmenu.lasty = 1;
  if (posx == TN_AUTO)
    posx = parent->WSpec.menubar.lastx;

  if (width == TN_POPUPMENU_WIDTH)
    {
      GrGetGCTextSize (widget->gc, widget->WSpec.popupmenu.caption, -1,
		       GR_TFASCII, &captionwidth, &captionheight, &base);
      width = captionwidth + 2;
    }

  widget->wid =
    GrNewWindow (parent->wid, posx, posy + 1, width, height, 0, bgcolor, 0);

  /*Create container window for menu items */
  /*Container is a sibling of the menubar */
  GrGetWindowInfo (parent->wid, &winfo);
  widget->WSpec.popupmenu.container =
    GrNewWindow (winfo.parent, posx, posy + height, CONTAINER_WIDTH,
		 CONTAINER_HEIGHT, 0, bgcolor, 0);

  GrGetWindowInfo (widget->wid, &winfo);
  parent->WSpec.menubar.lastx += winfo.width;
  GrSelectEvents (widget->wid, GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_FOCUS_OUT);
  GrSetGCUseBackground (widget->gc, GR_FALSE);
  return;
}

/*Event Handler for the PopUp Menu*/
void
PopUpMenuEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  GR_WINDOW_INFO winfo;
  GR_WINDOW_ID siblingwid, parentwid;
  TN_WIDGET *sibling;
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_DOWN:
      if(widget->enabled==GR_FALSE) break;
      widget->WSpec.popupmenu.selected ^= GR_TRUE;
      GrClearWindow (widget->wid, GR_FALSE);
      DrawPopUpMenu (widget);

      if (widget->WSpec.popupmenu.selected == GR_TRUE)
	{
	  GrMapWindow (widget->WSpec.popupmenu.container);
	  GrGetWindowInfo (widget->wid, &winfo);
	  parentwid = winfo.parent;
	  GrGetWindowInfo (parentwid, &winfo);
	  siblingwid = winfo.child;
	  while (siblingwid != 0)
	    {
	      sibling = GetFromRegistry (siblingwid);
	      if (sibling != widget)
		{
		  sibling->WSpec.popupmenu.selected = GR_FALSE;
		  GrUnmapWindow (sibling->WSpec.popupmenu.container);
		  GrClearWindow (sibling->wid, GR_FALSE);
		  DrawPopUpMenu (sibling);
		}
	      GrGetWindowInfo (siblingwid, &winfo);
	      siblingwid = winfo.sibling;
	    }
	}
      else
	      ClearAllContainers(widget);
      break;

    case GR_EVENT_TYPE_FOCUS_OUT:
      ClearAllContainers(widget);
      widget->WSpec.popupmenu.selected = GR_FALSE;
      GrClearWindow (widget->wid, GR_FALSE);
      DrawPopUpMenu (widget);
      break;

    case GR_EVENT_TYPE_EXPOSURE:
      DrawPopUpMenu (widget);
      break;
    }
}


/*PopUp Menu draw routine*/
void
DrawPopUpMenu (TN_WIDGET * popupmenu)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth, captionheight, base;
  GR_SIZE captionx, captiony;
  
  GrGetWindowInfo (popupmenu->wid, &winfo);


  /*Draw PopUp Menu caption */

  GrSetGCForeground (popupmenu->gc, popupmenu->WSpec.popupmenu.FGColor);
  GrGetGCTextSize (popupmenu->gc, popupmenu->WSpec.popupmenu.caption, -1,
		   GR_TFASCII, &captionwidth, &captionheight, &base);
  captionx = (winfo.width - captionwidth) / 2;
  captiony = (winfo.height - captionheight) / 2;

  if (captionx < 0)
    captionx = 0;		/*if window height or width < */
  if (captiony < 0)
    captiony = 0;		/*caption height or width - reset */

  if (popupmenu->WSpec.popupmenu.selected)
    {
      GrSetGCForeground (popupmenu->gc, GR_RGB (100,100, 255));
      GrFillRect (popupmenu->wid, popupmenu->gc, 0, 0, winfo.width - 1,
		  winfo.height - 1);
      GrSetGCForeground (popupmenu->gc, GR_RGB (255, 255, 255));
    }
  else
    GrSetGCForeground (popupmenu->gc, popupmenu->WSpec.popupmenu.FGColor);
  if(popupmenu->enabled==GR_FALSE)
	  GrSetGCForeground (popupmenu->gc,GR_RGB(120,120,120));
  else
	  GrSetGCForeground (popupmenu->gc, popupmenu->WSpec.popupmenu.FGColor);
  GrText (popupmenu->wid, popupmenu->gc, captionx, captiony,
	  popupmenu->WSpec.popupmenu.caption, -1, GR_TFASCII | GR_TFTOP);
  
  GrGetWindowInfo(popupmenu->WSpec.popupmenu.container,&winfo);
  GrSetGCForeground (popupmenu->gc, GR_RGB (0, 0, 0));
  GrLine (popupmenu->WSpec.popupmenu.container, popupmenu->gc, 0, winfo.height - 1, winfo.width - 1, winfo.height - 1);
  GrLine (popupmenu->WSpec.popupmenu.container, popupmenu->gc, winfo.width - 1, 0, winfo.width - 1, winfo.height - 1);
  GrSetGCForeground (popupmenu->gc, GR_RGB (255, 255, 255));
  GrLine (popupmenu->WSpec.popupmenu.container, popupmenu->gc, 0, 0, winfo.width - 1, 0);
  GrLine (popupmenu->WSpec.popupmenu.container, popupmenu->gc, 0, 0, 0, winfo.height - 1);
	    
  return;
}

void ClearAllContainers(TN_WIDGET *widget)
{
	GR_WINDOW_ID siblingwid;
	GR_WINDOW_INFO winfo;
	TN_WIDGET *sibling;
	
	GrUnmapWindow(widget->WSpec.popupmenu.container);
	GrGetWindowInfo(widget->WSpec.popupmenu.container,&winfo);
	siblingwid=winfo.child;
	while(siblingwid!=0)
	{
		sibling=GetFromRegistry(siblingwid);
		if(sibling->type==TN_CASCADEMENU)
		{
			sibling->WSpec.cascademenu.selected=GR_FALSE;
			GrUnmapWindow(sibling->WSpec.cascademenu.container);
			ClearCascadeContainer(sibling);
		}
		GrGetWindowInfo(siblingwid,&winfo);
		siblingwid=winfo.sibling;
	}
return;
}
					
void ClearCascadeContainer(TN_WIDGET *widget)
{
        GR_WINDOW_ID siblingwid;
        GR_WINDOW_INFO winfo;
	TN_WIDGET *sibling;

        GrGetWindowInfo(widget->WSpec.cascademenu.container,&winfo);
        siblingwid=winfo.child;
	while(siblingwid!=0)
	{
		sibling=GetFromRegistry(siblingwid);
		if(sibling->type==TN_CASCADEMENU)
		{
			sibling->WSpec.cascademenu.selected=GR_FALSE;
			GrUnmapWindow(sibling->WSpec.cascademenu.container);
			ClearCascadeContainer(sibling);
		}
		GrGetWindowInfo(siblingwid,&winfo);
                siblingwid=winfo.sibling;
	}
	return;
}

void DestroyPopUpMenu(TN_WIDGET *widget)
{
	GR_WINDOW_INFO winfo;
	GR_WINDOW_ID siblingwid;
	TN_WIDGET *sibling;
	GrGetWindowInfo(widget->WSpec.popupmenu.container,&winfo);
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
	GrDestroyWindow(widget->WSpec.popupmenu.container);
	DeleteFromRegistry(widget);
	return;
}
