 /*C Source file for TinyWidgets
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

#include<stdarg.h>
#include<stdlib.h>
#include "../include/tnWidgets.h"

void
tnMainLoop (void)
{
	GrMainLoop(EventResolveRoutine);
}



void
EventResolveRoutine (GR_EVENT * event)
{
  GR_WINDOW_ID wid;
  TN_WIDGET *WidgetPtr;

  switch (event->type)
    {
    case GR_EVENT_TYPE_ERROR:
      GrDefaultErrorHandler (event);
      break;

    case GR_EVENT_TYPE_NONE:
	case GR_EVENT_TYPE_CHLD_UPDATE:
    default:
      break;

    case GR_EVENT_TYPE_EXPOSURE:
    case GR_EVENT_TYPE_BUTTON_DOWN:
    case GR_EVENT_TYPE_BUTTON_UP:
    case GR_EVENT_TYPE_MOUSE_ENTER:
    case GR_EVENT_TYPE_MOUSE_EXIT:
    case GR_EVENT_TYPE_MOUSE_MOTION:
    case GR_EVENT_TYPE_MOUSE_POSITION:
    case GR_EVENT_TYPE_KEY_DOWN:
    case GR_EVENT_TYPE_KEY_UP:
    case GR_EVENT_TYPE_FOCUS_IN:
    case GR_EVENT_TYPE_FOCUS_OUT:
    case GR_EVENT_TYPE_UPDATE:
    case GR_EVENT_TYPE_CLOSE_REQ:
	case GR_EVENT_TYPE_TIMER:
      wid = event->general.wid;				// check that all above events have valid general.wid value
      WidgetPtr = GetFromRegistry (wid);
      if (WidgetPtr)
      	ProcessEvent(event, WidgetPtr);
      break;
      
    case GR_EVENT_TYPE_FDINPUT:
      break;
    }
  return;
}

void ProcessEvent(GR_EVENT *event,TN_WIDGET *WidgetPtr)
{
	/*I hate switch..cases*/
	if (WidgetPtr->type < MAXWIDGETS)
		(*EventDispatch[WidgetPtr->type]) (event,WidgetPtr);
	return;
}


int
tnRegisterCallBack (TN_WIDGET * widget,
		    USER_EVENT uevent, CallBackFuncPtr fp, DATA_POINTER dptr)
{
  if(widget==NULL) return -1;
  if ((CheckValidUserEvent (widget->type, uevent)) == -1)
    return -1;

  switch (widget->type)
    {
    case TN_WINDOW:
      widget->WSpec.window.CallBack[uevent].fp = fp;
      widget->WSpec.window.CallBack[uevent].dptr = dptr;
      break;

    case TN_BUTTON:
      widget->WSpec.button.CallBack[uevent].fp = fp;
      widget->WSpec.button.CallBack[uevent].dptr = dptr;
      break;

    case TN_CHECKBUTTON:
      widget->WSpec.checkbutton.CallBack[uevent].fp = fp;
      widget->WSpec.checkbutton.CallBack[uevent].dptr = dptr;
      break;
    case TN_LABEL:
      widget->WSpec.label.CallBack[uevent].fp = fp;
      widget->WSpec.label.CallBack[uevent].dptr = dptr;
      break;

    case TN_RADIOBUTTONGROUP:
      widget->WSpec.radiobuttongroup.CallBack[uevent].fp = fp;
      widget->WSpec.radiobuttongroup.CallBack[uevent].dptr = dptr;
      break;
    case TN_TEXTBOX:
      widget->WSpec.textbox.CallBack[uevent].fp = fp;
      widget->WSpec.textbox.CallBack[uevent].dptr = dptr;
      break;
    case TN_SCROLLBAR:
      widget->WSpec.scrollbar.CallBack[uevent].fp = fp;
      widget->WSpec.scrollbar.CallBack[uevent].dptr = dptr;
      break;	  
    case TN_PROGRESSBAR:
      widget->WSpec.progressbar.CallBack[uevent].fp = fp;
      widget->WSpec.progressbar.CallBack[uevent].dptr = dptr;
      break;
    case TN_LISTBOX:
      widget->WSpec.listbox.CallBack[uevent].fp = fp;
      widget->WSpec.listbox.CallBack[uevent].dptr = dptr;
      break;
    case TN_MENUBAR:
      widget->WSpec.menubar.CallBack[uevent].fp = fp;
      widget->WSpec.menubar.CallBack[uevent].dptr = dptr;
      break;

    case TN_MENUITEM:
      widget->WSpec.menuitem.CallBack[uevent].fp = fp;
      widget->WSpec.menuitem.CallBack[uevent].dptr = dptr;
      break;
      
    case TN_COMBOBOX:
      widget->WSpec.combobox.CallBack[uevent].fp = fp;
      widget->WSpec.combobox.CallBack[uevent].dptr = dptr;
      break;
      
    case TN_RAWWIDGET:
      widget->WSpec.rawwidget.CallBack[uevent].fp = fp;
      widget->WSpec.rawwidget.CallBack[uevent].dptr = dptr;
      break;

    case TN_PICTURE:
       widget->WSpec.picture.CallBack[uevent].fp = fp;
       widget->WSpec.picture.CallBack[uevent].dptr = dptr;
       break;
		   
    }
  return 1;
}

TN_WIDGET *
tnAppInitialize (int argc, char **argv)
{
  TN_WIDGET *MainWidget;
  int i;
  AddDestroyFuncs();
  AddEventDispatchFuncs();

  for(i=0; i < HASH_MAX_QUEUES; i++)
  {
	  Registry[i] = (TN_WIDGET *)malloc(sizeof(TN_WIDGET));
	  Registry[i]->next = Registry[i]->prev = NULL;
  }
  GrOpen ();
  MainWidget = (TN_WIDGET *) malloc (sizeof (TN_WIDGET));
  MainWidget->wid = GR_ROOT_WINDOW_ID;
  MainWidget->type=TN_APP_TYPE;
  AddToRegistry (MainWidget);
  TN_DEFAULT_FONT_NO=GrCreateFont(MWFONT_GUI_VAR, 0, NULL);
  return MainWidget;
}

TN_WIDGET *
tnCreateWidget (TN_WIDGET_TYPE type,
		TN_WIDGET * parent,
		int posx, int posy, ...)
{
  va_list ap;
  int currentprop;
  TN_WIDGET *NewWidget;
  GR_SIZE height=0;
  GR_SIZE width=0;
  GR_COLOR bgcolor=GR_RGB(200,200,200);
  GR_COLOR fgcolor=0;
  //GR_COLOR fillcolor=0;
  char fontname[20];
  GR_SIZE fontsize=6;
  char *caption=NULL;
  char *defaulttext=NULL;
  int maxlength = -1;
  int orientation=TN_VERTICAL;
  int minval=-1;
  int maxval=-1;
  int pagestep=-1;
  int linestep=-1;
  char **listitems=NULL;
  int count=0;
  int textboxtype = TN_SINGLE_LINE;
  char *filename = NULL;
  GR_BOOL stretch = GR_FALSE;
  GR_BOOL resize = GR_TRUE;
  GR_BOOL checkable=GR_FALSE;
  GR_BOOL exclusive=GR_FALSE;
  GR_BOOL discrete=GR_FALSE;
  GR_SIZE stepsize=10;
  GR_BOOL visible=GR_TRUE;
  GR_BOOL enabled=GR_TRUE;
  GR_BOOL haspixmap=GR_FALSE;
  if(parent==NULL) 
	return NULL;
  if(CheckValidParent(type,parent)==0)
	  return NULL;
  
  va_start (ap,posy );
 
  if (type >= MAXWIDGETS)
	return NULL;

  NewWidget = (TN_WIDGET *) malloc (sizeof (TN_WIDGET));
  NewWidget->type = type;
  NewWidget->gc = GrNewGC ();
  NewWidget->data=NULL;
  currentprop=va_arg(ap,int);
  strcpy(fontname,"");

 while(currentprop!=TN_END)
 {
	 switch(currentprop)
	 {
		 case TN_HEIGHT:
			 height=va_arg(ap,GR_SIZE);
			 break;
   		 case TN_WIDTH:
			 width=va_arg(ap,GR_SIZE);
			 break;
		 case TN_FONTNAME:
			 strcpy(fontname,va_arg(ap,char*));
			 break;
		 case TN_FONTSIZE:
			 fontsize=va_arg(ap,GR_SIZE);
                         break;
		 case TN_BGCOLOR:
			 bgcolor=va_arg(ap,GR_COLOR);
			 break;
		 case TN_FGCOLOR:
			 fgcolor=va_arg(ap,GR_COLOR);
			 break;
		 case TN_FILLCOLOR:
			 //fillcolor=va_arg(ap,GR_COLOR);
                         break;
		 case TN_CAPTION:
			 caption=va_arg(ap,char *);
			 break;
		 case TN_DEFAULTTEXT:
			 defaulttext=va_arg(ap,char *);
			 break;
		case TN_MAXLENGTH:
			 maxlength=va_arg(ap,int);
			 break;
		case TN_MINVAL:
			 minval=va_arg(ap,int);
			 break;
		case TN_MAXVAL:
			 maxval=va_arg(ap,int);
			 break;
		case TN_LINESTEP:
			 linestep=va_arg(ap,int);
			 break;						  
		case TN_PAGESTEP:
			 pagestep=va_arg(ap,int);
 			 break;						  
		case TN_ORIENTATION:
			 orientation=va_arg(ap,int);
 			 break;						  
		case TN_LISTITEMS:
			 listitems=va_arg(ap,char **);
			 break;
		case TN_COUNT:
			 count=va_arg(ap,int);
			 break;
		case TN_TEXTBOXTYPE:
			 textboxtype = va_arg (ap,int);
    			 break;
		case TN_RESIZE:
			 resize = va_arg (ap,int);
			 break;
		case TN_FILENAME:
			 filename = va_arg (ap, char *);
	    		 break;
		case TN_STRETCH:
			 stretch = va_arg(ap,int);
			 break;					 
		case TN_CHECKABLE:
			 checkable=va_arg(ap,int);
			 break;
		case TN_EXCLUSIVE:
			 exclusive=va_arg(ap,int);
			 break;
		case TN_DISCRETE:
			 discrete=va_arg(ap,int);
			 break;
		case TN_STEPSIZE:
			 stepsize=va_arg(ap,GR_SIZE);
			 break;
		case TN_VISIBLE:
			 visible=va_arg(ap,int);
			 break;
		case TN_ENABLED:
			 enabled=va_arg(ap,int);
			 break;
		case TN_PIXMAP:
			 haspixmap=va_arg(ap,int);
			 break;
	}
	currentprop=va_arg(ap,int);
 }	

  GrSetGCBackground(NewWidget->gc,bgcolor);
  NewWidget->visible=visible;
  NewWidget->enabled=enabled;
  switch (type)
    {
    case TN_BUTTON:
      CreateButton (NewWidget,parent,posx,posy,caption,height,width,fontname,fontsize,bgcolor,fgcolor,haspixmap,filename);
      break;
      
    case TN_WINDOW:
      CreateWindow (NewWidget,parent,posx,posy,caption,height,width,bgcolor);
      break;
      
    case TN_LABEL:
      CreateLabel (NewWidget,parent,posx,posy,caption,height,width,fontname,fontsize,bgcolor,fgcolor);
      break;      
      
    case TN_CHECKBUTTON:
      CreateCheckButton (NewWidget,parent,posx,posy,caption,height,width,fontname,fontsize,bgcolor,fgcolor);
      break;
      
    case TN_RADIOBUTTONGROUP:
      CreateRadioButtonGroup (NewWidget,parent,posx,posy,caption,height,width,fontname,fontsize,bgcolor,fgcolor);
      break;
      
    case TN_RADIOBUTTON:
      CreateRadioButton (NewWidget,parent,posx,posy,caption,height,width,fontname,fontsize,bgcolor,fgcolor);
      break;

    case TN_TEXTBOX:
      CreateTextBox (NewWidget,parent,posx,posy,defaulttext,height,width,fontname,fontsize,bgcolor,fgcolor,maxlength,textboxtype);
      break;		    
      
    case TN_SCROLLBAR:
      CreateScrollBar(NewWidget,parent,posx,posy,height,width,orientation,minval,maxval,linestep,pagestep);
      break;
      
    case TN_PROGRESSBAR:
      CreateProgressBar (NewWidget,parent,posx,posy,height,width,fontname,fontsize,bgcolor,fgcolor,discrete,stepsize);
	    break;
	    
    case TN_LISTBOX:
	    CreateListBox(NewWidget,parent,posx,posy,height,width,fontname,fontsize,bgcolor,fgcolor,listitems,count,resize);
	    break;
	    
    case TN_PICTURE:
	    CreatePicture (NewWidget, parent, posx, posy, filename, height, width,stretch);
	    break;
	    
    case TN_MENUBAR:
	     CreateMenuBar(NewWidget,parent,posx,posy,height,width,bgcolor);
	     break;
	     
    case TN_POPUPMENU:
	     CreatePopUpMenu(NewWidget,parent,posx,posy,caption,height,width,fontname,fontsize,bgcolor,fgcolor,exclusive);
	     break;
	     
    case TN_CASCADEMENU:
             CreateCascadeMenu(NewWidget,parent,posx,posy,caption,height,width,fontname,fontsize,bgcolor,fgcolor,exclusive);
             break;
             
    case TN_MENUITEM:
	     CreateMenuItem(NewWidget,parent,posx,posy,caption,height,width,fontname,fontsize,bgcolor,fgcolor,checkable);
	     break;
	     
    case TN_COMBOBOX:
	    CreateComboBox(NewWidget,parent,posx,posy,height,width,listitems,count,fontname,fontsize,bgcolor,fgcolor);
	    break;
	     
    case TN_RAWWIDGET:
	     CreateRawWidget(NewWidget,parent,posx,posy,height,width,fontname,fontsize,bgcolor,fgcolor);
	     break;
    }
  if(NewWidget->visible==GR_TRUE)
	  GrMapWindow (NewWidget->wid);
  tnSetEnabled(NewWidget,enabled);	  
  AddToRegistry(NewWidget);
  va_end (ap);
  return NewWidget;
}

int
tnDestroyWidget(TN_WIDGET* widget)
{
	GR_WINDOW_INFO winfo,child_winfo;
	GR_WINDOW_ID childwid;
	TN_WIDGET *child;
	if(widget==NULL) return -1;
	GrGetWindowInfo(widget->wid,&winfo);
	childwid=winfo.child;
	while(childwid)
	{
		child=GetFromRegistry(childwid);
		GrGetWindowInfo(childwid,&child_winfo);
		childwid=child_winfo.sibling;
		if(child!=NULL)
			tnDestroyWidget(child);
	}
	GrDestroyWindow(widget->wid);
	GrDestroyGC(widget->gc);
//	DeleteFromRegistry(widget);
	(*Destroy[widget->type]) (widget);
	widget=NULL;
	return 1;
}
	
void tnSetVisible(TN_WIDGET *widget, GR_BOOL visible)
{
	widget->visible=visible;
	if(visible==GR_FALSE)
		GrUnmapWindow(widget->wid);
	else 
		GrMapWindow(widget->wid);
	return;
}

void tnSetEnabled(TN_WIDGET *widget, GR_BOOL enabled)
{
	GR_WINDOW_INFO winfo;
	GR_WINDOW_ID siblingwid;
	TN_WIDGET *sibling;
	widget->enabled=enabled;
	GrClearWindow(widget->wid,GR_TRUE);
//	if(widget->type==TN_RADIOBUTTONGROUP)
//	{
		GrGetWindowInfo(widget->wid,&winfo);
		siblingwid=winfo.child;
		while(siblingwid!=0)
		{
			sibling=GetFromRegistry(siblingwid);
			if(sibling)
			{
			sibling->enabled=enabled;
			GrClearWindow(sibling->wid,GR_TRUE);
			}
			GrGetWindowInfo(siblingwid,&winfo);
			siblingwid=winfo.sibling;
		}
//	}
}

int 
CheckValidUserEvent(TN_WIDGET_TYPE type, USER_EVENT uevent)
{
	switch(type)
	{
		case TN_BUTTON:
			if(uevent>=BUTTON_CALLBACKS)
				return -1;
			break;
			
		case TN_WINDOW:
			if(uevent>=WINDOW_CALLBACKS)
				return -1;
			break;
			
		case TN_LABEL:
			if(uevent>=LABEL_CALLBACKS)
				return -1;
			break;
				
                case TN_CHECKBUTTON:
	                if(uevent>=CHECKBUTTON_CALLBACKS)
		                 return -1;
			break;
			
		case TN_RADIOBUTTONGROUP:
			if(uevent>=RADIOBUTTONGROUP_CALLBACKS) 
                                return -1;
			break;
				
		case TN_RADIOBUTTON:
			return -1;
		case TN_TEXTBOX:
			if(uevent>=TEXTBOX_CALLBACKS)
				return -1;
			break;
		case TN_SCROLLBAR:
			if(uevent>=SCROLLBAR_CALLBACKS)
				return -1;
			break;
		case TN_PROGRESSBAR:
			if(uevent>=PROGRESSBAR_CALLBACKS)
				return -1;
			break;

		case TN_LISTBOX:
			if(uevent>=LISTBOX_CALLBACKS)
	     			return -1;
			break;
		case TN_PICTURE:
			if(uevent>=PICTURE_CALLBACKS)
				return -1;
			break;
		case TN_MENUBAR:
			if(uevent>=MENUBAR_CALLBACKS)
				return -1;
			break;
		case TN_POPUPMENU:
			return -1;
		case TN_MENUITEM:
			if(uevent>=MENUITEM_CALLBACKS)
				return -1;
			break;
                case TN_CASCADEMENU:
                        return -1;
                        
                case TN_COMBOBOX:
	                if(uevent >=COMBOBOX_CALLBACKS)
        	        	return -1;
        	        break;
        	        
		case TN_RAWWIDGET:
			if(uevent>=RAWWIDGET_CALLBACKS) 
	      			return -1;
			break;
	} 
	return 1;
}


void 
AddToRegistry(TN_WIDGET *ptr)
{
	int hindex = ptr->wid % HASH_MAX_QUEUES;
	ptr->next  = Registry[hindex]->next;
	ptr->prev  = Registry[hindex];
	Registry[hindex]->next = ptr;
	if(ptr->next != NULL)
		(ptr->next)->prev = ptr;
	return;
}

TN_WIDGET *
GetFromRegistry(GR_WINDOW_ID wid)
{
	int hindex = wid % HASH_MAX_QUEUES;
	TN_WIDGET *temp = Registry[hindex]->next;
	while(temp && temp->wid != wid)
		temp=temp->next;
	return temp;
}

void
DeleteFromRegistry(TN_WIDGET *ptr)
{
	TN_WIDGET *temp;
	if(ptr == NULL) return;
	temp = ptr->prev;
	temp->next = ptr->next;
	if(ptr->next != NULL)
		(ptr->next)->prev = temp;
	ptr->next = NULL;
	ptr->prev = NULL;
	free(ptr);
	ptr = NULL;
	return;
}

void AddDestroyFuncs(void)
{
	Destroy[TN_WINDOW]=DestroyWindow;
  	Destroy[TN_BUTTON]=DestroyButton;
    	Destroy[TN_LABEL]=DestroyLabel;
	Destroy[TN_CHECKBUTTON]=DestroyCheckButton;
  	Destroy[TN_RADIOBUTTONGROUP]=DestroyRadioButtonGroup;
	Destroy[TN_RADIOBUTTON]=DestroyRadioButton;
	Destroy[TN_TEXTBOX]=DestroyTextBox;
	Destroy[TN_SCROLLBAR]=DestroyScrollBar;	
	Destroy[TN_PROGRESSBAR]=DestroyProgressBar;
	Destroy[TN_LISTBOX]=DestroyListBox;
	Destroy[TN_PICTURE]=DestroyPicture;
	Destroy[TN_MENUBAR]=DestroyMenuBar;
	Destroy[TN_POPUPMENU]=DestroyPopUpMenu;	 
 	Destroy[TN_CASCADEMENU]=DestroyCascadeMenu;
	Destroy[TN_MENUITEM]=DestroyMenuItem;
	Destroy[TN_RAWWIDGET]=DestroyRawWidget;
	Destroy[TN_COMBOBOX]=DestroyComboBox;
}
		  
void AddEventDispatchFuncs(void)
{
	EventDispatch[TN_WINDOW]=WindowEventHandler;
	EventDispatch[TN_BUTTON]=ButtonEventHandler;
	EventDispatch[TN_LABEL]=LabelEventHandler;
	EventDispatch[TN_CHECKBUTTON]=CheckButtonEventHandler;
	EventDispatch[TN_RADIOBUTTONGROUP]=RadioButtonGroupEventHandler;
	EventDispatch[TN_RADIOBUTTON]=RadioButtonEventHandler;
	EventDispatch[TN_TEXTBOX]=TextBoxEventHandler;
	EventDispatch[TN_SCROLLBAR]=ScrollBarEventHandler;
	EventDispatch[TN_PROGRESSBAR]=ProgressBarEventHandler;
	EventDispatch[TN_LISTBOX]=ListBoxEventHandler;
	EventDispatch[TN_PICTURE]=PictureEventHandler;
	EventDispatch[TN_MENUBAR]=MenuBarEventHandler;
	EventDispatch[TN_POPUPMENU]=PopUpMenuEventHandler;
	EventDispatch[TN_CASCADEMENU]=CascadeMenuEventHandler;
	EventDispatch[TN_MENUITEM]=MenuItemEventHandler;
	EventDispatch[TN_RAWWIDGET]=RawWidgetEventHandler;
	EventDispatch[TN_COMBOBOX]=ComboBoxEventHandler;
}

void tnGetWidgetProps(TN_WIDGET *widget,TN_WIDGET_PROPS *props)
{
	GR_WINDOW_INFO winfo,parentinfo;
	GR_WINDOW_ID sibling,child;
	
	props->type = widget->type;
	GrGetWindowInfo(widget->wid,&winfo);
 	props->height=winfo.height;
	props->width=winfo.width;
	GrGetWindowInfo(winfo.parent,&parentinfo);
	props->posx=winfo.x;		// returns relative coords
	props->posy=winfo.y;
//	props->posx=winfo.x-parentinfo.x;
//	props->posy=winfo.y-parentinfo.y;
	props->parent=GetFromRegistry(winfo.parent);
	sibling = winfo.sibling;
	child = winfo.child;
	while(1)
	{
		props->child=GetFromRegistry(child);
		if(props->child || !child)
			break;
		GrGetWindowInfo(child, &winfo);
		child = winfo.sibling;
		
	}
	while(1)
	{
		props->sibling=GetFromRegistry(sibling);
		if(props->sibling || !sibling)
			break;
		GrGetWindowInfo(sibling, &winfo);
		sibling = winfo.sibling;
	}
	
	return;
}

void tnSetWidgetProps(TN_WIDGET *widget, TN_WIDGET_PROPS *props)
{
	GR_WINDOW_INFO winfo,parentinfo;
	GrGetWindowInfo(widget->wid,&winfo);
	if(winfo.height!=props->height || winfo.width!=props->width)
		GrResizeWindow(widget->wid,props->width,props->height);
	GrGetWindowInfo(winfo.parent,&parentinfo);
	if((parentinfo.x-winfo.x)!=props->posx || (parentinfo.y-winfo.y)!=props->posy)
		GrMoveWindow(widget->wid,props->posx,props->posy);
	GrClearWindow(widget->wid,GR_TRUE);
	return;
}
	
void tnAttachData(TN_WIDGET *widget,DATA_POINTER p)
{
	widget->data=p;
	return;
}

DATA_POINTER tnGetAttachedData(TN_WIDGET *widget)
{
	return(widget->data);	
}

int CheckValidParent(int child,TN_WIDGET *parent_widget)
{
	int parent_type=parent_widget->type;
	
	switch(child)
	{
		case TN_WINDOW:
			return(parent_type==TN_APP_TYPE);
		case TN_BUTTON:
		case TN_SCROLLBAR:
		case TN_TEXTBOX:
		case TN_LISTBOX:
			return(parent_type==TN_WINDOW || parent_type==TN_COMBOBOX);
		case TN_LABEL:
		case TN_CHECKBUTTON:
		case TN_RADIOBUTTONGROUP:
		case TN_PICTURE:
		case TN_COMBOBOX:
		case TN_RAWWIDGET:
		case TN_PROGRESSBAR:
			return(parent_type==TN_WINDOW);
		case TN_RADIOBUTTON:
			return(parent_type==TN_RADIOBUTTONGROUP);
		case TN_MENUBAR:
			return (parent_type==TN_WINDOW);
		case TN_POPUPMENU:
			return (parent_type==TN_MENUBAR || parent_type==TN_POPUPMENU || parent_type==TN_WINDOW);
		case TN_CASCADEMENU:
			return (parent_type==TN_POPUPMENU || parent_type==TN_CASCADEMENU);  
		case TN_MENUITEM:
			return (parent_type==TN_MENUBAR || parent_type==TN_POPUPMENU || parent_type==TN_CASCADEMENU);
		default:
			return 1;
	}
}

void tnEndApp(void)
{
	GrClose();
	exit(0);
}

TN_WIDGET *GetParent(TN_WIDGET *widget)
{
	TN_WIDGET *parent;
	GR_WINDOW_INFO winfo;
	GrGetWindowInfo(widget->wid,&winfo);
	parent=GetFromRegistry(winfo.parent);
	if(!parent)
		return NULL;
	return parent;
}
