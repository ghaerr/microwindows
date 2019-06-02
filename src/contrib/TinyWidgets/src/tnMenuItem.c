 /*C Source file for the Menu Item widget
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
/*Create Menu Item widget*/
void
CreateMenuItem (TN_WIDGET * widget,
	      TN_WIDGET * parent,
	      int posx,
	      int posy,
	      char *caption,
	      GR_SIZE height,
	      GR_SIZE width,
	      char *fontname,
	      GR_SIZE fontsize, GR_COLOR bgcolor, GR_COLOR fgcolor,GR_BOOL checkable)
{
  int i;
  GR_FONT_ID font; 
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth, captionheight, base;
  GR_WINDOW_ID parentwid;
  if(strcmp(fontname,""))
	  font=GrCreateFont (fontname, fontsize, NULL);
  else
	  font=TN_DEFAULT_FONT_NO;

  GrSetGCFont (widget->gc, font);
  if (height == 0) 
     height = TN_MENUITEM_HEIGHT;
  if(width==0)
      width = TN_MENUITEM_WIDTH;
  if (caption)
    strcpy ((widget->WSpec.menuitem.caption), caption);
  widget->WSpec.menuitem.FGColor = fgcolor;
  widget->WSpec.menuitem.checkable=checkable;
  /*Resize window to accomodate caption*/
    if(width==TN_MENUITEM_WIDTH)
    {
	    GrGetGCTextSize (widget->gc, widget->WSpec.menuitem.caption, -1, GR_TFASCII,&captionwidth, &captionheight, &base);
	    width=captionwidth+2;
    }
    
  if(parent->type==TN_POPUPMENU)
  {
         widget->WSpec.menuitem.popupmenuwid=parent->wid;
	 posy=parent->WSpec.popupmenu.lasty;
	 posx+=1;
	 parent->WSpec.popupmenu.lasty+=height;
	 parentwid=parent->WSpec.popupmenu.container;
	 GrGetWindowInfo(parent->WSpec.popupmenu.container,&winfo);
	 GrResizeWindow(parent->WSpec.popupmenu.container,winfo.width,winfo.height+height+1);
	 width=winfo.width-2;
  }
  else if(parent->type==TN_CASCADEMENU)
  {
	 widget->WSpec.menuitem.popupmenuwid=parent->wid;
         posy=parent->WSpec.cascademenu.lasty;
         posx+=1;
         parent->WSpec.cascademenu.lasty+=height;
         parentwid=parent->WSpec.cascademenu.container;
         GrGetWindowInfo(parent->WSpec.cascademenu.container,&winfo);
         GrResizeWindow(parent->WSpec.cascademenu.container,winfo.width,winfo.height+height+1);
         width=winfo.width-2;
  }
  else
  {
  /*parent is a menubar*/
  	parentwid=parent->wid;
	posx=parent->WSpec.menubar.lastx;
	parent->WSpec.menubar.lastx+=width;
	posy+=1;
  }
  /*Attach to the Menu Bar or the Pop up menu container or the Cascade Menu container*/
  widget->wid =
    GrNewWindow (parentwid, posx, posy, width, height, 0, bgcolor, 0);

  for (i = 0; i < MENUITEM_CALLBACKS; i++)
	      widget->WSpec.menuitem.CallBack[i].fp = NULL;
  widget->WSpec.menuitem.selected = GR_FALSE;
  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_EXPOSURE);
  GrSetGCUseBackground(widget->gc,GR_FALSE);
  return;
}

/*Event Handler for the Menu Item*/
void
MenuItemEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  TN_WIDGET *parent,*parentmenu,*sibling;
  GR_WINDOW_INFO winfo;
  GR_WINDOW_ID siblingwid,parentwid;
  GR_BOOL exclusive;
  
  GrGetWindowInfo(widget->wid,&winfo);
  parent=GetFromRegistry(winfo.parent);
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_DOWN:
	    if(widget->enabled==GR_FALSE) break;
	    if(parent==NULL) /*parent is the popup menu or the cascade menucontainer*/
	    {
		    UnmapForwardContainers(winfo.parent);
		    parentmenu=GetFromRegistry(widget->WSpec.menuitem.popupmenuwid);
		    if(parentmenu->type==TN_POPUPMENU)
		    {
			    parentmenu->WSpec.popupmenu.selected=GR_FALSE;
			    GrClearWindow(widget->WSpec.menuitem.popupmenuwid,GR_FALSE);
			    DrawPopUpMenu(parentmenu);
		    }
		    else
		    {
                            parentmenu->WSpec.cascademenu.selected=GR_FALSE;
                            GrClearWindow(widget->WSpec.menuitem.popupmenuwid,GR_FALSE);
                            DrawCascadeMenu(parentmenu);
		    }
			    
			    GrMapWindow(winfo.parent);
	    }
	    else 
	    {
		    GrSetFocus(winfo.parent);
		    GrGetWindowInfo(winfo.parent,&winfo);
		    siblingwid=winfo.child;
		    while(siblingwid!=0)
		    {
			    if(siblingwid!=widget->wid)
			    {
				    sibling=GetFromRegistry(siblingwid);
					    if(sibling->type==TN_POPUPMENU)
					    {
						    sibling->WSpec.popupmenu.selected=GR_FALSE;
						    GrUnmapWindow(sibling->WSpec.popupmenu.container);
						    GrClearWindow(siblingwid,GR_FALSE);
						    DrawPopUpMenu(sibling);
					    }
					    else /*its a menu item*/
					    {
	      					    sibling->WSpec.menuitem.selected=GR_FALSE;
	   					    GrClearWindow(siblingwid,GR_FALSE);
						    DrawMenuItem(sibling);
				     	    }
			    }
			    GrGetWindowInfo(siblingwid,&winfo);
			    siblingwid=winfo.sibling;
		    }
				      
	    }		    
	    if(widget->WSpec.menuitem.checkable==GR_TRUE)
	    {
		    parentmenu=GetFromRegistry(widget->WSpec.menuitem.popupmenuwid);
		    if(parentmenu->type==TN_POPUPMENU)
		    {
			    parentwid=parentmenu->WSpec.popupmenu.container;
			    exclusive=parentmenu->WSpec.popupmenu.exclusive;
		    }
		    else
		    {
			    parentwid=parentmenu->WSpec.cascademenu.container;
			    exclusive=parentmenu->WSpec.cascademenu.exclusive;
		    }
		    if(exclusive==GR_TRUE)
		    {
		    GrGetWindowInfo(parentwid,&winfo);
		    siblingwid=winfo.child;
		    while(siblingwid!=0)
		    {
			    sibling=GetFromRegistry(siblingwid);
			    if(sibling->type==TN_MENUITEM && sibling->WSpec.menuitem.checkable==GR_TRUE && sibling!=widget)
			    {
				    sibling->WSpec.menuitem.selected=GR_FALSE;
				    GrClearWindow(siblingwid,GR_FALSE);
				    DrawMenuItem(sibling);
			    }
			    GrGetWindowInfo(siblingwid,&winfo);
			    siblingwid=winfo.sibling;
		    }
		    }
		    widget->WSpec.menuitem.selected^=GR_TRUE;
	    }
	    else
		    widget->WSpec.menuitem.selected=GR_TRUE;
	    GrClearWindow(widget->wid,GR_FALSE);
	    DrawMenuItem(widget);
      break;
    case GR_EVENT_TYPE_BUTTON_UP:
           if(widget->enabled==GR_FALSE) break;
      	   if(widget->WSpec.menuitem.checkable==GR_FALSE)
	       	   widget->WSpec.menuitem.selected=GR_FALSE;
	   if(parent==NULL) /*parent is the popup menu or the cascade menu container*/
	   {
		   parentmenu=GetFromRegistry(widget->WSpec.menuitem.popupmenuwid);
		   UnmapAllContainers(parentmenu);
	   }
	   
           GrClearWindow(widget->wid,GR_FALSE);
           DrawMenuItem(widget);
	    /*Call user callback */
	   if (widget->WSpec.menuitem.CallBack[CLICKED].fp)
		  (*(widget->WSpec.menuitem.CallBack[CLICKED].fp)) (widget,
						    widget->WSpec.menuitem.
						    CallBack[CLICKED].dptr);
      break;
    case GR_EVENT_TYPE_EXPOSURE:
      DrawMenuItem (widget);
      if(widget->enabled==GR_FALSE) break;
      if(parent==NULL)
      {	      
	      parentmenu=GetFromRegistry(widget->WSpec.menuitem.popupmenuwid);
	      GrClearWindow(widget->WSpec.menuitem.popupmenuwid,GR_FALSE);
	      if(parentmenu->type==TN_POPUPMENU) 
		      DrawPopUpMenu(parentmenu);
	      else
		      DrawCascadeMenu(parentmenu);
      }
      break;
    }
}


/*Menu Item draw routine*/
void
DrawMenuItem (TN_WIDGET * menuitem)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth, captionheight, base;
  GR_SIZE captionx, captiony;
  TN_WIDGET *parent; 
	if(menuitem->visible==GR_FALSE) return;
  GrGetWindowInfo (menuitem->wid, &winfo);
	      
  /*Draw Menu Item caption */
  GrSetGCForeground (menuitem->gc, menuitem->WSpec.menuitem.FGColor);
  GrGetGCTextSize (menuitem->gc, menuitem->WSpec.menuitem.caption, -1, GR_TFASCII, &captionwidth, &captionheight, &base);
  parent=GetFromRegistry(winfo.parent);
  if(parent==NULL)
	  captionx=0;
  else
	  captionx = (winfo.width - captionwidth) / 2;
  captiony = (winfo.height - captionheight) / 2;
  if(captionx<0)
    captionx = 0;		/*if window height or width < */
  if (captiony < 0)
    captiony = 0;		/*caption height or width - reset */
  
  if(menuitem->WSpec.menuitem.selected)
  {
	  if(menuitem->WSpec.menuitem.checkable==GR_TRUE)
	  {
		  GrSetGCForeground(menuitem->gc,GR_RGB(0,0,0));
		  GrFillEllipse(menuitem->wid,menuitem->gc,captionwidth+4,winfo.height/2+1,2,2);
	  }
	  else
	  {
		  GrSetGCForeground(menuitem->gc,GR_RGB(100,100,255));
		  GrFillRect(menuitem->wid,menuitem->gc,0,0,winfo.width-1,winfo.height-1);
		  GrSetGCForeground(menuitem->gc,GR_RGB(255,255,255));
	  }
  }
  else   GrSetGCForeground (menuitem->gc,menuitem->WSpec.popupmenu.FGColor);
  if(menuitem->enabled==GR_FALSE) 
	  GrSetGCForeground(menuitem->gc,GR_RGB(120,120,120));
  else 
	  GrSetGCForeground (menuitem->gc,menuitem->WSpec.popupmenu.FGColor);
	 GrText (menuitem->wid, menuitem->gc, captionx, captiony, menuitem->WSpec.menuitem.caption,-1, GR_TFASCII | GR_TFTOP);  
  return;
}

void UnmapAllContainers(TN_WIDGET * widget)
{
	if(widget->type==TN_CASCADEMENU)
	{
		GrUnmapWindow(widget->WSpec.cascademenu.container);
      		widget->WSpec.cascademenu.selected=GR_FALSE;
 		GrClearWindow(widget->wid,GR_FALSE);
		DrawCascadeMenu(widget);
								
		UnmapAllContainers(GetFromRegistry(widget->WSpec.cascademenu.popupmenuwid));
	}
	else 
	{
		GrUnmapWindow(widget->WSpec.popupmenu.container);
		widget->WSpec.popupmenu.selected=GR_FALSE;
		GrClearWindow(widget->wid,GR_FALSE);
		DrawPopUpMenu(widget);
	}
}
		
void UnmapForwardContainers(GR_WINDOW_ID containerwid)
{
	GR_WINDOW_INFO winfo;
	GR_WINDOW_ID siblingwid;
	TN_WIDGET *sibling;
	GrGetWindowInfo(containerwid,&winfo);
	siblingwid=winfo.child;
	while(siblingwid!=0)
	{
		sibling=GetFromRegistry(siblingwid);
		if(sibling->type==TN_CASCADEMENU)
		{
			GrUnmapWindow(sibling->WSpec.cascademenu.container);
			sibling->WSpec.cascademenu.selected=GR_FALSE;
			GrClearWindow(siblingwid,GR_FALSE);
			DrawCascadeMenu(sibling);
			UnmapForwardContainers(sibling->WSpec.cascademenu.container);
		}
		GrGetWindowInfo(siblingwid,&winfo);
		siblingwid=winfo.sibling;
	}
}

void DestroyMenuItem(TN_WIDGET *widget)
{
	DeleteFromRegistry(widget);
	return;
}
		
