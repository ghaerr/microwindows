 /*C Source file for the Push Button widget
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
/*Create Push Button widget*/
void
CreateButton (TN_WIDGET * widget,
	      TN_WIDGET * parent,
	      int posx,
	      int posy,
	      char *caption,
	      GR_SIZE height,
	      GR_SIZE width,
	      char *fontname,
	      GR_SIZE fontsize, GR_COLOR bgcolor, GR_COLOR fgcolor,GR_BOOL haspixmap,char *filename)
{
  int i;
  GR_FONT_ID font;
  if(strcmp(fontname,""))
	  font=GrCreateFont (fontname, fontsize, NULL);
  else
	  font=TN_DEFAULT_FONT_NO;
  
  GrSetGCFont (widget->gc, font);
  if (height == 0) 
      height = TN_BUTTON_HEIGHT;
  if(width==0)
      width = TN_BUTTON_WIDTH;
  widget->WSpec.button.haspixmap=haspixmap;
  if(haspixmap==GR_TRUE)
  {
	  widget->WSpec.button.face.imageid=GrLoadImageFromFile(filename,0);
	  widget->WSpec.button.face.caption=strdup(filename);
  }
  else  
  if(caption!=NULL)
	  widget->WSpec.button.face.caption=strdup(caption);
  else
	  widget->WSpec.button.face.caption=strdup("Button");
  
  widget->WSpec.button.FGColor = fgcolor;

  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);

  for (i = 0; i < BUTTON_CALLBACKS; i++)
    widget->WSpec.button.CallBack[i].fp = NULL;
  widget->WSpec.button.st_down = GR_FALSE;
  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_EXPOSURE);
  return;
}

/*Event Handler for the Push Button*/
void
ButtonEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_DOWN:
      if(!widget->enabled || !widget->visible)
         break;
      widget->WSpec.button.st_down = GR_TRUE;	/*Status=down */
      widget->WSpec.button.changebuttons = event->button.changebuttons;
      GrSetFocus (widget->wid);
      GrClearWindow (widget->wid, GR_FALSE);
      DrawButton (widget);
      break;
    case GR_EVENT_TYPE_BUTTON_UP:
      if(widget->enabled==GR_FALSE)
         break;
      widget->WSpec.button.st_down = GR_FALSE;	/*Status=up */
      GrClearWindow (widget->wid, GR_FALSE);
      DrawButton (widget);
      /*Call user callback */
      if (widget->WSpec.button.CallBack[CLICKED].fp)
	
	  (*(widget->WSpec.button.CallBack[CLICKED].fp)) (widget,
						    widget->WSpec.button.
						    CallBack[CLICKED].dptr);
      break;
    case GR_EVENT_TYPE_EXPOSURE:
      DrawButton (widget);
      break;
    }
}


/*Push Button draw routine*/
void
DrawButton (TN_WIDGET * button)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth, captionheight, base;
  GR_SIZE captionx, captiony;
  
  GrGetWindowInfo (button->wid, &winfo);

  /*Draw Button borders */
  if (!button->WSpec.button.st_down)
    GrSetGCForeground (button->gc, GR_RGB (255, 255, 255));
  else
    GrSetGCForeground (button->gc, GR_RGB (0, 0, 0));

  GrLine (button->wid, button->gc, 0, 0, winfo.width - 1, 0);
  GrLine (button->wid, button->gc, 0, 0, 0, winfo.height - 1);

  if (!button->WSpec.button.st_down)
    GrSetGCForeground (button->gc, GR_RGB (0, 0, 0));
  else
    GrSetGCForeground (button->gc, GR_RGB (255, 255, 255));

  GrLine (button->wid, button->gc, winfo.width - 1, 0, winfo.width - 1,
	  winfo.height - 1);
  GrLine (button->wid, button->gc, 0, winfo.height - 1, winfo.width - 1,
	  winfo.height - 1);

  /*Draw Button caption */
  if(button->enabled)
     GrSetGCForeground (button->gc, button->WSpec.button.FGColor);
  else
     GrSetGCForeground (button->gc, GR_RGB(120,120,120));
  if(button->WSpec.button.haspixmap==GR_FALSE && button->WSpec.button.face.caption)
  {
	  GrGetGCTextSize (button->gc, button->WSpec.button.face.caption, -1, GR_TFASCII,&captionwidth, &captionheight, &base);
	  captionx = (winfo.width - captionwidth) / 2;
	  captiony = (winfo.height - captionheight) / 2;
	  if (captionx < 0)
	      	  captionx = 0;		/*if window height or width < */
	  if (captiony < 0)
	      	  captiony = 0;		/*caption height or width - reset */
	  if (!button->WSpec.button.st_down)
		  GrText (button->wid, button->gc, captionx, captiony,button->WSpec.button.face.caption,-1, GR_TFASCII | GR_TFTOP);
	  else
		  GrText (button->wid, button->gc, captionx + 1, captiony + 1,button->WSpec.button.face.caption,-1, GR_TFASCII | GR_TFTOP);
	}
if(button->WSpec.button.haspixmap==GR_TRUE)
{
	 
  if (!button->WSpec.button.st_down)
	  GrDrawImageToFit(button->wid,button->gc,5,5,winfo.width-10,winfo.height-10,button->WSpec.button.face.imageid);
  else
	  GrDrawImageToFit(button->wid,button->gc,6,6,winfo.width-9,winfo.height-9,button->WSpec.button.face.imageid);
}
  return;
}

void DestroyButton(TN_WIDGET *widget)
{
	if(widget->WSpec.button.haspixmap==GR_TRUE)
		GrFreeImage(widget->WSpec.button.face.imageid);
	free(widget->WSpec.button.face.caption);
	DeleteFromRegistry(widget);
	return;
}

int tnGetButtonPressed(TN_WIDGET *widget)
{
	if(widget->type != TN_BUTTON)
		return -1;
	return widget->WSpec.button.changebuttons;
}

int tnSetButtonPixmap(TN_WIDGET *widget,char *filename)
{
	if(widget->type != TN_BUTTON)
		return -1;
	if(filename == NULL)
		return -1;
	if(widget->WSpec.button.haspixmap == GR_TRUE)
		GrFreeImage(widget->WSpec.button.face.imageid);
	widget->WSpec.button.face.imageid = GrLoadImageFromFile(filename,0);
	if(widget->WSpec.button.face.caption)
		free (widget->WSpec.button.face.caption);
	widget->WSpec.button.face.caption=strdup(filename);
		
	widget->WSpec.button.haspixmap = GR_TRUE;
	GrClearWindow(widget->wid,GR_FALSE);
	DrawButton(widget);
	return 1;
}

int tnRemoveButtonPixmap(TN_WIDGET *widget)
{
	if(widget->type != TN_BUTTON)
		return -1;
	if(!widget->WSpec.button.haspixmap)
		return -1;
	GrFreeImage(widget->WSpec.button.face.imageid);
	widget->WSpec.button.haspixmap = GR_FALSE;
	widget->WSpec.button.face.imageid = 0;
	if(widget->WSpec.button.face.caption)
		free (widget->WSpec.button.face.caption);
	widget->WSpec.button.face.caption=strdup("Button");
	GrClearWindow(widget->wid,GR_FALSE);
	DrawButton(widget);
	return 1;
}

int tnSetButtonCaption(TN_WIDGET *widget,char *caption)
{
	if(widget->type != TN_BUTTON)
		return -1;
	if(widget->WSpec.button.haspixmap)
		return 0;
	if(widget->WSpec.button.face.caption)
		free(widget->WSpec.button.face.caption);
	widget->WSpec.button.face.caption=strdup(caption);
	GrClearWindow(widget->wid,GR_TRUE);	
	return 1;
}

int tnGetButtonCaption(TN_WIDGET *widget,char *caption)
{
        if(widget->type != TN_BUTTON)
                return -1;
        strcpy(caption,widget->WSpec.button.face.caption);
	return 1;
}

