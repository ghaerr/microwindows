 /*Header file for the PopUp Menu widget
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

#ifndef _TNPOPUPMENU_H_
#define _TNPOPUPMENU_H_
#include "tnBase.h"

#define TN_POPUPMENU_WIDTH 	15 /*Default popup menu width & height*/
#define TN_POPUPMENU_HEIGHT	15
#define CONTAINER_WIDTH		50
#define CONTAINER_HEIGHT 	1 /*initial container height*/
typedef struct
{
  char caption[20];		/*Caption for the Pop Menu */
  GR_COLOR FGColor;
  GR_COORD lasty;
  GR_BOOL selected;
  GR_WINDOW_ID container;
  GR_BOOL exclusive;
}
TN_STRUCT_POPUPMENU;

void
CreatePopUpMenu (TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      char *, GR_SIZE, GR_SIZE, char *, GR_SIZE, GR_COLOR, GR_COLOR,GR_BOOL);
void PopUpMenuEventHandler (GR_EVENT *, TN_WIDGET *);
void DrawPopUpMenu (TN_WIDGET *);
void ClearAllContainers (TN_WIDGET *);
void ClearCascadeContainer (TN_WIDGET *);
void DestroyPopUpMenu(TN_WIDGET *);
#endif /*_TNPOPUPMENU_H_*/
