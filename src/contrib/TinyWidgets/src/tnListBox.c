 /*C Source file for the ListBox widget
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
/*Create ListBox widget*/
void
CreateListBox(TN_WIDGET * widget,
	      TN_WIDGET * parent,
	      int posx,
	      int posy,
	      GR_SIZE height,
	      GR_SIZE width,
	      char *fontname,
	      GR_SIZE fontsize, 
	      GR_COLOR bgcolor, 
	      GR_COLOR fgcolor,
	      char **listitem,
	      int count,
	      GR_BOOL resize)
{
  int i;
  struct listentry *p;
  GR_FONT_ID font;
  if(strcmp(fontname,""))
	  font= GrCreateFont (fontname, fontsize, NULL);
  else
	  font=TN_DEFAULT_FONT_NO;

  GrSetGCFont (widget->gc, font);
  if (height == 0) 
      height = TN_LISTBOX_HEIGHT;
  if(width==0)
      width = TN_LISTBOX_WIDTH;
  
  widget->WSpec.listbox.FGColor = fgcolor;
  widget->WSpec.listbox.resize  = resize;
  
  bgcolor = GR_RGB(255,255,255);
  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);
  widget->WSpec.listbox.list=NULL;
  widget->WSpec.listbox.last=NULL;
   
  for (i = 0; i < LISTBOX_CALLBACKS; i++)
    widget->WSpec.listbox.CallBack[i].fp = NULL;
  for(i=0;i<count;i++)
  {
	  p=NewListEntry();
	  p->string=(char *)malloc((strlen(listitem[i])+1)*sizeof(char));
	  strcpy(p->string,listitem[i]);
	  if(widget->WSpec.listbox.list==NULL)
	  {
		  widget->WSpec.listbox.list=p;
		  widget->WSpec.listbox.last=p;
	  }
	  else
	  {
	   	  widget->WSpec.listbox.last->next=p;
		  p->prev = widget->WSpec.listbox.last;
		  widget->WSpec.listbox.last=p;
	  }		  
  }  
  widget->WSpec.listbox.numitems=count;
  widget->WSpec.listbox.currtop = widget->WSpec.listbox.list;
  GrSetGCUseBackground(widget->gc,GR_FALSE);
	  
  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_BUTTON_UP|GR_EVENT_MASK_BUTTON_DOWN |
		  GR_EVENT_MASK_EXPOSURE);
  return;
}

/*Event Handler for the ListBox*/
void
ListBoxEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
	GR_FONT_INFO finfo;
	GR_GC_INFO gcinfo;
	GR_SCREEN_INFO sinfo;
	struct listentry *p;
	GR_COORD y;
	int i,control,index;
	GrGetScreenInfo(&sinfo);
  	switch (event->type)
    	{
	    case GR_EVENT_TYPE_BUTTON_UP:
		break;
	    case GR_EVENT_TYPE_BUTTON_DOWN:	   
			if(!widget->visible || !widget->enabled)
				break; 
      			y=event->button.y;
			control=event->button.modifiers & MWKMOD_CTRL;
		        GrGetGCInfo(widget->gc,&gcinfo);
		        GrGetFontInfo(gcinfo.font,&finfo);   
	                index=(y-1)/finfo.height;
			if(!control)
				for(p=widget->WSpec.listbox.list;p;p=p->next)
					if(p->selected)
						p->selected=GR_FALSE;
			p=widget->WSpec.listbox.currtop;
      			for(i=0;p;i++,p=p->next)
		        { 
				if(i==index)
				{
					p->selected=GR_TRUE;
					break;
				}
      			}
			GrSetFocus (widget->wid);
		        GrClearWindow (widget->wid, GR_FALSE);
		        DrawListBox (widget);
			if(widget->WSpec.listbox.CallBack[CLICKED].fp)
			      (*(widget->WSpec.listbox.CallBack[CLICKED].fp)) (widget,widget->WSpec.listbox.CallBack[CLICKED].dptr);
      			break;

	    case GR_EVENT_TYPE_EXPOSURE:
	      		DrawListBox (widget);
		        break;
      }
     return;	
}

/*ListBox draw routine*/
void
DrawListBox (TN_WIDGET * listbox)
{
  GR_WINDOW_INFO winfo;
  GR_GC_INFO gcinfo;
  GR_FONT_INFO finfo;
  int i;
  int nitems = listbox->WSpec.listbox.numitems;	
  GR_COORD x,y;
  struct listentry *p;
  if(!listbox->visible)
     return;
  
  GrGetWindowInfo (listbox->wid, &winfo);
  GrGetGCInfo(listbox->gc,&gcinfo);
  GrGetFontInfo(gcinfo.font,&finfo);
  if(listbox->WSpec.listbox.resize == GR_FALSE)
	  nitems = winfo.height/finfo.height;	  
   if(listbox->WSpec.listbox.resize && (nitems*finfo.height)>winfo.height)
   {
	   GrResizeWindow(listbox->wid,winfo.width,(nitems * finfo.height)+4);
	   GrGetWindowInfo (listbox->wid, &winfo);
   }
  if(!listbox->enabled)
   {
	GrSetGCForeground(listbox->gc, GR_RGB(200,200,200));
	GrFillRect(listbox->wid,listbox->gc,0,0,winfo.width,winfo.height);
    }
		   
  /*Draw ListBox borders */
  GrSetGCForeground (listbox->gc, GR_RGB (0, 0, 0));

  GrLine (listbox->wid, listbox->gc, 0, 0, winfo.width - 1, 0);
  GrLine (listbox->wid, listbox->gc, 0, 0, 0, winfo.height - 1);

  GrSetGCForeground (listbox->gc, GR_RGB (255, 255, 255));

  GrLine (listbox->wid, listbox->gc, winfo.width - 1, 0, winfo.width - 1,
	  winfo.height - 1);
  GrLine (listbox->wid, listbox->gc, 0, winfo.height - 1, winfo.width - 1,
	  winfo.height - 1);
  
  for(i=0,x=3,y=2,p=listbox->WSpec.listbox.currtop;p && i<nitems;i++,p=p->next,y+=finfo.height)
  { 
	  if(p->selected)
	  {

   	  	  GrSetGCForeground (listbox->gc, GR_RGB(0,0,255));
		  GrFillRect(listbox->wid,listbox->gc,x-2,y,winfo.width-2,finfo.height);  
		  GrSetGCForeground (listbox->gc, GR_RGB(255,255,255));
	  }
	  else
   	  	  GrSetGCForeground (listbox->gc, listbox->WSpec.listbox.FGColor);
	  GrText(listbox->wid,listbox->gc,x,y,p->string,-1,GR_TFASCII | GR_TFTOP);
  }

  return;
}

struct listentry * NewListEntry(void)
{
	struct listentry *temp=(struct listentry *)malloc(sizeof(struct listentry));
	temp->selected=GR_FALSE;
	temp->next=NULL;	
	temp->prev = NULL;
	return temp;
}

int
tnGetSelectedListItems(TN_WIDGET *widget, char ***listinfo, int *count)
{
	struct listentry *p;
	
	int i;

	if(widget->type != TN_LISTBOX)
		return -1;
	
	*count=tnGetSelectedListNum(widget);
	*listinfo=(char **)malloc((*count) * sizeof(char *));
	for(p=widget->WSpec.listbox.list, i=0 ;p;p=p->next)
		if(p->selected)
		{
			(*listinfo)[i]=(char *)malloc((strlen(p->string) + 1)*sizeof (char));
			strcpy((*listinfo)[i],p->string);
			i++;
		}
	return 1;
}
int
tnGetAllListItems(TN_WIDGET *widget, char ***listinfo, int *count)
{
	struct listentry *p;
	
	int i;
	*count=0;

	if(widget->type != TN_LISTBOX)
		return -1;
	
	*listinfo=(char **)malloc((*count) * sizeof(char *));
	for(p=widget->WSpec.listbox.list, i=0 ;p;p=p->next)
		{
			(*listinfo)[i]=(char *)malloc((strlen(p->string) + 1)*sizeof (char));
			strcpy((*listinfo)[i],p->string);
			i++;
			(*count)++;
		}
	return 1;
}

int tnGetSelectedListNum(TN_WIDGET *widget)
{
	struct listentry *p;
        int count;
	if(widget->type != TN_LISTBOX)
		return -1;
	for(p=widget->WSpec.listbox.list , count=0 ;p;p=p->next)
   		if(p->selected)
			count ++;
	return count;
}
	
int
tnGetSelectedListPos(TN_WIDGET *widget,int **pos,int *count)
{
	struct listentry *p;
	int i,j;
	if(widget->type != TN_LISTBOX)
		return -1;
	 *count=tnGetSelectedListNum(widget);
	 (*pos)=(int *)malloc((*count) * sizeof(int));
	 
	 for(p=widget->WSpec.listbox.list, i=0,j=0 ;p;p=p->next,j++)
		 if(p->selected)
			 (*pos)[i++]=j;
	 
	 return 1;
}	 

int tnGetListItemPos(TN_WIDGET *widget,char *s)
{
	struct listentry *p;
	int i;
	if(widget->type != TN_LISTBOX)
		return -1;
	for(p=widget->WSpec.listbox.list,i=0;p;p=p->next,i++)
		if(!strcmp(p->string,s))
			return i;
	return -1;
}
	
int tnAddItemToListBox(TN_WIDGET *listbox,char *str)
{
	struct listentry *newentry;
	if(listbox->type!=TN_LISTBOX)
		return -1;
	if(str==NULL)
		return -1;
	newentry = NewListEntry();
	newentry->string = (char *)malloc((strlen(str)+1)*sizeof(char));
	strcpy(newentry->string,str);
	if(listbox->WSpec.listbox.last)
	{
		listbox->WSpec.listbox.last->next = newentry;
		newentry->prev = listbox->WSpec.listbox.last;
	}
	else
	{
		listbox->WSpec.listbox.list = newentry;
		listbox->WSpec.listbox.currtop = newentry;
	}
	listbox->WSpec.listbox.last = newentry;
	(listbox->WSpec.listbox.numitems)++;
	GrClearWindow(listbox->wid,GR_TRUE);
	return 1;
}
		
int tnAddItemToListBoxAt(TN_WIDGET *listbox,char *str,int n)
{
	struct listentry *newentry,*p;
	int i = 0;
        if(listbox->type!=TN_LISTBOX)
                return -1;
	if(str==NULL)
                return -1;
	if(n < 0)
		return -1;
        newentry = NewListEntry();
	newentry->string = (char *)malloc((strlen(str)+1)*sizeof(char));
        strcpy(newentry->string,str);
	p = listbox->WSpec.listbox.list;
	while(p && i < n)
	{
		p = p->next;
		i++;
	}
	newentry->next = p;
	newentry->prev = p->prev;
	if(p)
	{
		p->prev = newentry;
		if(p->prev)
			(p->prev)->next = newentry;
		else
			listbox->WSpec.listbox.list = newentry;
	}		
	else
		listbox->WSpec.listbox.last = newentry;
		
	(listbox->WSpec.listbox.numitems)++;
	GrClearWindow(listbox->wid,GR_TRUE);
	return 1;
}

void DeletePtr(TN_WIDGET *listbox,struct listentry *p)
{
	if(p->prev)
		(p->prev)->next = p->next;
	else
		listbox->WSpec.listbox.list = p->next;
	if(p->next)
		(p->next)->prev = p->prev;
	else
		listbox->WSpec.listbox.last = p->prev;
	if(listbox->WSpec.listbox.currtop == p)
	{
		if(p->prev)
			listbox->WSpec.listbox.currtop = p->prev;
		else
			listbox->WSpec.listbox.currtop = p->next;
	}
	p->prev = NULL;
	p->next = NULL;
        free(p);
	(listbox->WSpec.listbox.numitems)--;
	return;
}


int tnDeleteItemFromListBox(TN_WIDGET *listbox,char *str)
{
	struct listentry *p;
        if(listbox->type!=TN_LISTBOX)
                return -1;
        if(str==NULL)
                return -1;
	p = listbox->WSpec.listbox.list;
        while(p && strcmp(p->string,str)!=0)
                p = p->next;
	if(!p)
		return -1;
	DeletePtr(listbox,p);
	GrClearWindow(listbox->wid,GR_TRUE);
	return 1;
}

	
int tnDeleteItemFromListBoxAt(TN_WIDGET *listbox,int n)
{
	struct listentry *p;
        int i;
        if(listbox->type!=TN_LISTBOX)
                return -1;
        if(n < 0)
                return -1;
        for(p = listbox->WSpec.listbox.list,i=0;p && i < n; p=p->next,i++);
	if(!p)
		return -1;
	DeletePtr(listbox,p);
        GrClearWindow(listbox->wid,GR_TRUE);
	return 1;
}

int tnSetSelectedListItemAt(TN_WIDGET *listbox,int n,GR_BOOL selected)
{
	struct listentry *p;
	int i;
	if(listbox->type!=TN_LISTBOX)
		return -1;
	if(n < 0)
		return -1;
	for(p = listbox->WSpec.listbox.list,i=0;p && i < n; p=p->next,i++);
	if(!p)
		return -1;
	p->selected = selected;
	GrClearWindow(listbox->wid,GR_TRUE);
	return 1;
}

	
	

int tnDeleteSelectedItems(TN_WIDGET *listbox)
{
	struct listentry *p,*temp;
        if(listbox->type!=TN_LISTBOX)
                return -1;
	for(p = listbox->WSpec.listbox.list;p ;)
	{	temp = p->next;
		if(p->selected)
		   DeletePtr(listbox,p);
		p = temp;
	}
	GrClearWindow(listbox->wid,GR_TRUE);
	return 1;
}

int tnDeleteAllItemsFromListBox(TN_WIDGET *listbox)
{
        struct listentry *p,*temp;
        if(listbox->type!=TN_LISTBOX)
                return -1;
        for(p = listbox->WSpec.listbox.list;p ;)
        {       temp = p->next;
		p->next = NULL;
		p->prev = NULL;
		free(p);
		p=temp;
        }
	listbox->WSpec.listbox.list = NULL;
	listbox->WSpec.listbox.last = NULL;
	listbox->WSpec.listbox.currtop = NULL;
	listbox->WSpec.listbox.numitems = 0;
        GrClearWindow(listbox->wid,GR_TRUE);
        return 1;
}



int tnSetSelectedListItem(TN_WIDGET *listbox,char *str,GR_BOOL selected)
{
	struct listentry *p;
	if(listbox->type != TN_LISTBOX)
		return -1;
	if(str == NULL)
		return -1;
	p = listbox->WSpec.listbox.list;
	while(p && strcmp(p->string,str)!=0)
		p=p->next;
	if(!p)
		return -1;
	
	p->selected = selected;
	GrClearWindow(listbox->wid,GR_TRUE);
	return 1;
}

int tnListItemsLineUp(TN_WIDGET *listbox,int n)
{
	struct listentry *p;
	int i;
	if(listbox->type != TN_LISTBOX)
		return -1;
	if(listbox->WSpec.listbox.resize == GR_TRUE)
		return -1;
	if(n < 0)
		return -1;
	for(p = listbox->WSpec.listbox.currtop,i=0; p && p->next && i < n;p=p->next,i++); 
	listbox->WSpec.listbox.currtop = p;
	GrClearWindow(listbox->wid,GR_FALSE);
	DrawListBox(listbox);
	return 1;
}

int tnListItemsLineDown(TN_WIDGET *listbox,int n)
{
	struct listentry *p;
	int i;
	if(listbox->type != TN_LISTBOX)
		return -1;
	if(listbox->WSpec.listbox.resize == GR_TRUE)
		return -1;
	if(n < 0)
		return -1;
	for(p = listbox->WSpec.listbox.currtop,i=0; p && p->prev && i < n;p=p->prev,i++); 
	listbox->WSpec.listbox.currtop = p;
	GrClearWindow(listbox->wid,GR_FALSE);
	DrawListBox(listbox);
	return 1;
}

int tnGetListTop(TN_WIDGET *listbox)
{
	struct listentry *p;
	int i;
	if(listbox->type != TN_LISTBOX)
		return -1;
	if (listbox->WSpec.listbox.currtop == NULL)			// fix playlist.c demo
		return -1;
	for(p = (listbox->WSpec.listbox.currtop)->prev,i=0;p;i++,p=p->prev);
	return i;
}

int tnGetListBoxResize(TN_WIDGET *listbox,GR_BOOL *resize)
{
	if(listbox->type != TN_LISTBOX)
		return -1;
	*resize = listbox->WSpec.listbox.resize;
	return 1;
}

int tnSetListBoxResize(TN_WIDGET *listbox, GR_BOOL resize)
{
	if(listbox->type != TN_LISTBOX)
		return -1;
	listbox->WSpec.listbox.resize = resize;
	return 1;
}
				
		

void DestroyListBox(TN_WIDGET *widget)
{
	struct listentry *temp=widget->WSpec.listbox.list;
	struct listentry *prev=temp;
	while(temp!=NULL)
	{
		prev=temp;
		temp=temp->next;
		free(prev->string);
		free(prev);
		prev=NULL;
	}
	DeleteFromRegistry(widget);
	return;
}

