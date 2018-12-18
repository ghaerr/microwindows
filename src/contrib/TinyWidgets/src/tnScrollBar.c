 /*
 * C Source file for the ScrollBar widget
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
/*Create Scroll Bar widget*/
void
CreateScrollBar(TN_WIDGET * widget,
	      TN_WIDGET * parent,
	      int posx,
	      int posy,
	      GR_SIZE height,
	      GR_SIZE width,
	      int orientation,
	      int minval,
	      int maxval,
	      int linestep,
	      int pagestep
	      )
{
  int i,thumbsize;
  GR_COLOR bgcolor=GR_RGB(150,150,150);
  
  widget->WSpec.scrollbar.orientation=orientation;
  widget->WSpec.scrollbar.minval=(minval==-1)?TN_SCROLLBAR_MINVAL:minval;
  widget->WSpec.scrollbar.maxval=(maxval==-1)?TN_SCROLLBAR_MAXVAL:maxval;
  widget->WSpec.scrollbar.pagestep=(pagestep==-1)?TN_SCROLLBAR_PAGESTEP:pagestep;
  widget->WSpec.scrollbar.linestep=(linestep==-1)?TN_SCROLLBAR_LINESTEP:linestep;
  
  widget->WSpec.scrollbar.thumbpos=widget->WSpec.scrollbar.minval;
  widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_NOSCROLL;
  
  if(orientation==TN_VERTICAL)
  {
  	if (height == 0||height<(3*TN_SCROLLBAR_COMPONENT_SIZE)) 
  	    height = TN_SCROLLBAR_VERTICAL_HEIGHT;
  	if(width==0)
  	    width = TN_SCROLLBAR_VERTICAL_WIDTH;
  }
  else
  {
	  if (height == 0)
     		  height = TN_SCROLLBAR_HORIZONTAL_HEIGHT;
 	  if(width==0||width<(3*TN_SCROLLBAR_COMPONENT_SIZE))
     		  width = TN_SCROLLBAR_HORIZONTAL_WIDTH;
  }

  thumbsize=TN_SCROLLBAR_COMPONENT_SIZE-((widget->WSpec.scrollbar.maxval-widget->WSpec.scrollbar.minval-100)*TN_SCROLLBAR_COMPONENT_SIZE)/100;
  if(orientation == TN_VERTICAL)
	  widget->WSpec.scrollbar.thumbsize = (height-40) * thumbsize / 120;
  else
	  widget->WSpec.scrollbar.thumbsize = (width-40) * thumbsize / 120;
  
  if(widget->WSpec.scrollbar.thumbsize<TN_MIN_THUMBSIZE)
	  widget->WSpec.scrollbar.thumbsize=TN_MIN_THUMBSIZE;
  
   

  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);
  bgcolor=GR_RGB(200,200,200);
   
  if(orientation==TN_VERTICAL)
  {
		  widget->WSpec.scrollbar.upleft=
		  GrNewWindow (widget->wid, 0, 0, width, TN_SCROLLBAR_COMPONENT_SIZE, 0, bgcolor, 0);
	  widget->WSpec.scrollbar.downright=
		  GrNewWindow (widget->wid, 0, height-TN_SCROLLBAR_COMPONENT_SIZE, width, TN_SCROLLBAR_COMPONENT_SIZE, 0, bgcolor, 0);

	  widget->WSpec.scrollbar.thumb=
		  GrNewWindow (widget->wid, 0, TN_SCROLLBAR_COMPONENT_SIZE, width,widget->WSpec.scrollbar.thumbsize , 0, bgcolor, 0);
   }
  
  else
  {
	  widget->WSpec.scrollbar.upleft=
		  GrNewWindow (widget->wid, 0, 0, TN_SCROLLBAR_COMPONENT_SIZE,height, 0, bgcolor, 0);
	  widget->WSpec.scrollbar.downright=
		  GrNewWindow (widget->wid, width-TN_SCROLLBAR_COMPONENT_SIZE,0, TN_SCROLLBAR_COMPONENT_SIZE,height, 0, bgcolor, 0);

	  widget->WSpec.scrollbar.thumb=
		  GrNewWindow (widget->wid, TN_SCROLLBAR_COMPONENT_SIZE,0, widget->WSpec.scrollbar.thumbsize,height, 0, bgcolor, 0);

	  
  }
  GrMapWindow(widget->WSpec.scrollbar.upleft);
  GrMapWindow(widget->WSpec.scrollbar.downright);
  GrMapWindow(widget->WSpec.scrollbar.thumb);
 
  for (i = 0; i < SCROLLBAR_CALLBACKS; i++)
    widget->WSpec.scrollbar.CallBack[i].fp = NULL;
  widget->WSpec.scrollbar.st_downright_down=GR_FALSE;
  widget->WSpec.scrollbar.st_thumb_down = GR_FALSE;
  widget->WSpec.scrollbar.st_upleft_down=GR_FALSE;
  
  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_TIMER );
  GrSelectEvents (widget->WSpec.scrollbar.upleft,0);
  GrSelectEvents (widget->WSpec.scrollbar.downright,0);
  GrSelectEvents (widget->WSpec.scrollbar.thumb,0);

  return;
}

/*Event Handler for the Push Button*/
void
ScrollBarEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{

    GR_WINDOW_INFO winfo_thumb,winfo_scrollbar;	
    int x,y;
    int new_mid,new_thumbpos;
    int min = widget->WSpec.scrollbar.minval;
    int max = widget->WSpec.scrollbar.maxval;	
    int range = max - min;	
    int available_pixels;

    switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_DOWN:
      if(!widget->enabled || !widget->visible)
	  break;	
      if(range==0)
	      widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_NOSCROLL;
      else if(event->button.subwid == widget->WSpec.scrollbar.upleft)
      {
	      widget->WSpec.scrollbar.st_upleft_down=GR_TRUE;
	      widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_LINEDOWN;
 	      if((widget->WSpec.scrollbar.thumbpos - widget->WSpec.scrollbar.linestep) >= min)
	         widget->WSpec.scrollbar.thumbpos-=widget->WSpec.scrollbar.linestep;
	      else
		 widget->WSpec.scrollbar.thumbpos = widget->WSpec.scrollbar.minval;
	      widget->WSpec.scrollbar.tid=GrCreateTimer(widget->wid,SCROLLBAR_TIMEOUT);
      }
     
      else if(event->button.subwid == widget->WSpec.scrollbar.downright)
      {
	      widget->WSpec.scrollbar.st_downright_down=GR_TRUE;
	      widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_LINEUP;	      
	      if( (widget->WSpec.scrollbar.thumbpos + widget->WSpec.scrollbar.linestep) <= max)
	      widget->WSpec.scrollbar.thumbpos+=widget->WSpec.scrollbar.linestep;
	      else
		      widget->WSpec.scrollbar.thumbpos = widget->WSpec.scrollbar.maxval;
	      widget->WSpec.scrollbar.tid=GrCreateTimer(widget->wid,SCROLLBAR_TIMEOUT);	      
      }
      else if(event->button.subwid==widget->WSpec.scrollbar.thumb)
      {
	      /*Drag thumb code*/
	      widget->WSpec.scrollbar.st_thumb_down=GR_TRUE;
      }
      
      else if(event->button.subwid == widget->wid)
      {
	      GrGetWindowInfo(widget->wid,&winfo_scrollbar);     
	      GrGetWindowInfo(widget->WSpec.scrollbar.thumb,&winfo_thumb);
	      x=abs(winfo_scrollbar.x-winfo_thumb.x);
	      y=abs(winfo_scrollbar.y-winfo_thumb.y);
	      if((widget->WSpec.scrollbar.orientation == TN_VERTICAL && event->button.y<y)||(widget->WSpec.scrollbar.orientation == TN_HORIZONTAL && event->button.x<x))
	      {
		      widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_PAGEDOWN;
		      if( (widget->WSpec.scrollbar.thumbpos - widget->WSpec.scrollbar.pagestep) >= min)
	   		      widget->WSpec.scrollbar.thumbpos-=widget->WSpec.scrollbar.pagestep;
    		      else
    			      widget->WSpec.scrollbar.thumbpos = min;
		      widget->WSpec.scrollbar.st_pageup=GR_FALSE;
	      }
	      else
	      {
		      widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_PAGEUP;	
		      if( (widget->WSpec.scrollbar.thumbpos + widget->WSpec.scrollbar.pagestep) <= max)
	       		      widget->WSpec.scrollbar.thumbpos+=widget->WSpec.scrollbar.pagestep;
     		      else
		    	      widget->WSpec.scrollbar.thumbpos = max;
		      widget->WSpec.scrollbar.st_pageup=GR_TRUE;
	      }
	      widget->WSpec.scrollbar.tid=GrCreateTimer(widget->wid,SCROLLBAR_TIMEOUT);
	      
      }      
      if(range!=0)
      {
       GrClearWindow (widget->wid, GR_FALSE);
       DrawScrollBar(widget);
      }
      
      if (!widget->WSpec.scrollbar.st_thumb_down && widget->WSpec.scrollbar.CallBack[CLICKED].fp)
	
	  (*(widget->WSpec.scrollbar.CallBack[CLICKED].fp)) (widget,
						    widget->WSpec.scrollbar.
						    CallBack[CLICKED].dptr);
      break;
      
    case GR_EVENT_TYPE_BUTTON_UP:
      if(range == 0 || !widget->enabled || !widget->visible)
		break;	
      widget->WSpec.scrollbar.st_upleft_down=GR_FALSE;
      widget->WSpec.scrollbar.st_downright_down=GR_FALSE;
      widget->WSpec.scrollbar.st_pageup=GR_FALSE;
      GrDestroyTimer(widget->WSpec.scrollbar.tid);
      if(widget->WSpec.scrollbar.st_thumb_down&&event->button.subwid!=widget->WSpec.scrollbar.thumb)
      {
	      /*Calculate new thumbpos based on released location*/
	      widget->WSpec.scrollbar.LastScrollEvent= TN_SCROLL_THUMBMOVE;
	      GrGetWindowInfo(widget->wid,&winfo_scrollbar);
	      if(widget->WSpec.scrollbar.orientation==TN_VERTICAL)
	      {
		      available_pixels=winfo_scrollbar.height-(2*TN_SCROLLBAR_COMPONENT_SIZE) - widget->WSpec.scrollbar.thumbsize;
		      new_mid=event->button.y;
	      }
	      else
	      {
		      available_pixels=winfo_scrollbar.width-(2*TN_SCROLLBAR_COMPONENT_SIZE) - widget->WSpec.scrollbar.thumbsize;
		      new_mid=event->button.x;
	      }
	      
	      new_thumbpos=(new_mid-(widget->WSpec.scrollbar.thumbsize/2)-TN_SCROLLBAR_COMPONENT_SIZE)*(range)/available_pixels+min;

	      if(new_thumbpos > max)
		      new_thumbpos=max;
	      else if(new_thumbpos < min)
	   		      new_thumbpos=min;
	      widget->WSpec.scrollbar.thumbpos=new_thumbpos;
      }     
      
      GrClearWindow(widget->wid,GR_FALSE);
      DrawScrollBar(widget);
      if( widget->WSpec.scrollbar.st_thumb_down && widget->WSpec.scrollbar.CallBack[CLICKED].fp)      
      {
	      widget->WSpec.scrollbar.st_thumb_down=GR_FALSE;
	      DrawThumb(widget);	
	      (*(widget->WSpec.scrollbar.CallBack[CLICKED].fp)) (widget,
						        widget->WSpec.scrollbar.						        CallBack[CLICKED].dptr);
      }
            break;	    
    case GR_EVENT_TYPE_TIMER:
	    if(widget->WSpec.scrollbar.st_upleft_down==GR_TRUE)	
	    {
		    widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_LINEDOWN;
      		    if((widget->WSpec.scrollbar.thumbpos - widget->WSpec.scrollbar.linestep) >= min)
	   		    widget->WSpec.scrollbar.thumbpos-=widget->WSpec.scrollbar.linestep;
      		    else
	   		    widget->WSpec.scrollbar.thumbpos = widget->WSpec.scrollbar.minval;

	    }
	    
	    else if(widget->WSpec.scrollbar.st_downright_down==GR_TRUE)
	    {
		    /*st_downright_down==GR_TRUE*/
      		    widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_LINEUP;
		    if( (widget->WSpec.scrollbar.thumbpos + widget->WSpec.scrollbar.linestep) <= max)
	      		    widget->WSpec.scrollbar.thumbpos+=widget->WSpec.scrollbar.linestep;
      		    else
      			    widget->WSpec.scrollbar.thumbpos = widget->WSpec.scrollbar.maxval;
	    }
	    
	    else 
	    {
		    /*pageup or pagedown*/
		    if(widget->WSpec.scrollbar.st_pageup==GR_FALSE)
      		    {
      			    widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_PAGEDOWN;
      		    if( (widget->WSpec.scrollbar.thumbpos - widget->WSpec.scrollbar.pagestep) >= min)
			    widget->WSpec.scrollbar.thumbpos-=widget->WSpec.scrollbar.pagestep;
		    else
			    widget->WSpec.scrollbar.thumbpos = min;
      		    }
      		    else
      		    {
      			    widget->WSpec.scrollbar.LastScrollEvent=TN_SCROLL_PAGEUP;	
      			    if( (widget->WSpec.scrollbar.thumbpos + widget->WSpec.scrollbar.pagestep) <= max)
      				    widget->WSpec.scrollbar.thumbpos+=widget->WSpec.scrollbar.pagestep;
      			    else
      				    widget->WSpec.scrollbar.thumbpos = max;
      		    }
		    
	    }
	    
	    if(range!=0)
      	    {
	     	    GrClearWindow (widget->wid, GR_FALSE);
	     	    DrawScrollBar(widget);
      	    }
	    
      	    if (widget->WSpec.scrollbar.CallBack[CLICKED].fp)
	  	    (*(widget->WSpec.scrollbar.CallBack[CLICKED].fp)) (widget,
								       widget->WSpec.scrollbar.CallBack[CLICKED].dptr);
	    break;
	    
    case GR_EVENT_TYPE_EXPOSURE:
      	    if(widget->visible)
	   	    DrawScrollBar(widget);
      	    break;
    }
}

void DrawScrollBar(TN_WIDGET *scrollbar)
{
	DrawUpLeft(scrollbar);
	DrawDownRight(scrollbar);
	DrawThumb(scrollbar);
}


/*Upleft Button draw routine*/
void
DrawUpLeft(TN_WIDGET * scrollbar)
{
	GR_WINDOW_ID wid=scrollbar->WSpec.scrollbar.upleft;
	GR_WINDOW_INFO winfo;
	GR_POINT arrow[3];
	
	GrGetWindowInfo(wid,&winfo);
	if (!scrollbar->WSpec.scrollbar.st_upleft_down)
	    	GrSetGCForeground (scrollbar->gc, GR_RGB (255, 255, 255));
      	else
  		GrSetGCForeground (scrollbar->gc, GR_RGB (0, 0, 0));
    	GrLine (wid, scrollbar->gc, 0, 0, winfo.width - 1, 0);
  	GrLine (wid, scrollbar->gc, 0, 0, 0, winfo.height - 1);

	if (!scrollbar->WSpec.scrollbar.st_upleft_down)
	    	GrSetGCForeground (scrollbar->gc, GR_RGB (0, 0, 0));
      	else
  		GrSetGCForeground (scrollbar->gc, GR_RGB (255, 255, 255));
    	GrLine (wid, scrollbar->gc, winfo.width - 1, 0, winfo.width - 1,
			              winfo.height - 1);
  	GrLine (wid, scrollbar->gc, 0, winfo.height - 1, winfo.width - 1,
			                winfo.height - 1);
	
	if(scrollbar->WSpec.scrollbar.orientation==TN_VERTICAL)
	{
		arrow[0].x=winfo.width/2;
		arrow[0].y=5;
		arrow[1].x=arrow[0].x+5;
		arrow[1].y=15;
		arrow[2].x=arrow[0].x-5;
		arrow[2].y=15;
		GrFillPoly(wid,scrollbar->gc,3,arrow);
			
	}
	else
	{
		arrow[0].x=5;
		arrow[0].y=winfo.height/2;
		arrow[1].x=15;
		arrow[1].y=arrow[0].y+5;
		arrow[2].x=15;
		arrow[2].y=arrow[0].y-5;
		GrFillPoly(wid,scrollbar->gc,3,arrow);

	}

}

/*DownRight Button draw routine*/
void
DrawDownRight(TN_WIDGET * scrollbar)
{
	GR_WINDOW_ID wid=scrollbar->WSpec.scrollbar.downright;
	GR_WINDOW_INFO winfo;
	GR_POINT arrow[3];
	
	GrGetWindowInfo(scrollbar->wid,&winfo);
	/*Move this window if scrollbar size changed dynamically!*/
	if(scrollbar->WSpec.scrollbar.orientation==TN_HORIZONTAL)
		GrMoveWindow(wid,winfo.width-TN_SCROLLBAR_COMPONENT_SIZE,0);
	else
		GrMoveWindow(wid,0,winfo.height-TN_SCROLLBAR_COMPONENT_SIZE);
	
			
	
	GrGetWindowInfo(wid,&winfo);
	if (!scrollbar->WSpec.scrollbar.st_downright_down)
	    	GrSetGCForeground (scrollbar->gc, GR_RGB (255, 255, 255));
      	else
  		GrSetGCForeground (scrollbar->gc, GR_RGB (0, 0, 0));
    	GrLine (wid, scrollbar->gc, 0, 0, winfo.width - 1, 0);
  	GrLine (wid, scrollbar->gc, 0, 0, 0, winfo.height - 1);

	if (!scrollbar->WSpec.scrollbar.st_downright_down)
	    	GrSetGCForeground (scrollbar->gc, GR_RGB (0, 0, 0));
      	else
  		GrSetGCForeground (scrollbar->gc, GR_RGB (255, 255, 255));
    	GrLine (wid, scrollbar->gc, winfo.width - 1, 0, winfo.width-1,winfo.height-1);
  	GrLine (wid, scrollbar->gc, 0, winfo.height - 1, winfo.width - 1,
			                winfo.height - 1);
	
	if(scrollbar->WSpec.scrollbar.orientation==TN_VERTICAL)
	{
		arrow[0].x=winfo.width/2;
		arrow[0].y=15;
		arrow[1].x=arrow[0].x+5;
		arrow[1].y=5;
		arrow[2].x=arrow[0].x-5;
		arrow[2].y=5;
		GrFillPoly(wid,scrollbar->gc,3,arrow);
			
	}
	else
	{
		arrow[0].x=15;
		arrow[0].y=winfo.height/2;
		arrow[1].x=5;
		arrow[1].y=arrow[0].y+5;
		arrow[2].x=5;
		arrow[2].y=arrow[0].y-5;
		GrFillPoly(wid,scrollbar->gc,3,arrow);

	}


}

/*Thumb draw routine*/
void
DrawThumb(TN_WIDGET * scrollbar)
{
	GR_WINDOW_ID wid=scrollbar->WSpec.scrollbar.thumb;
	GR_WINDOW_INFO winfo;
	int available_pixels,new_mid;
	int new_x,new_y;
 	if((scrollbar->WSpec.scrollbar.maxval - scrollbar->WSpec.scrollbar.minval)== 0 || !scrollbar->enabled)	
	{
		GrGetWindowInfo(wid, &winfo);
		GrSetGCForeground(scrollbar->gc,GR_RGB(150,150,150));
		GrFillRect(wid,scrollbar->gc,0,0,winfo.width,winfo.height);
		return;
	  }
	GrGetWindowInfo(scrollbar->wid,&winfo);
	
	if(scrollbar->WSpec.scrollbar.orientation==TN_VERTICAL)
	{
		available_pixels=winfo.height-(2*TN_SCROLLBAR_COMPONENT_SIZE)-scrollbar->WSpec.scrollbar.thumbsize;
		if(available_pixels)
		{
			new_mid=(((available_pixels*(scrollbar->WSpec.scrollbar.thumbpos-scrollbar->WSpec.scrollbar.minval))/(scrollbar->WSpec.scrollbar.maxval-scrollbar->WSpec.scrollbar.minval)))+TN_SCROLLBAR_COMPONENT_SIZE+(scrollbar->WSpec.scrollbar.thumbsize/2);
			new_x=0;
			new_y=new_mid-(scrollbar->WSpec.scrollbar.thumbsize/2);
			GrMoveWindow(wid,new_x,new_y);
		}
	}
	else
	{
		available_pixels=winfo.width-(2*TN_SCROLLBAR_COMPONENT_SIZE)-scrollbar->WSpec.scrollbar.thumbsize;
	       	if(available_pixels)
		{
			new_mid=(((available_pixels*(scrollbar->WSpec.scrollbar.thumbpos-scrollbar->WSpec.scrollbar.minval))/(scrollbar->WSpec.scrollbar.maxval-scrollbar->WSpec.scrollbar.minval)))+TN_SCROLLBAR_COMPONENT_SIZE+(scrollbar->WSpec.scrollbar.thumbsize/2);
			new_x=new_mid-(scrollbar->WSpec.scrollbar.thumbsize/2);
			new_y=0;
			GrMoveWindow(wid,new_x,new_y);
     		}	
	}
	GrClearWindow(wid,GR_FALSE);
	GrGetWindowInfo(wid,&winfo);
	
	if(scrollbar->WSpec.scrollbar.st_thumb_down)
	{
		GrSetGCForeground (scrollbar->gc, GR_RGB (0,0,0)); 
		GrFillRect(wid,scrollbar->gc,0,0,winfo.width,winfo.height);
	}

	GrSetGCForeground (scrollbar->gc, GR_RGB (255, 255, 255));
	GrLine (wid, scrollbar->gc, 0, 0, winfo.width - 1, 0);
        GrLine (wid, scrollbar->gc, 0, 0, 0, winfo.height - 1);
	GrSetGCForeground (scrollbar->gc, GR_RGB (0, 0, 0));
	GrLine (wid, scrollbar->gc, winfo.width - 1, 0, winfo.width - 1,                        winfo.height - 1);
        GrLine (wid, scrollbar->gc, 0, winfo.height - 1, 
		winfo.width - 1,winfo.height - 1);

	return;
}

int tnGetLastScrollEvent(TN_WIDGET *widget)
{
	if(widget->type!=TN_SCROLLBAR)
		return -1;

	else return(widget->WSpec.scrollbar.LastScrollEvent);
}

int tnGetThumbPosition(TN_WIDGET *widget)
{
	if(widget->type!=TN_SCROLLBAR)
		return -1;
	else return(widget->WSpec.scrollbar.thumbpos);
}

int tnSetScrollRange(TN_WIDGET *scrollbar,int minval,int maxval)
{
	GR_WINDOW_INFO winfo;
	int width,height,thumbsize;
	if(scrollbar->type!=TN_SCROLLBAR)
		return -1;
	if(maxval < minval)
	  return -1;
	scrollbar->WSpec.scrollbar.minval = minval;
	scrollbar->WSpec.scrollbar.maxval = maxval;
	if(minval>maxval)
		thumbsize=TN_SCROLLBAR_COMPONENT_SIZE-((scrollbar->WSpec.scrollbar.maxval-scrollbar->WSpec.scrollbar.minval-100)*TN_SCROLLBAR_COMPONENT_SIZE)/100;
	else
		thumbsize=TN_SCROLLBAR_COMPONENT_SIZE;
	
	GrGetWindowInfo(scrollbar->wid,&winfo);
	if(scrollbar->WSpec.scrollbar.orientation == TN_VERTICAL)
		scrollbar->WSpec.scrollbar.thumbsize= (winfo.height-40) * thumbsize/120;
	else
		scrollbar->WSpec.scrollbar.thumbsize= (winfo.width-40) * thumbsize/120;
	if(scrollbar->WSpec.scrollbar.thumbsize<TN_MIN_THUMBSIZE)
          scrollbar->WSpec.scrollbar.thumbsize=TN_MIN_THUMBSIZE;
	if(scrollbar->WSpec.scrollbar.thumbpos > maxval)
		scrollbar->WSpec.scrollbar.thumbpos = maxval;
	if(scrollbar->WSpec.scrollbar.thumbpos < minval)
		scrollbar->WSpec.scrollbar.thumbpos = minval;

	if(scrollbar->WSpec.scrollbar.orientation == TN_VERTICAL)
	{
		width = winfo.width;
		height = scrollbar->WSpec.scrollbar.thumbsize;
	}
	else
	{
		width = scrollbar->WSpec.scrollbar.thumbsize;
		height = winfo.height;
	}
	GrResizeWindow(scrollbar->WSpec.scrollbar.thumb,width,height);
	GrClearWindow(scrollbar->wid,GR_FALSE);
	DrawScrollBar(scrollbar);
	return 1;
}

int tnSetThumbPosition(TN_WIDGET *scrollbar,int n)
{
	if(scrollbar->type!=TN_SCROLLBAR)
                return -1;
	if(n < scrollbar->WSpec.scrollbar.minval || n > scrollbar->WSpec.scrollbar.maxval)
		return -1;
	 scrollbar->WSpec.scrollbar.thumbpos = n;
	 GrClearWindow(scrollbar->wid,GR_FALSE);
	 DrawScrollBar(scrollbar);
	 return 1;
}

void DestroyScrollBar(TN_WIDGET *widget)
{
	DeleteFromRegistry(widget);
	return;
}

int tnGetScrollBarOrientation(TN_WIDGET *widget, int *orientation)
{
	if(widget->type!=TN_SCROLLBAR)
		return -1;
	*orientation=widget->WSpec.scrollbar.orientation;
	return 1;
}

int tnSetScrollBarOrientation(TN_WIDGET *widget, int orientation)
{
	GR_WINDOW_INFO winfo;
	int height,width;
	
	if(widget->type!=TN_SCROLLBAR)
		return -1;
	if(orientation==widget->WSpec.scrollbar.orientation)
		return 1;
	
  	GrGetWindowInfo(widget->wid,&winfo);
	height=winfo.height;
	width=winfo.width;
	
	if(orientation==TN_VERTICAL)
  	{
  		if (height == 0||height<(3*TN_SCROLLBAR_COMPONENT_SIZE)) 
  		    height = TN_SCROLLBAR_VERTICAL_HEIGHT;
  		    
		width = TN_SCROLLBAR_VERTICAL_WIDTH;
  	}
  	else
  	{
	     	  height = TN_SCROLLBAR_HORIZONTAL_HEIGHT;
	 	  if(width==0||width<(3*TN_SCROLLBAR_COMPONENT_SIZE))
     			  width = TN_SCROLLBAR_HORIZONTAL_WIDTH;
  	}
	
	GrResizeWindow(widget->wid,width,height);
	
	widget->WSpec.scrollbar.orientation=orientation;
	GrClearWindow(widget->wid,GR_TRUE);
	return 1;
}


int tnGetScrollRange(TN_WIDGET *widget, int *minval,int *maxval)
{
	if(widget->type!=TN_SCROLLBAR)
		return -1;
	*minval=widget->WSpec.scrollbar.minval;
	*maxval=widget->WSpec.scrollbar.maxval;
	return 1;
}

int tnGetScrollStepSizes(TN_WIDGET *widget, int *pagestep,int *linestep)
{
	if(widget->type!=TN_SCROLLBAR)
		return -1;
	*pagestep=widget->WSpec.scrollbar.pagestep;
	*linestep=widget->WSpec.scrollbar.linestep;
	return 1;
}

int tnSetScrollStepSizes(TN_WIDGET *widget, int pagestep,int linestep)
{
	if(widget->type!=TN_SCROLLBAR)
		return -1;
	widget->WSpec.scrollbar.pagestep=pagestep;
	widget->WSpec.scrollbar.linestep=linestep;
	return 1;
}

