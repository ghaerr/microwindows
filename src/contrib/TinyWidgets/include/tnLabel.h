 /*Header file for the Static Label widget
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

#ifndef _TNLABEL_H_
#define _TNLABEL_H_
#include "tnBase.h"

#define LABEL_CALLBACKS 1
#define TN_LABEL_HEIGHT 1	/*Default height & width */
#define TN_LABEL_WIDTH 1

typedef struct
{
  char *caption;		/*Label caption */
  GR_COLOR FGColor;		/*Caption color */
  CallBackStruct CallBack[LABEL_CALLBACKS];
}
TN_STRUCT_LABEL;

void
CreateLabel (TN_WIDGET *,
	     TN_WIDGET *,
	     int,
	     int,
	     char *, GR_SIZE, GR_SIZE, char *, GR_SIZE, GR_COLOR, GR_COLOR);

void DrawLabel (TN_WIDGET *);
void LabelEventHandler (GR_EVENT *, TN_WIDGET *);
void DestroyLabel(TN_WIDGET *);
int tnSetLabelCaption(TN_WIDGET *, char *);
int tnGetLabelCaption(TN_WIDGET *, char *);
int tnSetLabelCaptionColor(TN_WIDGET *,TN_COLOR);
TN_COLOR tnGetLabelCaptionColor(TN_WIDGET *);
#endif /*_TNLABEL_H_*/
