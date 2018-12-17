 /*Header file for the Window widget
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

#ifndef _TNWINDOW_H_
#define _TNWINDOW_H_
#define WINDOW_CALLBACKS 2
#include "tnBase.h"
#define TN_WINDOW_HEIGHT 200	/*Default Window width & height */
#define TN_WINDOW_WIDTH 300
typedef struct
{
  GR_COORD clickedx;
  GR_COORD clickedy;
  CallBackStruct CallBack[WINDOW_CALLBACKS];
}
TN_STRUCT_WINDOW;

void
CreateWindow (TN_WIDGET *,
	      TN_WIDGET *, int, int, char *, GR_SIZE, GR_SIZE, GR_COLOR);
void WindowEventHandler (GR_EVENT *, TN_WIDGET *);
void DestroyWindow(TN_WIDGET *);
void tnGetClickedPos(TN_WIDGET *, int * , int *);
int tnGetWindowTitle(TN_WIDGET *, char *);
int tnSetWindowTitle(TN_WIDGET *, char *);
#endif	/*_TNWINDOW_H_*/
