 /*Header file for the  Cascade Menu widget
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

#ifndef _TNCASCADEMENU_H_
#define _TNCASCADEMENU_H_
#include "tnBase.h"

#define TN_CASCADEMENU_WIDTH 	15 /*Default cascade menu width & height*/
#define TN_CASCADEMENU_HEIGHT	15
#define CONTAINER_WIDTH		50
#define CONTAINER_HEIGHT 	1 /*initial container height*/
typedef struct
{
  char caption[20];		/*Caption for the Pop Menu */
  GR_COLOR FGColor;
  GR_COORD lasty;
  GR_BOOL selected;
  GR_WINDOW_ID popupmenuwid;
  GR_WINDOW_ID container;
  GR_BOOL exclusive;
}
TN_STRUCT_CASCADEMENU;

void
CreateCascadeMenu (TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      char *, GR_SIZE, GR_SIZE, char *, GR_SIZE, GR_COLOR, GR_COLOR,GR_BOOL);
void CascadeMenuEventHandler (GR_EVENT *, TN_WIDGET *);
void DrawCascadeMenu (TN_WIDGET *);
void DestroyCascadeMenu(TN_WIDGET *);
#endif /*_TNCASCADEMENU_H_*/
