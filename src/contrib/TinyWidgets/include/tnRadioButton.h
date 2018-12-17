 /*Header file for the Radio Button widget
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

#ifndef _TNRADIOBUTTON_H_
#define _TNRADIOBUTTON_H_
#include "tnBase.h"

#define RADIOBUTTON_CALLBACKS   0
#define TN_RADIOBUTTON_HEIGHT  13
#define TN_RADIOBUTTON_WIDTH    1

/* Macro to chaeck if the Radio Button is Active */
#define TN_RADIOBUTTONACTIVE(widget) widget->WSpec.radiobutton.st_down 

/* Structure to store data for a Radiobutton Object */
typedef struct
{
	  char *caption;
	  GR_COLOR FGColor;
	  GR_BOOL st_down;
}
TN_STRUCT_RADIOBUTTON;

void CreateRadioButton (TN_WIDGET *,
  	                TN_WIDGET *,
	    		int ,
			int ,
			char *,
			GR_SIZE,
			GR_SIZE,
    			char *,
			GR_SIZE,
			GR_COLOR,
			GR_COLOR);

void RadioButtonEventHandler(GR_EVENT *,TN_WIDGET *);
void DrawRadioButton (TN_WIDGET *);
void DestroyRadioButton(TN_WIDGET *);
int tnSetRadioButtonCaption(TN_WIDGET *,char *);
int tnGetRadioButtonCaption(TN_WIDGET *, char *);
int tnSetRadioButtonStatus(TN_WIDGET *,GR_BOOL);
#endif /*_TNRADIOBUTTON_H_*/

