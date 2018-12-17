 /*Header file for the Push Button widget
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

#ifndef _TNBUTTON_H_
#define _TNBUTTON_H_
#define BUTTON_CALLBACKS 1
#include "tnBase.h"
#define TN_BUTTON_COLOR GR_RGB(200,200,200)
#define TN_BUTTON_HEIGHT 30
#define TN_BUTTON_WIDTH 60

#define TN_HASPIXMAP(widget) widget->WSpec.button.haspixmap

typedef struct
{
  struct
  {
	  char *caption;		/*Caption/Filename  for the Push Button */
	  GR_IMAGE_ID imageid;
  } face;
  GR_BOOL haspixmap;
  GR_COLOR FGColor;		/*Color for the button text */
  GR_BOOL st_down;		/*Button status */
  GR_BUTTON changebuttons;      /* Buttons which went down or up */ 
  CallBackStruct CallBack[BUTTON_CALLBACKS];	/*Button CLICKED callback */
}
TN_STRUCT_BUTTON;

void
CreateButton (TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      char *, GR_SIZE, GR_SIZE, char *, GR_SIZE, GR_COLOR, GR_COLOR, GR_BOOL,char *);
void ButtonEventHandler (GR_EVENT *, TN_WIDGET *);
void DrawButton (TN_WIDGET *);
void DestroyButton(TN_WIDGET *);
int tnGetButtonPressed(TN_WIDGET *);
int tnSetButtonPixmap(TN_WIDGET *,char *);
int tnRemoveButtonPixmap(TN_WIDGET *);
int tnSetButtonCaption(TN_WIDGET *,char *);
int tnGetButtonCaption(TN_WIDGET *,char *);
#define tnGetButtonFilename tnGetButtonCaption
#endif /*_TNBUTTON_H_*/

