 /*Header file for the Radio Button Group widget
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

#ifndef _TNRADIOBUTTONGROUP_H_
#define _TNRADIOBUTTONGROUP_H_

#define RADIOBUTTONGROUP_CALLBACKS 2  
#include "tnBase.h"

/* Default Height And Width For A Radio Button Group */
#define TN_RADIOBUTTONGROUP_HEIGHT 50
#define TN_RADIOBUTTONGROUP_WIDTH 100

/* Structure For A Radio Button Group Object */ 
typedef struct
{
  char *caption;
  GR_COLOR FGColor;
  int clickedposx;
  int clickedposy;
  CallBackStruct CallBack[RADIOBUTTONGROUP_CALLBACKS];
}
TN_STRUCT_RADIOBUTTONGROUP;


void
CreateRadioButtonGroup (TN_WIDGET *,
	     TN_WIDGET *,
	     int,
	     int,
	     char *,
	     GR_SIZE,
	     GR_SIZE,
	     char *,
	     GR_SIZE,
	     GR_COLOR,
	     GR_COLOR);

void DrawRadioButtonGroup (TN_WIDGET *);
void RadioButtonGroupEventHandler(GR_EVENT *,TN_WIDGET *);
void DestroyRadioButtonGroup(TN_WIDGET *);	
int tnSetRadioButtonGroupCaption(TN_WIDGET *, char *);
int tnGetRadioButtonGroupCaption(TN_WIDGET *, char *);
int tnGetRadioButtonGroupClickedPos(TN_WIDGET *, int *,int *);
TN_WIDGET *tnGetSelectedRadioButton(TN_WIDGET *);
#endif /*_TNRADIOBUTTONGROUP_H_*/
