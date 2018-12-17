 /*C Source file for the Radio Button widget
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

/* Create a Radio Button And Initialize all variables */
void
CreateRadioButton (TN_WIDGET *widget,
				   TN_WIDGET *parent,
				   int posx,
				   int posy,
				   char *caption,
				   GR_SIZE height,
				   GR_SIZE width,
				   char *fontname,
				   GR_SIZE fontsize,
				   GR_COLOR bgcolor,
				   GR_COLOR fgcolor)

{
  /* Initialize Font To be used */
  GR_FONT_ID font;
  if(strcmp(fontname,""))
	  font=GrCreateFont(fontname,fontsize,NULL);
  else 
	  font=TN_DEFAULT_FONT_NO;
  GrSetGCFont(widget->gc,font);

  /* Initialize Caption */
  if(caption!=NULL)
	 widget->WSpec.radiobutton.caption=strdup(caption);
  else
	  widget->WSpec.radiobutton.caption=strdup("Radio Button");

  /* Store The Foreground color*/
  widget->WSpec.radiobutton.FGColor=fgcolor;

  /* Take Default Parameters, if not specified */
  if(height==0) 
	  height=TN_RADIOBUTTON_HEIGHT;
  if(width==0)
	  width=TN_RADIOBUTTON_WIDTH;

  /* Initialize the state of the radio button as not active */
  widget->WSpec.radiobutton.st_down=GR_FALSE;
  
  /* Create a window for the radio button */
  widget->wid=GrNewWindow(parent->wid,posx,posy,width,height,0,bgcolor,0);
  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_BUTTON_UP| GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_EXPOSURE);
  return;
}

void 
RadioButtonEventHandler(GR_EVENT *event,TN_WIDGET *widget)
{
	GR_WINDOW_INFO winfo;
	TN_WIDGET *sibling,*parent;
	GR_WINDOW_ID siblingwid,parentwid;
	switch(event->type)
	{
		case GR_EVENT_TYPE_BUTTON_UP:
			break;
		case GR_EVENT_TYPE_BUTTON_DOWN:
			if(widget->enabled==GR_FALSE) break;
			/* Set the focus to the radio button */
			GrSetFocus(widget->wid);

			/* If the radio button was already active then ignore the event */
			if(widget->WSpec.radiobutton.st_down==GR_TRUE)
				break;

			/* Find the parent (Radio Button Group) for the radio button */
			GrGetWindowInfo(widget->wid,&winfo);
			parentwid = winfo.parent;

			/* Get the parent's first sibling */
			GrGetWindowInfo(parentwid,&winfo);
			siblingwid = winfo.child;

			/* Search for the sibling(radio button) which was previously  
			 * active and deactivate it.
			 */
			while(siblingwid != 0)
			{
  	             sibling = GetFromRegistry(siblingwid);
			     if(sibling->WSpec.radiobutton.st_down == GR_TRUE)
			     {
  			        sibling->WSpec.radiobutton.st_down = GR_FALSE;
			        GrClearWindow(sibling->wid,GR_FALSE);
				    DrawRadioButton(sibling);
				    break;
			     }
			     GrGetWindowInfo(siblingwid,&winfo);
			     siblingwid = winfo.sibling;
			}

			/* Make the currently selected radio button active and redraw it */
			widget->WSpec.radiobutton.st_down=GR_TRUE;
            GrClearWindow(widget->wid,GR_FALSE);
            DrawRadioButton(widget);

			/* Call the callback function (if any) registered with the parent
			 * i.e. Radio button group
			 */
  			parent = GetFromRegistry(parentwid);
			if(parent->WSpec.radiobuttongroup.CallBack[SELECTED].fp)
		           (*(parent->WSpec.radiobuttongroup.CallBack[SELECTED].fp)) (parent, parent->WSpec.radiobuttongroup.CallBack[SELECTED].dptr);
			break;

		case GR_EVENT_TYPE_EXPOSURE:
			DrawRadioButton(widget);
			break;
	}
}


/* Draw routine for Radio Button */
void
DrawRadioButton (TN_WIDGET * radiobutton)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth,captionheight,base; 
  GR_COORD circlex, circley;
  
  GrGetGCTextSize(radiobutton->gc,radiobutton->WSpec.radiobutton.caption,-1,GR_TFASCII,&captionwidth,&captionheight,&base);
  captionheight=captionheight<13?13:captionheight;
  
  GrGetWindowInfo (radiobutton->wid,&winfo);
  
  if(winfo.width==TN_RADIOBUTTON_WIDTH)             
	  GrResizeWindow(radiobutton->wid,captionwidth + 18,captionheight);
 
  GrGetWindowInfo (radiobutton->wid,&winfo);			                  
  circlex=8;
  circley=(winfo.height)/2;

  GrSetGCForeground(radiobutton->gc,GR_RGB(255,255,255));
  GrFillEllipse(radiobutton->wid,radiobutton->gc,circlex,circley,4,4);
  
  GrSetGCForeground(radiobutton->gc,GR_RGB(0,0,0));
  GrArcAngle(radiobutton->wid,radiobutton->gc,circlex,circley,5,5,2880,14400,GR_ARC);	  

  if(radiobutton->WSpec.radiobutton.st_down==GR_TRUE)	
  {
  	  GrSetGCForeground(radiobutton->gc,GR_RGB(0,0,0));
	  GrFillEllipse(radiobutton->wid,radiobutton->gc,circlex,circley,2,2);
  }
  
  GrSetGCForeground(radiobutton->gc,radiobutton->WSpec.radiobutton.FGColor);
  GrText(radiobutton->wid,radiobutton->gc,18,(winfo.height-captionheight)/2,radiobutton->WSpec.radiobutton.caption,strlen(radiobutton->WSpec.radiobutton.caption), GR_TFASCII|GR_TFTOP);
  
  return;
}

void DestroyRadioButton(TN_WIDGET *widget)
{
	free(widget->WSpec.radiobutton.caption);
	DeleteFromRegistry(widget);
	return;
}

int tnSetRadioButtonCaption(TN_WIDGET *widget, char *caption)
{
	if(widget->type!=TN_RADIOBUTTON)
		return -1;
	if(caption!=NULL)
	{
		free(widget->WSpec.radiobutton.caption);
		widget->WSpec.radiobutton.caption=strdup(caption);
		GrClearWindow(widget->wid,GR_TRUE);
	}
	return 1;
}

int tnGetRadioButtonCaption(TN_WIDGET *widget, char *caption)
{
	if(widget->type!=TN_RADIOBUTTON)
		return -1;
	strcpy(caption,widget->WSpec.radiobutton.caption);
	return 1;
}
		
int tnSetRadioButtonStatus(TN_WIDGET *widget,GR_BOOL status)
{
	GR_WINDOW_INFO winfo;
	GR_WINDOW_ID siblingwid;
	TN_WIDGET *sibling;

	if(widget->type!=TN_RADIOBUTTON)
		return -1;
	if(widget->WSpec.radiobutton.st_down!=status)
	{
		if(status==GR_TRUE)
		{
			GrGetWindowInfo(widget->wid,&winfo);
			GrGetWindowInfo(winfo.parent,&winfo);
			siblingwid=winfo.child;
			while(siblingwid!=0)
			{
				sibling=GetFromRegistry(siblingwid);
				if(TN_RADIOBUTTONACTIVE(sibling))
				{
					sibling->WSpec.radiobutton.st_down=GR_FALSE;
					GrClearWindow(siblingwid,GR_TRUE);
				}
				GrGetWindowInfo(siblingwid,&winfo);
				siblingwid=winfo.sibling;
			}
		}
		widget->WSpec.radiobutton.st_down=status;
		GrClearWindow(widget->wid,GR_TRUE);
	}
	return 1;
}
