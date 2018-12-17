 /*C Source file for the Progress Bar widget
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

/*Create Progress Bar with specified params*/
void
CreateProgressBar (TN_WIDGET * widget,
	     TN_WIDGET * parent,
	     int posx,
	     int posy,
	     GR_SIZE height,
	     GR_SIZE width,
	     char *fontname,
	     GR_SIZE fontsize, GR_COLOR bgcolor, GR_COLOR fgcolor,GR_BOOL discrete,GR_SIZE stepsize)
{
  int i;
  GR_FONT_ID font;
  if(strcmp(fontname,""))
	  font=GrCreateFont (fontname, fontsize, NULL);
  else
	  font=TN_DEFAULT_FONT_NO;

  GrSetGCFont (widget->gc, font);

  widget->WSpec.progressbar.FGColor = fgcolor;
  if (height == 0)
	    height = TN_PROGRESSBAR_HEIGHT;
  if(width==0)
	    width = TN_PROGRESSBAR_WIDTH;
  
  widget->WSpec.progressbar.value=0;
  widget->WSpec.progressbar.discrete=discrete;
  widget->WSpec.progressbar.stepsize=stepsize;
  GrSetGCUseBackground(widget->gc,GR_FALSE);
  /*Create window for progressbar widget */
  for(i=0;i<PROGRESSBAR_CALLBACKS;i++)
	  widget->WSpec.progressbar.CallBack[i].fp=NULL;
    
  widget->wid =  GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);
  GrSelectEvents (widget->wid, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_BUTTON_UP|GR_EVENT_MASK_BUTTON_DOWN);
  return;
}

/*Event Handler for ProgressBar*/

void
ProgressBarEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  switch (event->type)
    {
    case GR_EVENT_TYPE_EXPOSURE:
      DrawProgressBar (widget);
      break;
    case GR_EVENT_TYPE_BUTTON_UP:
      break;
      
    case GR_EVENT_TYPE_BUTTON_DOWN:
      if (widget->WSpec.progressbar.CallBack[CLICKED].fp)
	      (*(widget->WSpec.progressbar.CallBack[CLICKED].fp)) (widget,widget->WSpec.progressbar.CallBack[CLICKED].dptr);
      break;
		
    }
}

/*ProgressBar drawing routine*/

void
DrawProgressBar (TN_WIDGET * progressbar)
{
  GR_WINDOW_INFO winfo;
  GR_SIZE captionx, captiony;
  GR_GC_INFO gcinfo;
  GR_FONT_INFO finfo;
  GR_COORD boxx,boxy;
  GR_SIZE maxwidth;
  int count,i,startx,stepsize=progressbar->WSpec.progressbar.stepsize;
  char s[5];

  GrGetWindowInfo (progressbar->wid, &winfo);
  GrGetGCInfo(progressbar->gc,&gcinfo);
  GrGetFontInfo(gcinfo.font,&finfo);
  
   if(winfo.height<=finfo.height) 
  {
	  GrResizeWindow(progressbar->wid,winfo.width,finfo.height+4);
  	  GrGetWindowInfo (progressbar->wid, &winfo);
  }

  boxx=1;
  boxy=1;
  maxwidth=winfo.width-2;
  GrSetGCForeground(progressbar->gc,progressbar->WSpec.progressbar.FGColor);
  if(progressbar->WSpec.progressbar.discrete==GR_TRUE)
 {
	 count=((progressbar->WSpec.progressbar.value*maxwidth)/100)/(stepsize+2);
	 startx=boxx;
	 for(i=1;i<=count;++i)
	 {
		 GrFillRect(progressbar->wid,progressbar->gc,startx,boxy,stepsize,winfo.height-1);
		 
		 startx+=stepsize+2;
	 }		 
	 GrFillRect(progressbar->wid,progressbar->gc,startx,boxy,((progressbar->WSpec.progressbar.value*maxwidth)/100)-startx,winfo.height-1);
	 
 }
 else
       	 GrFillRect(progressbar->wid,progressbar->gc,boxx,boxy,(progressbar->WSpec.progressbar.value*maxwidth)/100,winfo.height-1); 
  /*Draw border*/
  GrSetGCForeground(progressbar->gc,GR_RGB(0,0,0));
  GrLine(progressbar->wid,progressbar->gc,0,0,winfo.width-1,0);
  GrLine(progressbar->wid,progressbar->gc,0,0,0,winfo.height-1);

  GrSetGCForeground(progressbar->gc,GR_RGB(255,255,255));
  GrLine(progressbar->wid,progressbar->gc,0,winfo.height-1,winfo.width-1,winfo.height-1);
  GrLine(progressbar->wid,progressbar->gc,winfo.width-1,winfo.height-1,winfo.width-1,0);
/*Draw progress text*/
  GrSetGCForeground (progressbar->gc, GR_RGB(0,0,0));
 if(progressbar->WSpec.progressbar.value>=100) 
	 progressbar->WSpec.progressbar.value=100;
	 
 sprintf(s,"%d",progressbar->WSpec.progressbar.value);
 strcat(s,"%");
 
 captiony=(winfo.height-finfo.height)/2;
 captionx=(winfo.width-3*finfo.maxwidth)/2;
  GrText (progressbar->wid, progressbar->gc, captionx, captiony,
	  s, -1, GR_TFASCII | GR_TFTOP);
 
  return;
}

void tnProgressBarUpdate(TN_WIDGET *widget, int percval)
{
	if(widget->type==TN_PROGRESSBAR && widget->enabled)
	{
		widget->WSpec.progressbar.value=percval;
		GrClearWindow(widget->wid,GR_FALSE);
		DrawProgressBar(widget);
	}
	return;
}

void DestroyProgressBar(TN_WIDGET *widget)
{
	DeleteFromRegistry(widget);
	return;
}

int tnGetProgressBarValue(TN_WIDGET *widget)
{
	if(widget->type!=TN_PROGRESSBAR)
		return -1;
	else 
		return widget->WSpec.progressbar.value;	
}


int tnGetProgressBarStepSize(TN_WIDGET *widget)
{
	if(widget->type!=TN_PROGRESSBAR)
		return -1;
	else 
		return widget->WSpec.progressbar.stepsize;
}

int tnSetProgressBarStepSize(TN_WIDGET *widget, int step)
{
	if(widget->type!=TN_PROGRESSBAR || step <= 0 || step > 100 )
		return -1;
	widget->WSpec.progressbar.stepsize = step;
	GrClearWindow(widget->wid,GR_TRUE);
	return 1;
}
	

TN_COLOR tnGetProgressBarFillColor(TN_WIDGET *widget)
{
	if(widget->type!=TN_PROGRESSBAR)
		return -1;
	else
		return widget->WSpec.progressbar.FGColor;
}

int tnSetProgressBarFillColor(TN_WIDGET *widget, TN_COLOR color)
{
	if(widget->type!=TN_PROGRESSBAR || color == -1)
		return -1;
	widget->WSpec.progressbar.FGColor = color;
	GrClearWindow(widget->wid,GR_TRUE);
	return 1;
}

int tnSetProgressBarStyle(TN_WIDGET *widget,TN_BOOL discrete)
{
	if(widget->type!=TN_PROGRESSBAR)
		return -1;
	widget->WSpec.progressbar.discrete = discrete;
	GrClearWindow(widget->wid,GR_TRUE);
	return 1;
}
			
	
		
						
