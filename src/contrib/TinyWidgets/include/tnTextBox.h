 /*Header file for the Text Box widget
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

#ifndef _TNTEXTBOX_H_
#define _TNTEXTBOX_H_
#define TEXTBOX_CALLBACKS 3 
#include "tnBase.h"
#define TN_TEXTBOX_SINGLELINE_HEIGHT 15 
#define TN_TEXTBOX_MULTILINE_HEIGHT 90
#define TN_TEXTBOX_WIDTH 90 
#define MAXBUFFER 80
#define TN_SINGLE_LINE 0
#define TN_MULTI_LINE  1


typedef struct
{
  GR_COLOR FGColor;
  char *buffer;
  int currpos;
  int lastpos;
  int viewstart;
  int size;
  int overwrite;
  int hasfocus;
  int lines;
  int type;
  int lastline;
  int cursorx;
  int cursory;
  CallBackStruct CallBack[TEXTBOX_CALLBACKS];
}
TN_STRUCT_TEXTBOX;

void
CreateTextBox (TN_WIDGET * ,
              TN_WIDGET *,
              int ,
              int ,
	      char *,
              GR_SIZE ,
              GR_SIZE ,
              char * ,
              GR_SIZE ,
              GR_COLOR ,
              GR_COLOR ,
	      int,
	      int);
void TextBoxEventHandler(GR_EVENT *,TN_WIDGET *);
void DrawTextBox (TN_WIDGET *,int);
void DrawTextEntry (TN_WIDGET *);
void DrawTextArea (TN_WIDGET *,int);
void AddStringToBuffer(TN_WIDGET *,char *);
void AddCharToBuffer(TN_WIDGET *,char );
int ShiftRemainder(TN_WIDGET *);
void AdvanceCursor(TN_WIDGET *);
struct buff * NewStructBuff(void);
void ProcessSpecialKeys(TN_WIDGET *,unsigned int );
void ShiftCursor(TN_WIDGET *,int );
void ShiftCharsLeft(TN_WIDGET *);
char *tnGetText(TN_WIDGET *);
int tnSetText(TN_WIDGET *,char *);
void SetCursor(TN_WIDGET *,GR_EVENT *);
void SetCursorLine(TN_WIDGET *,GR_EVENT *);
void SetCursorArea(TN_WIDGET *,GR_EVENT *);
void SetCursorxy(TN_WIDGET *,int ,int);
void tnLineUp(TN_WIDGET*);
void tnLineDown(TN_WIDGET*);
void EraseCursor(TN_WIDGET *);
void DrawCursor(TN_WIDGET *,int ,int );
void DestroyTextBox(TN_WIDGET *);
void strrev(char *);
#endif /*_TNTEXTBOX_H_*/
