 /*Header file for the Menu Item widget
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

#ifndef _TNMENUITEM_H_
#define _TNMENUITEM_H_
#define MENUITEM_CALLBACKS 1
#include "tnBase.h"

#define TN_MENUITEM_HEIGHT 	15
#define TN_MENUITEM_WIDTH	15
#define TN_ISCHECKED(widget)	return widget->WSpec.menuitem.selected?1:0
typedef struct
{
  char caption[20];		/*Caption for the Push Button */
  GR_COLOR FGColor;		/*Color for the button text */
  GR_BOOL selected;
  GR_WINDOW_ID popupmenuwid;
  GR_BOOL checkable;
  CallBackStruct CallBack[MENUITEM_CALLBACKS];	/*Menu Item CLICKED callback */
}
TN_STRUCT_MENUITEM;

void
CreateMenuItem (TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      char *, GR_SIZE, GR_SIZE, char *, GR_SIZE, GR_COLOR, GR_COLOR,GR_BOOL);
void MenuItemEventHandler (GR_EVENT *, TN_WIDGET *);
void DrawMenuItem (TN_WIDGET *);
void UnmapAllContainers (TN_WIDGET *);
void UnmapForwardContainers(GR_WINDOW_ID);
void DestroyMenuItem(TN_WIDGET *);
#endif /*_TNMENUITEM_H_*/
