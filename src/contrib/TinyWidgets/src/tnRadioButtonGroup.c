 /*C Source file for the Radio Button Group widget
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

/* Creates a Radio Button Group And Initializes various parameters */
void
CreateRadioButtonGroup (TN_WIDGET *widget,
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
  int i;
  /* Set the Font For the Group */
  GR_FONT_ID font;
  if(strcmp(fontname,""))
	  font=GrCreateFont(fontname,fontsize,NULL);
  else
	  font=TN_DEFAULT_FONT_NO;
  GrSetGCFont(widget->gc,font);
  
  /* Initialize the caption */
  if(caption!=NULL)
	  widget->WSpec.radiobuttongroup.caption=strdup(caption);
  else
	  widget->WSpec.radiobuttongroup.caption=strdup("RadioButtonGroup");
  /* Store The Foreground color */
  widget->WSpec.label.FGColor=fgcolor;

  /* Take Default Height & Width if not specified */
  if(height==0)  
	  height=TN_RADIOBUTTONGROUP_HEIGHT;
  if(width==0)
	  width= TN_RADIOBUTTONGROUP_WIDTH;

  widget->WSpec.radiobuttongroup.clickedposx = -1;
  widget->WSpec.radiobuttongroup.clickedposy = -1;
  
  
  /* Initialize Callbacks */
  for(i=0; i < RADIOBUTTONGROUP_CALLBACKS;i++)
	  widget->WSpec.radiobuttongroup.CallBack[i].fp = NULL;

  /* Create a window amd Select the events that it receives */
  widget->wid=GrNewWindow(parent->wid,posx,posy,width,height,0,bgcolor,0);
  GrSelectEvents (widget->wid, GR_EVENT_MASK_BUTTON_UP|GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_EXPOSURE);

  return;
}

/* Event Handler For a Radio Button Group */
void 
RadioButtonGroupEventHandler(GR_EVENT *event,TN_WIDGET *widget)
{
	switch(event->type)
	{
		case GR_EVENT_TYPE_EXPOSURE:
			DrawRadioButtonGroup(widget);
			break;
		case GR_EVENT_TYPE_BUTTON_UP:
			break;
		case GR_EVENT_TYPE_BUTTON_DOWN:
			widget->WSpec.radiobuttongroup.clickedposx = event->button.x;
			widget->WSpec.radiobuttongroup.clickedposy = event->button.y;
			if (widget->WSpec.radiobuttongroup.CallBack[CLICKED].fp)
				(*(widget->WSpec.radiobuttongroup.CallBack[CLICKED].fp)) (widget,widget->WSpec.radiobuttongroup.CallBack[CLICKED].dptr);
			break;
	}
}

/* Draw Routine for a Radio Button Group */
void
DrawRadioButtonGroup (TN_WIDGET * radiobuttongroup)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionwidth,captionheight,base;
  GR_SIZE captionx,captiony;
	  
  GrGetWindowInfo (radiobuttongroup->wid, &winfo);
  GrGetGCTextSize(radiobuttongroup->gc,radiobuttongroup->WSpec.radiobuttongroup.caption,-1,GR_TFASCII,&captionwidth,&captionheight,&base);
		  
  captionx=3; 
  captiony=0;  
  GrSetGCForeground(radiobuttongroup->gc,radiobuttongroup->WSpec.radiobuttongroup.FGColor);
  
  GrText(radiobuttongroup->wid,radiobuttongroup->gc,captionx,captiony,radiobuttongroup->WSpec.radiobuttongroup.caption,-1,GR_TFASCII|GR_TFTOP);  	  

  /*Draw white border*/
  GrSetGCForeground(radiobuttongroup->gc,GR_RGB(255,255,255));
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,0,captionheight/2,captionx-1,captionheight/2);
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,captionwidth+3,captionheight/2,winfo.width-1,captionheight/2);
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,winfo.width-1,captionheight/2,winfo.width-1,winfo.height-1);
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,winfo.width-1,winfo.height-1,0,winfo.height-1);
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,1,winfo.height-1,1,captionheight/2);

  /*Draw black border*/
  GrSetGCForeground(radiobuttongroup->gc,GR_RGB(0,0,0));
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,0,captionheight/2-1,captionx-1,captionheight/2-1);
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,captionwidth+3,captionheight/2-1,winfo.width-1,captionheight/2-1);
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,winfo.width-2,captionheight/2+1,winfo.width-2,winfo.height-2);
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,winfo.width-2,winfo.height-2,2,winfo.height-2);
  GrLine(radiobuttongroup->wid,radiobuttongroup->gc,0,winfo.height-1,0,captionheight/2-1);
 
 return;
}

void DestroyRadioButtonGroup(TN_WIDGET *widget)
{
	free(widget->WSpec.radiobuttongroup.caption);
	DeleteFromRegistry(widget);
	return;
}

int tnSetRadioButtonGroupCaption(TN_WIDGET *widget, char *caption)
{
	if(widget->type!=TN_RADIOBUTTONGROUP)
		return -1;
	if(caption!=NULL)
	{
		free(widget->WSpec.radiobuttongroup.caption);
		widget->WSpec.radiobuttongroup.caption=strdup(caption);
		GrClearWindow(widget->wid,GR_TRUE);
	}
	return 1 ; 
}

int tnGetRadioButtonGroupCaption(TN_WIDGET *widget, char *caption)
{
	if(widget->type!=TN_RADIOBUTTONGROUP)
		return -1;
	strcpy(caption,widget->WSpec.radiobuttongroup.caption);
	return 1;
}
	
int tnGetRadioButtonGroupClickedPos(TN_WIDGET *widget, int *x,int *y)
{
	if(widget->type!=TN_RADIOBUTTONGROUP)
		return -1;
	*x = widget->WSpec.radiobuttongroup.clickedposx;
	*y = widget->WSpec.radiobuttongroup.clickedposy;
	return 1;
}

TN_WIDGET *tnGetSelectedRadioButton(TN_WIDGET *widget)
{
	GR_WINDOW_ID siblingwid;
	GR_WINDOW_INFO winfo;
	TN_WIDGET *sibling;

	GrGetWindowInfo(widget->wid,&winfo);
	siblingwid=winfo.child;
	while(siblingwid!=0)
	{
		sibling=GetFromRegistry(siblingwid);
		if(TN_RADIOBUTTONACTIVE(sibling))
			return sibling;
		GrGetWindowInfo(siblingwid,&winfo);
		siblingwid=winfo.sibling;
	}
	return NULL;
}
