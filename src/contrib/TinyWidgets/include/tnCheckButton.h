 /*Header file for the Check Button widget
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

#ifndef _TNCHECKBUTTON_H_
#define _TNCHECKBUTTON_H_
#define CHECKBUTTON_CALLBACKS 1
#include "tnBase.h"

#define TN_CHECKBUTTONACTIVE(widget) widget->WSpec.checkbutton.st_down	/*Returns status of checkbutton */
#define TN_CHECKBUTTON_HEIGHT 13	/*Default height & width */
#define TN_CHECKBUTTON_WIDTH 1
typedef struct
{
  char *caption;		/*caption string for Checkbutton */
  GR_COLOR FGColor;		/*text color for caption */
  GR_BOOL st_down;		/*Checkbutton status */
  CallBackStruct CallBack[1];	/*CLICKED callback */
}
TN_STRUCT_CHECKBUTTON;
void CreateCheckButton (TN_WIDGET *,
			TN_WIDGET *,
			int,
			int,
			char *,
			GR_SIZE,
			GR_SIZE, char *, GR_SIZE, GR_COLOR, GR_COLOR);

void CheckButtonEventHandler (GR_EVENT *, TN_WIDGET *);
void DrawCheckButton (TN_WIDGET *);
void DestroyCheckButton(TN_WIDGET *);
int tnSetCheckButtonCaption(TN_WIDGET *,char *);
int tnGetCheckButtonCaption(TN_WIDGET *,char *);
int tnSetCheckButtonStatus(TN_WIDGET *, GR_BOOL);
#endif /*_TNCHECKBUTTON_H_*/
