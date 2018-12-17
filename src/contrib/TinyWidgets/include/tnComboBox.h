 /*Header file for the Combo Box widget
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

#ifndef _TNCOMBOBOX_H_
#define _TNCOMBOBOX_H_
#define COMBOBOX_CALLBACKS 3
#include "tnBase.h"

#define TN_COMBOBOX_HEIGHT 15 
#define TN_COMBOBOX_WIDTH 80


typedef struct
{
   char *selected;	
   TN_WIDGET *text;
   TN_WIDGET *button;
   TN_WIDGET *listbox;
   TN_WIDGET *scroll;
   GR_COLOR  FGColor;
   GR_BOOL listvisible;
   int     listheight;
   CallBackStruct CallBack[COMBOBOX_CALLBACKS];	/* callback */
}
TN_STRUCT_COMBOBOX;

void
CreateComboBox(TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      GR_SIZE, GR_SIZE, char **, int,char *,GR_SIZE, GR_COLOR, GR_COLOR);
void ComboBoxEventHandler (GR_EVENT *, TN_WIDGET *);
void comboboxlistfunc(TN_WIDGET *, DATA_POINTER);
void comboboxbuttonfunc(TN_WIDGET *, DATA_POINTER);
void comboboxscrollfunc(TN_WIDGET *, DATA_POINTER );
int tnSetSelectedComboItem(TN_WIDGET *,char *,GR_BOOL);
char * tnGetSelectedComboItem(TN_WIDGET *);
int tnAddItemToComboBox(TN_WIDGET *,char *);
int tnDeleteItemFromComboBox(TN_WIDGET *,char *);
int tnDeleteAllItemsFromComboBox(TN_WIDGET *);
void DestroyComboBox(TN_WIDGET *);
#endif /*_TNCOMBOBOX_H_*/
