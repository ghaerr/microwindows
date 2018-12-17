 /*C Source file for the Static Label widget
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

/*Create Label with specified params*/
void
CreateLabel (TN_WIDGET * widget,
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
  GR_SIZE captionwidth, captionheight, base;
  GR_FONT_ID font;
  
  if(strcmp(fontname,""))
  {
	  font=GrCreateFont (fontname, fontsize, NULL);
	  GrSetGCFont (widget->gc, font);
  }
  else
  {
	  GrSetGCFont (widget->gc,TN_DEFAULT_FONT_NO);
  }

  if(caption!=NULL)
	  widget->WSpec.label.caption=strdup(caption);
  else 
  	  widget->WSpec.label.caption=strdup("Label");
	  
  GrGetGCTextSize (widget->gc, widget->WSpec.label.caption, -1, GR_TFASCII,
		   &captionwidth, &captionheight, &base);
  GrResizeWindow (widget->wid, captionwidth, captionheight);
  height=captionheight;
  width=captionwidth;
  
  
  widget->WSpec.label.FGColor = fgcolor;

  for(i=0;i<LABEL_CALLBACKS;i++)
	  widget->WSpec.label.CallBack[i].fp=NULL;
  
  /*Create window for label widget */
  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);
  GrSelectEvents (widget->wid, GR_EVENT_MASK_EXPOSURE|GR_EVENT_MASK_BUTTON_UP|GR_EVENT_MASK_BUTTON_DOWN);
  return;
}

/*Event Handler for Label*/

void
LabelEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  switch (event->type)
    {
    case GR_EVENT_TYPE_EXPOSURE:
      DrawLabel (widget);
      break;
    case GR_EVENT_TYPE_BUTTON_UP:
      break;
    case GR_EVENT_TYPE_BUTTON_DOWN:
     if (widget->WSpec.label.CallBack[CLICKED].fp)
	     (*(widget->WSpec.label.CallBack[CLICKED].fp)) (widget,
							     widget->WSpec.
							     label.
							     CallBack[CLICKED].
							     dptr);
     break;
      
    }
}

/*Label drawing routine*/

void
DrawLabel (TN_WIDGET * label)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth, captionheight, base;
  GR_SIZE captionx, captiony;
  GrGetWindowInfo (label->wid, &winfo);

  GrSetGCForeground (label->gc, label->WSpec.label.FGColor);
  GrGetGCTextSize (label->gc, label->WSpec.label.caption, -1, GR_TFASCII,
		   &captionwidth, &captionheight, &base);

 if (winfo.height < captionheight || winfo.width < captionwidth)
    {
      GrResizeWindow (label->wid, captionwidth, captionheight);
      GrGetWindowInfo (label->wid, &winfo);
    }				/*Resize the window size to caption height & width
				   else use the values given by the user */

  captionx = (winfo.width - captionwidth) / 2;
  captiony = (winfo.height - captionheight) / 2;

  if (captionx < 0)
    captionx = 0;		/*If caption height or width < window height or */
  if (captiony < 0)
    captiony = 0;		/*width - reset to 0 */

/*Draw caption*/
  GrText (label->wid, label->gc, captionx, captiony,
	  label->WSpec.label.caption, strlen (label->WSpec.label.caption),
	  GR_TFASCII | GR_TFTOP);
  return;
}

void DestroyLabel(TN_WIDGET *widget)
{
	free(widget->WSpec.label.caption);
	DeleteFromRegistry(widget);
	return;
}

int tnSetLabelCaption(TN_WIDGET *widget, char *caption)
{
	if(widget->type!=TN_LABEL) 
		return -1;
	if(caption!=NULL)
	{
		free(widget->WSpec.label.caption);
		widget->WSpec.label.caption=strdup(caption);
	}
	GrClearWindow(widget->wid,GR_TRUE);
	return 1;
}

int tnSetLabelCaptionColor(TN_WIDGET *widget, TN_COLOR color)
{
	if(widget->type!=TN_LABEL)
		return -1;
	widget->WSpec.label.FGColor=color;
	return 1;
}

TN_COLOR tnGetLabelCaptionColor(TN_WIDGET *widget)
{
        if(widget->type!=TN_LABEL)
	                return -1;
	else
		return widget->WSpec.label.FGColor;
}

int tnGetLabelCaption(TN_WIDGET *widget,char *caption)
{
        if(widget->type != TN_LABEL)
                return -1;
        strcpy(caption,widget->WSpec.label.caption);
	return 1;
}
