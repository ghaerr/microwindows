 /*Header file for the Progress Bar widget
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

#ifndef _TNPROGRESSBAR_H_
#define _TNPROGRESSBAR_H_
#include "tnBase.h"

#define TN_PROGRESSBAR_HEIGHT 15	/*Default height & width */
#define TN_PROGRESSBAR_WIDTH 100
#define PROGRESSBAR_CALLBACKS 1
#define ISDISCRETE(widget) widget->WSpec.progressbar.discrete

typedef struct
{
	GR_COLOR FGColor; /*Fill color for progress bar*/
	int value; /*Current percentage value*/
	GR_SIZE stepsize;
	GR_BOOL discrete;
	CallBackStruct CallBack[PROGRESSBAR_CALLBACKS];
}
TN_STRUCT_PROGRESSBAR;

void
CreateProgressBar (TN_WIDGET *,
	     TN_WIDGET *,
	     int,
	     int,
	     GR_SIZE, GR_SIZE, char *, GR_SIZE, GR_COLOR, GR_COLOR, GR_BOOL, GR_SIZE);

void DrawProgressBar (TN_WIDGET *);
void ProgressBarEventHandler (GR_EVENT *, TN_WIDGET *);
void tnProgressBarUpdate(TN_WIDGET *,int);
void DestroyProgressBar(TN_WIDGET *);
int tnGetProgressBarValue(TN_WIDGET *);
int tnGetProgressBarStepSize(TN_WIDGET *);
int tnSetProgressBarStepSize(TN_WIDGET *,int);
TN_COLOR tnGetProgressBarFillColor(TN_WIDGET *);
int tnSetProgressBarFillColor(TN_WIDGET *,TN_COLOR);
int tnSetProgressBarStyle(TN_WIDGET *,TN_BOOL);
#endif /*_TNPROGRESSBAR_H_*/
