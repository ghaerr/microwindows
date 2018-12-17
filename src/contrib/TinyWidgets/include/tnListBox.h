 /*Header file for the ListBox widget
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

#ifndef _TNLISTBOX_H_
#define _TNLISTBOX_H_
#define LISTBOX_CALLBACKS 1
#include "tnBase.h"

#define TN_LISTBOX_HEIGHT 15
#define TN_LISTBOX_WIDTH 80
struct listentry{
		  char *string;
		  GR_BOOL selected;
		  struct listentry *next;
		  struct listentry *prev;
};


typedef struct
{
  struct listentry *list;
  struct listentry *last;
  struct listentry *currtop;
  int numitems;
  GR_BOOL resize;
  GR_COLOR FGColor;				/*Color for the listbox text */
  CallBackStruct CallBack[LISTBOX_CALLBACKS];	/*ListBox CLICKED callback */
}
TN_STRUCT_LISTBOX;

void
CreateListBox (TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      GR_SIZE, GR_SIZE, char *, GR_SIZE, GR_COLOR, GR_COLOR,char **,int,GR_BOOL);
void ListBoxEventHandler (GR_EVENT *, TN_WIDGET *);
void DrawListBox (TN_WIDGET *);
struct listentry * NewListEntry(void);
int tnGetSelectedListItems(TN_WIDGET *,char ***,int *);
int tnGetAllListItems(TN_WIDGET *,char ***,int *);
int tnGetSelectedListNum(TN_WIDGET *);
int tnGetSelectedListPos(TN_WIDGET *,int **,int *);
int tnGetListItemPos(TN_WIDGET *, char *);
int tnAddItemToListBox(TN_WIDGET *,char *);
int tnAddItemToListBoxAt(TN_WIDGET *,char *,int);
int tnDeleteItemFromListBox(TN_WIDGET *,char *);
int tnDeleteItemFromListBoxAt(TN_WIDGET *,int);
int tnDeleteSelectedItems(TN_WIDGET *);
int tnDeleteAllItemsFromListBox(TN_WIDGET *);
int tnSetSelectedListItem(TN_WIDGET *,char *,GR_BOOL);
int tnSetSelectedListItemAt(TN_WIDGET *,int,GR_BOOL);
int tnListItemsLineUp(TN_WIDGET *,int);
int tnListItemsLineDown(TN_WIDGET *,int);
int tnGetListTop(TN_WIDGET *);
int tnGetListBoxResize(TN_WIDGET *, GR_BOOL *);
int tnSetListBoxResize(TN_WIDGET *, GR_BOOL );
void DeletePtr(TN_WIDGET *,struct listentry *);
void DestroyListBox(TN_WIDGET *);
#endif /*_TNLISTBOX_H_*/
