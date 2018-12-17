 /*C source file for the Check button widget
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

void
CreateCheckButton (TN_WIDGET * widget,
		   TN_WIDGET * parent,
		   int posx,
		   int posy,
		   char *caption,
		   GR_SIZE height,
		   GR_SIZE width,
		   char *fontname,
		   GR_SIZE fontsize, GR_COLOR bgcolor, GR_COLOR fgcolor)
{
  int i;
  GR_FONT_ID font;
  
  if(strcmp(fontname,""))
	  font=GrCreateFont (fontname, fontsize, NULL);
  else
	  font=TN_DEFAULT_FONT_NO;
  GrSetGCFont (widget->gc, font);
  if(caption!=NULL)
	  widget->WSpec.checkbutton.caption=strdup(caption);
  else
	  widget->WSpec.checkbutton.caption=strdup("Check Button");
  widget->WSpec.checkbutton.FGColor = fgcolor;
  if (height == 0)
      height = TN_CHECKBUTTON_HEIGHT;
  if(width == 0)
      width = TN_CHECKBUTTON_WIDTH;
  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);
  for (i = 0; i < CHECKBUTTON_CALLBACKS; i++)
    widget->WSpec.checkbutton.CallBack[i].fp = NULL;	/*Reset callback(s) */
  widget->WSpec.checkbutton.st_down = GR_FALSE;	/*Reset status */
  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_BUTTON_UP|GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_EXPOSURE);
  return;
}

void
CheckButtonEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_UP:
	    break;
	    
    case GR_EVENT_TYPE_BUTTON_DOWN:
      if(!widget->enabled || !widget->visible)
	  break;	
      GrSetFocus (widget->wid);
      if (widget->WSpec.checkbutton.st_down)
	widget->WSpec.checkbutton.st_down = GR_FALSE;
      else
	widget->WSpec.checkbutton.st_down = GR_TRUE;
      GrClearWindow (widget->wid, GR_FALSE);
      DrawCheckButton (widget);

      if (widget->WSpec.checkbutton.CallBack[CLICKED].fp)
	(*(widget->WSpec.checkbutton.CallBack[CLICKED].fp)) (widget,
						       widget->WSpec.
						       checkbutton.CallBack[CLICKED].dptr);	/*Call user callback */

      break;

    case GR_EVENT_TYPE_EXPOSURE:
      if(widget->visible)	
          DrawCheckButton (widget);
      break;
    }
}


void
DrawCheckButton (TN_WIDGET * checkbutton)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth, captionheight, base;
  GR_COORD boxx, boxy;

  GrGetGCTextSize (checkbutton->gc, checkbutton->WSpec.checkbutton.caption,
		   -1, GR_TFASCII, &captionwidth, &captionheight, &base);
  captionheight = captionheight < 13 ? 13 : captionheight;

  GrGetWindowInfo (checkbutton->wid, &winfo);

  if (winfo.width == TN_CHECKBUTTON_WIDTH)
    GrResizeWindow (checkbutton->wid, captionwidth + 18, captionheight);

  GrGetWindowInfo (checkbutton->wid, &winfo);
  boxx = 0;
  boxy = (winfo.height - 13) / 2;
  /*Draw the checkbox */
  GrSetGCForeground (checkbutton->gc, GR_RGB (255, 255, 255));
  GrFillRect (checkbutton->wid, checkbutton->gc, boxx, boxy, 13, 13);

  GrSetGCForeground (checkbutton->gc, GR_RGB (0, 0, 0));
  /*Give it a 3-D look */
  GrLine (checkbutton->wid, checkbutton->gc, boxx, boxy, boxx + 13, boxy);
  GrLine (checkbutton->wid, checkbutton->gc, boxx, boxy, boxx, boxy + 13);

  GrSetGCForeground (checkbutton->gc, GR_RGB (255, 255, 255));

  GrLine (checkbutton->wid, checkbutton->gc, boxx, boxy + 13, boxx + 13,
	  boxy + 13);
  GrLine (checkbutton->wid, checkbutton->gc, boxx + 13, boxy, boxx + 13,
	  boxy + 13);

/*Draw the check mark*/
  /*This is UGLY looking - Change me */
  GrSetGCForeground (checkbutton->gc, 0);
  if (checkbutton->WSpec.checkbutton.st_down)
    {
      GrLine (checkbutton->wid, checkbutton->gc, boxx + 3, boxy + (13 / 2),
	      9 / 2 + boxx, boxy + 10);
      GrLine (checkbutton->wid, checkbutton->gc, boxx + 2, boxy + (13 / 2),
	      9 / 2 + boxx, boxy + 11);
      GrLine (checkbutton->wid, checkbutton->gc, boxx + 4, boxy + 11,
	      boxx + 8, boxy + 2);
      GrLine (checkbutton->wid, checkbutton->gc, boxx + (9 / 2), boxy + 11,
	      boxx + 9, boxy + 2);
    }

  /*Draw the caption */
  if(checkbutton->enabled)
    GrSetGCForeground (checkbutton->gc, checkbutton->WSpec.checkbutton.FGColor);
  else
    GrSetGCForeground (checkbutton->gc, GR_RGB(120,120,120)); 

  GrText (checkbutton->wid, checkbutton->gc, 18,
	  (winfo.height - captionheight) / 2,
	  checkbutton->WSpec.checkbutton.caption,
	  strlen (checkbutton->WSpec.checkbutton.caption),
	  GR_TFASCII | GR_TFTOP);

  return;
}

void DestroyCheckButton(TN_WIDGET *widget)
{
	free(widget->WSpec.checkbutton.caption);
	DeleteFromRegistry(widget);
	return;
}

int tnSetCheckButtonCaption(TN_WIDGET *widget, char *caption)
{
	if(widget->type!=TN_CHECKBUTTON) 
		return -1;
	if(caption!=NULL)
	{
		free(widget->WSpec.checkbutton.caption);
		widget->WSpec.checkbutton.caption=strdup(caption);
		GrClearWindow(widget->wid,GR_TRUE);
	}
	
	return 1;
}

int tnGetCheckButtonCaption(TN_WIDGET *widget, char *caption)
{
	if(widget->type!=TN_CHECKBUTTON) 
		return -1;
	strcpy(caption,widget->WSpec.checkbutton.caption);
	return 1;
}

int tnSetCheckButtonStatus(TN_WIDGET *widget,GR_BOOL status)
{
	if(widget->type!=TN_CHECKBUTTON)
		return -1;
	if(widget->WSpec.checkbutton.st_down!=status)
	{
		widget->WSpec.checkbutton.st_down=status;
		GrClearWindow(widget->wid,GR_TRUE);
	}
	return 1;
}
