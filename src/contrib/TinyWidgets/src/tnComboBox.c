 /*C Source file for the Combo Box widget
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
#include "../include/tnTextBox.h"
/*Create Combo Box widget*/
void
CreateComboBox(TN_WIDGET * widget,
	      TN_WIDGET * parent,
	      int posx,
	      int posy,
	      GR_SIZE height,
	      GR_SIZE width,
	      char **strings,
	      int count,
	      char *fontname,
	      GR_SIZE fontsize, GR_COLOR bgcolor, GR_COLOR fgcolor)
{
  int i;
  GR_FONT_INFO finfo;
  GR_FONT_ID font;
  if(strcmp(fontname,""))
	  font=GrCreateFont (fontname, fontsize, NULL);
  else
	  font=TN_DEFAULT_FONT_NO;
  GrSetGCFont (widget->gc, font);
  GrGetFontInfo(font,&finfo);
  widget->WSpec.combobox.listheight = finfo.height * 5;
  if (height == 0) 
      height = TN_COMBOBOX_HEIGHT;
  if(width==0)
      width = TN_COMBOBOX_WIDTH;
  widget->WSpec.combobox.FGColor = fgcolor;
  bgcolor = GR_RGB(255,255,255);

  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);

  for (i = 0; i < COMBOBOX_CALLBACKS; i++)
    widget->WSpec.combobox.CallBack[i].fp = NULL;
  widget->WSpec.combobox.selected = NULL;
  GrSelectEvents (widget->wid, GR_EVENT_MASK_FOCUS_OUT | GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP|GR_EVENT_MASK_EXPOSURE);
  widget->WSpec.combobox.text = tnCreateWidget(TN_TEXTBOX,widget,0,0,TN_WIDTH,width-20,TN_HEIGHT,height,TN_ENABLED,GR_FALSE,TN_FGCOLOR,fgcolor,TN_FONTNAME,fontname,TN_FONTSIZE,fontsize,TN_END);
  widget->WSpec.combobox.button = tnCreateWidget(TN_BUTTON,widget,width-20,0,TN_WIDTH,20,TN_HEIGHT,height,TN_CAPTION,"",TN_END);
  widget->WSpec.combobox.listbox = tnCreateWidget(TN_LISTBOX,widget,0,height,TN_WIDTH,width-20,TN_HEIGHT,widget->WSpec.combobox.listheight,TN_FONTNAME,fontname,TN_FONTSIZE,fontsize,TN_FGCOLOR,fgcolor,TN_RESIZE,GR_FALSE,TN_END);
  widget->WSpec.combobox.scroll = tnCreateWidget(TN_SCROLLBAR,widget,width-20,height,TN_WIDTH,20,TN_HEIGHT,widget->WSpec.combobox.listheight,TN_MINVAL,0,TN_MAXVAL,count<5?0:count-5,TN_END);
   for (i = 0; i < count; i++)
	   tnAddItemToListBox(widget->WSpec.combobox.listbox,strings[i]);
  widget->WSpec.combobox.listvisible = GR_FALSE;
  tnRegisterCallBack(widget->WSpec.combobox.button, CLICKED,comboboxbuttonfunc,widget);
  tnRegisterCallBack(widget->WSpec.combobox.listbox,CLICKED,comboboxlistfunc,widget);
  tnRegisterCallBack(widget->WSpec.combobox.scroll,CLICKED,comboboxscrollfunc,widget);
  return;
}

void
ComboBoxEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{	
	GR_WINDOW_INFO winfo;
	GR_WINDOW_ID wid;
	GR_POINT point[3]; 
	TN_WIDGET *list = widget->WSpec.combobox.listbox;
	TN_WIDGET *text = widget->WSpec.combobox.text;
	TN_WIDGET *scroll = widget->WSpec.combobox.scroll;
	TN_WIDGET *button = widget->WSpec.combobox.button;
	
	switch (event->type)
	{
		case GR_EVENT_TYPE_EXPOSURE:
			GrSetGCForeground(button->gc,GR_RGB(0,0,0));
			GrGetWindowInfo(button->wid,&winfo);
			point[0].x = 8;
			point[1].x = 14;
			point[2].x = 11;
			point[0].y = point[1].y = winfo.height/3;
			point[2].y = 2*winfo.height/3;
			GrFillPoly(button->wid,button->gc,3,point);
			break;
			
		case GR_EVENT_TYPE_BUTTON_DOWN:
			break;
		case GR_EVENT_TYPE_BUTTON_UP:
			break;
		case GR_EVENT_TYPE_FOCUS_OUT:
			wid = GrGetFocus();
			if(wid == button->wid || wid == scroll->wid || wid == text->wid || wid == list->wid||wid == widget->wid)
			{
				GrSetFocus(widget->wid);
				return;
			}
			GrGetWindowInfo(widget->wid,&winfo);
			if(widget->WSpec.combobox.listvisible)
			{
				GrResizeWindow(widget->wid,winfo.width,winfo.height-widget->WSpec.combobox.listheight);
				widget->WSpec.combobox.listvisible = GR_FALSE;
			}
			break;
	}
	return;
}
			      

void comboboxbuttonfunc(TN_WIDGET *button,DATA_POINTER dptr)
{
	GR_WINDOW_INFO winfo;
	TN_WIDGET *widget = (TN_WIDGET *)dptr;
	if(widget->enabled == GR_FALSE)
		return;
	GrGetWindowInfo(widget->wid,&winfo);	 
	GrSetFocus(widget->wid);
	if(!widget->WSpec.combobox.listvisible)
	 { 
		 GrResizeWindow(widget->wid,winfo.width,winfo.height+widget->WSpec.combobox.listheight);
		 GrRaiseWindow(widget->wid);
		 widget->WSpec.combobox.listvisible = GR_TRUE;
	 }
	else
	{
		GrResizeWindow(widget->wid,winfo.width,winfo.height-widget->WSpec.combobox.listheight);
		widget->WSpec.combobox.listvisible = GR_FALSE;
	}
	if(widget->WSpec.combobox.CallBack[CLICKED].fp != NULL)
		                  (*(widget->WSpec.combobox.CallBack[CLICKED].fp)) (widget, widget->WSpec.combobox.CallBack[CLICKED].dptr);
	return;
}

void comboboxlistfunc(TN_WIDGET *listbox, DATA_POINTER dptr)
{
	TN_WIDGET *widget = (TN_WIDGET *)dptr;
	GR_WINDOW_INFO winfo;
	char **items;
	int count,i;
	GrSetFocus(widget->wid);
	tnGetSelectedListItems(listbox, &items, &count);
	if(count == 0)
		return;
	for(i=1;i<count;i++)
		tnSetSelectedListItem(listbox,items[i],GR_FALSE);
	tnSetText(widget->WSpec.combobox.text, items[0]);
	widget->WSpec.combobox.selected = (char *)malloc((strlen(items[0])+1)*sizeof(char));
	strcpy(widget->WSpec.combobox.selected,items[0]);
	if(widget->WSpec.combobox.listvisible)
	{
		GrGetWindowInfo(widget->wid,&winfo);
		GrResizeWindow(widget->wid,winfo.width,winfo.height-widget->WSpec.combobox.listheight);
		widget->WSpec.combobox.listvisible = GR_FALSE;
	}
	if(widget->WSpec.combobox.CallBack[SELECTED].fp != NULL)
		  (*(widget->WSpec.combobox.CallBack[SELECTED].fp)) (widget, widget->WSpec.combobox.CallBack[SELECTED].dptr);
	return;
	
}

void comboboxscrollfunc(TN_WIDGET *scrollbar, DATA_POINTER dptr)
{
	TN_WIDGET *widget=(TN_WIDGET *)dptr;
	int position = tnGetThumbPosition(scrollbar);
	int currtop = tnGetListTop(widget->WSpec.combobox.listbox);
	GrSetFocus(widget->wid);
	if(currtop < position)
		tnListItemsLineUp(widget->WSpec.combobox.listbox,position-currtop);
	if(currtop > position)
		tnListItemsLineDown(widget->WSpec.combobox.listbox,currtop-position);
	return;
	
}

int
tnSetSelectedComboItem(TN_WIDGET *widget,char *item, GR_BOOL selected)
{
	TN_WIDGET *listbox;
	char **list;
	int count,i;
	if(widget->type != TN_COMBOBOX)
			return -1;
	listbox = widget->WSpec.combobox.listbox;
	if(tnSetSelectedListItem(listbox,item,selected) == -1)
		return -1;
	tnGetSelectedListItems(listbox,&list,&count);
	for(i=0;i<count;i++)
		if(strcmp(list[i],item))
			tnSetSelectedListItem(listbox,list[i],GR_FALSE);
	if(selected == GR_TRUE)
	{
		tnSetText(widget->WSpec.combobox.text,item);
		if(widget->WSpec.combobox.selected)
			free(widget->WSpec.combobox.selected);
		widget->WSpec.combobox.selected= strdup(item);
	}
	else
	{
		tnSetText(widget->WSpec.combobox.text,"");
		if(widget->WSpec.combobox.selected)
			free(widget->WSpec.combobox.selected);
		widget->WSpec.combobox.selected = NULL;
	}
	return 1;
}

char * tnGetSelectedComboItem(TN_WIDGET *widget)
{
	char *str;
	if(widget->type != TN_COMBOBOX)
		return NULL;
	if(widget->WSpec.combobox.selected == NULL)
		return NULL;
	str = (char *)malloc((strlen(widget->WSpec.combobox.selected)+1)*sizeof(char));
	strcpy(str,widget->WSpec.combobox.selected);
	return str;
}

int tnAddItemToComboBox(TN_WIDGET *widget,char *item)
{
	TN_WIDGET *listbox;
	int count;		
	if(widget->type != TN_COMBOBOX)
		return -1;
	listbox = widget->WSpec.combobox.listbox;
	if(tnAddItemToListBox(widget->WSpec.combobox.listbox,item) == -1)
		return -1;
	count = listbox->WSpec.listbox.numitems;	
	tnSetScrollRange(widget->WSpec.combobox.scroll,0,count<5?0:count-5); 
	return 1;
}

int tnDeleteItemFromComboBox(TN_WIDGET *widget,char *item)
{
	TN_WIDGET *listbox;
	int count;
	if(widget->type != TN_COMBOBOX)
		return -1;
	listbox = widget->WSpec.combobox.listbox;
	if(tnDeleteItemFromListBox(listbox,item) == -1)
		return -1;
	count = listbox->WSpec.listbox.numitems;		
	if(widget->WSpec.combobox.selected && strcmp(widget->WSpec.combobox.selected,item) == 0)
	{
		tnSetText(widget->WSpec.combobox.text,"");
		free(widget->WSpec.combobox.selected);
		widget->WSpec.combobox.selected = NULL;
	}
	tnSetScrollRange(widget->WSpec.combobox.scroll,0,count<5?0:count-5); 
	return 1;
}

int tnDeleteAllItemsFromComboBox(TN_WIDGET *widget)
{
	TN_WIDGET *listbox;
	if(widget->type != TN_COMBOBOX)
		return -1;
	listbox = widget->WSpec.combobox.listbox;
	if(tnDeleteAllItemsFromListBox(listbox) == -1)
		return -1;
	widget->WSpec.combobox.selected = NULL;
	tnSetText(widget->WSpec.combobox.text,"");
	tnSetScrollRange(widget->WSpec.combobox.scroll,0,0); 
	return 1;

}

void 
DestroyComboBox(TN_WIDGET *widget)
{
	if(widget->WSpec.combobox.selected)
		free(widget->WSpec.combobox.selected);
	DeleteFromRegistry(widget);
	return;
}

				
