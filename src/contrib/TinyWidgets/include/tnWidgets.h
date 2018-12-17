 /*Header file for TinyWidgets
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

#ifndef _TNWIDGETS_H_
#define _TNWIDGETS_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define TN_BOOL 	GR_BOOL
#define TN_TRUE		GR_TRUE
#define TN_FALSE	GR_FALSE
#define TN_COORD	GR_COORD
#define TN_SIZE 	GR_SIZE
#define TN_COLOR	GR_COLOR
#define TN_RGB		GR_RGB

#define HASH_MAX_QUEUES 5	/*Maximum number of hash queues for Registry */
#define MAX_EVENTS      12	/*Number of events defined by the GUI */

#define TN_TEXT_COLOR 	0
#define TN_AUTO 	0
/*Definitions for various types widgets provided by Tiny Widgets*/

#define TN_WINDOW 		0
#define TN_BUTTON 		1
#define TN_LABEL  		2
#define TN_CHECKBUTTON 		3
#define TN_RADIOBUTTONGROUP 	4
#define TN_RADIOBUTTON 		5
#define TN_TEXTBOX		6
#define TN_SCROLLBAR		7
#define TN_PROGRESSBAR		8
#define TN_LISTBOX		9
#define TN_PICTURE		10
#define TN_COMBOBOX		11
#define TN_RAWWIDGET 		12
#define TN_MENUBAR		13
#define TN_POPUPMENU		14
#define TN_MENUITEM		15
#define TN_CASCADEMENU		16

#define MAXWIDGETS		17
#define TN_APP_TYPE		18
/*Definitions for widget properties*/
#define TN_CAPTION 	0
#define TN_HEIGHT 	1
#define TN_WIDTH 	2
#define TN_FONTNAME 	3
#define TN_FONTSIZE 	4
#define TN_BGCOLOR 	5
#define TN_FGCOLOR 	6
#define TN_MAXLENGTH    7
#define TN_DEFAULTTEXT  8
#define TN_MINVAL	9
#define TN_MAXVAL	10
#define TN_LINESTEP	11
#define TN_PAGESTEP	12
#define TN_ORIENTATION	13
#define TN_FILLCOLOR	14
#define TN_LISTITEMS	15
#define TN_COUNT	16
#define TN_TEXTBOXTYPE  17
#define TN_RESIZE       18
#define TN_FILENAME     19
#define TN_STRETCH      20
#define TN_CHECKABLE	21
#define TN_EXCLUSIVE	22
#define TN_DISCRETE	23
#define TN_STEPSIZE	24
#define TN_VISIBLE	25
#define TN_ENABLED	26
#define TN_PIXMAP	28
/*End of arg specifier*/
#define TN_END		29

struct widget;
#include "tnBase.h"
#include "tnWindow.h"
#include "tnLabel.h"
#include "tnButton.h"
#include "tnCheckButton.h"
#include "tnRadioButtonGroup.h"
#include "tnRadioButton.h"
#include "tnTextBox.h"
#include "tnScrollBar.h"
#include "tnProgressBar.h"
#include "tnListBox.h"
#include "tnPicture.h"
#include "tnMenuBar.h"
#include "tnPopUpMenu.h"
#include "tnCascadeMenu.h"
#include "tnMenuItem.h"
#include "tnComboBox.h"
#include "tnRawWidget.h"

#define CLICKED 	0
#define CLOSED		1
#define GOTFOCUS 	0
#define LOSTFOCUS 	1
#define MODIFIED 	2
#define SELECTED	1
#define TN_ISVISIBLE(widget) widget->visible
#define TN_ISENABLED(widget) widget->enabled

/*Definition for a Widget*/
struct widget
{
  struct widget *prev;		/*Pointer to the previous widget on hash queue */
  TN_WIDGET_TYPE type;		/*Type of widget */
  GR_WINDOW_ID wid;		/*ID to the corresponding window */
  GR_BOOL enabled;
  GR_BOOL visible;
  GR_GC_ID gc;
  DATA_POINTER data;	/*User Data associated with this widget*/
  /*Widget Specific Definitions */
  union
  {
    TN_STRUCT_WINDOW window;
    TN_STRUCT_BUTTON button;
    TN_STRUCT_LABEL label;    
    TN_STRUCT_CHECKBUTTON checkbutton;
    TN_STRUCT_RADIOBUTTONGROUP radiobuttongroup;
    TN_STRUCT_RADIOBUTTON radiobutton;
    TN_STRUCT_TEXTBOX textbox;
    TN_STRUCT_SCROLLBAR scrollbar;
    TN_STRUCT_PROGRESSBAR progressbar;
    TN_STRUCT_LISTBOX listbox;
    TN_STRUCT_PICTURE picture;
    TN_STRUCT_MENUBAR menubar;
    TN_STRUCT_POPUPMENU popupmenu;
    TN_STRUCT_MENUITEM menuitem;
    TN_STRUCT_CASCADEMENU cascademenu;
    TN_STRUCT_COMBOBOX combobox;
    TN_STRUCT_RAWWIDGET rawwidget;
  }
  WSpec;
  struct widget *next;		/*Pointer to the next widget on the hash queue */
};

typedef struct
{
	TN_WIDGET_TYPE type;
	GR_SIZE height;
	GR_SIZE width;
	GR_COORD posx;
	GR_COORD posy;
	TN_WIDGET *parent;
	TN_WIDGET *child;
	TN_WIDGET *sibling;
} TN_WIDGET_PROPS;

GR_FONT_ID TN_DEFAULT_FONT_NO;
/*A registry is maintained in the form of a hash queue */
TN_WIDGET *Registry[HASH_MAX_QUEUES];
DestroyFuncPtr Destroy[MAXWIDGETS];
EventDispatchFuncPtr EventDispatch[MAXWIDGETS];
		
/*Prototypes for the Application main functions*/
void tnMainLoop (void);
int tnRegisterCallBack (TN_WIDGET *, USER_EVENT, CallBackFuncPtr, DATA_POINTER);
void EventResolveRoutine (GR_EVENT * );
void ProcessEvent(GR_EVENT * ,TN_WIDGET *);
TN_WIDGET *tnAppInitialize (int, char **);
TN_WIDGET *tnCreateWidget (TN_WIDGET_TYPE,TN_WIDGET *,int , int , ...);
int tnDestroyWidget(TN_WIDGET* );
void tnSetVisible(TN_WIDGET *, GR_BOOL);
void tnSetEnabled(TN_WIDGET *, GR_BOOL);
int CheckValidUserEvent(TN_WIDGET_TYPE, USER_EVENT );
void AddToRegistry(TN_WIDGET *);
TN_WIDGET *GetFromRegistry(GR_WINDOW_ID );
TN_WIDGET *GetParent(TN_WIDGET *);
void DeleteFromRegistry(TN_WIDGET *);
void AddDestroyFuncs(void);
void AddEventDispatchFuncs(void);
void tnSetWidgetProps(TN_WIDGET *, TN_WIDGET_PROPS *);
void tnGetWidgetProps(TN_WIDGET *, TN_WIDGET_PROPS *);
void tnAttachData(TN_WIDGET *,DATA_POINTER );
DATA_POINTER tnGetAttachedData(TN_WIDGET *);
int CheckValidParent(int,TN_WIDGET *);
void tnEndApp(void);
#endif /* _TNWIDGETS_H_*/

