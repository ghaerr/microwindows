 /*Header file for the Text Box widget
    * This file is part of `TinyWidgets', a widget set for the nano-X GUI which 
    * is a part of the Microwindows project (www.microwindows.org).
    * Copyright C 2000
    * Sunil Soman <sunil_soman@vsnl.com>
    * Amit Kulkarni <amms@vsnl.net>
    * Navin Thadani <navs@vsnl.net>
    *
    * This library is free software; you can redistribute it and/or modify it
    * under the terms of the GNU Lesser General Public License as published by 
    * the Free Software Foundation; either version 2.1 of the License, 
    * or (at your option) any later version.
    *
    * This library is distributed in the hope that it will be useful, but 
    * WITHOUT ANY WARRANTY; without even the implied warranty of 
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser 
    * General Public License for more details.
    *
    * You should have received a copy of the GNU Lesser General Public License 
    * along with this library; if not, write to the Free Software Foundation, 
    * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
  */

#include "../include/tnWidgets.h"
#include<string.h>
#include<ctype.h>

void
CreateTextBox (TN_WIDGET * widget,
	       TN_WIDGET * parent,
	       int posx,
	       int posy,
	       char *defaulttext,
	       GR_SIZE height,
	       GR_SIZE width,
	       char *fontname,
	       GR_SIZE fontsize,
	       GR_COLOR bgcolor, GR_COLOR fgcolor, int maxlength, int type)
{

  int i;
  GR_SIZE charheight;
  GR_FONT_ID font;
  GR_FONT_INFO fontinfo;
  if(strcmp(fontname,""))
	  font  = GrCreateFont (fontname, fontsize, NULL);
  else
	  font = TN_DEFAULT_FONT_NO;

  GrSetGCFont (widget->gc, font);
  if (height == 0)
    {
      if (type == TN_SINGLE_LINE)
	height = TN_TEXTBOX_SINGLELINE_HEIGHT;
      else
	height = TN_TEXTBOX_MULTILINE_HEIGHT;
    }
  if (width == 0)
    width = TN_TEXTBOX_WIDTH;


  widget->WSpec.textbox.FGColor = fgcolor;
  bgcolor = GR_RGB (255, 255, 255);

  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0, bgcolor, 0);

  GrSelectEvents (widget->wid,
		  GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP);
  for (i = 0; i < TEXTBOX_CALLBACKS; i++)
    widget->WSpec.textbox.CallBack[i].fp = NULL;

  widget->WSpec.textbox.viewstart = 0;
  widget->WSpec.textbox.currpos = 0;
  widget->WSpec.textbox.lastpos = -1;
  widget->WSpec.textbox.size = 0;
  widget->WSpec.textbox.buffer = NULL;
  widget->WSpec.textbox.overwrite = 0;
  widget->WSpec.textbox.hasfocus = 0;
  widget->WSpec.textbox.lastline = 0;
  widget->WSpec.textbox.cursorx = 2;
  widget->WSpec.textbox.cursory = 2;
  widget->WSpec.textbox.type = type;
  if (type == TN_SINGLE_LINE)
    widget->WSpec.textbox.lines = 1;
  else
    {
      GrGetFontInfo (font, &fontinfo);
      charheight = fontinfo.height;
      widget->WSpec.textbox.lines = (height - 4) / charheight;
    }
  if (defaulttext)
    AddStringToBuffer (widget, defaulttext);
  return;

}


void
TextBoxEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  static int cursorx, cursory;
  GR_WINDOW_INFO winfo;
  GR_GC_INFO gcinfo;
  GR_FONT_INFO finfo;
  GR_SIZE textht, textwd, textbase;
  char str[2] = { "A" };
  cursorx = widget->WSpec.textbox.cursorx;
  cursory = widget->WSpec.textbox.cursory;
  
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_DOWN:
      if(!widget->enabled)
      	break;
      SetCursor (widget, event);
      GrSetFocus (event->button.subwid);
      GrSelectEvents (widget->wid,
		      GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_BUTTON_UP |
		      GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_FOCUS_OUT);
      widget->WSpec.textbox.hasfocus = GR_TRUE;
      if (widget->WSpec.textbox.CallBack[GOTFOCUS].fp)
	(*(widget->WSpec.textbox.CallBack[GOTFOCUS].fp)) (widget,
							  widget->WSpec.
							  textbox.
							  CallBack
							  [GOTFOCUS].dptr);



      break;

    case GR_EVENT_TYPE_EXPOSURE:
      GrClearWindow (widget->wid, GR_FALSE);
      DrawTextBox (widget, 1);
      break;

    case GR_EVENT_TYPE_FOCUS_OUT:
      GrSelectEvents (widget->wid,
		      GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_EXPOSURE);
      if (widget->WSpec.textbox.CallBack[LOSTFOCUS].fp)
	(*(widget->WSpec.textbox.CallBack[LOSTFOCUS].fp)) (widget,
							   widget->WSpec.
							   textbox.
							   CallBack
							   [LOSTFOCUS].dptr);
      EraseCursor (widget);
      widget->WSpec.textbox.hasfocus = GR_FALSE;
      break;

    case GR_EVENT_TYPE_KEY_DOWN:
      switch (event->keystroke.ch)
	{
	case MWKEY_BACKSPACE:
	case 127:
	  /*Backspace key */
	  if (widget->WSpec.textbox.currpos == 0)
	    break;
	  ShiftCharsLeft (widget);
	  DrawTextBox (widget, 0);
	  if (widget->WSpec.textbox.CallBack[MODIFIED].fp)
	    (*(widget->WSpec.textbox.CallBack[MODIFIED].fp)) (widget,
							      widget->WSpec.
							      textbox.
							      CallBack
							      [MODIFIED].
							      dptr);


	  break;


	default:
	  /*These are actual characters that will be entered in the buffer */

      	  EraseCursor(widget);
	  AddCharToBuffer (widget, event->keystroke.ch);
	  GrGetWindowInfo (widget->wid, &winfo);
	  DrawTextBox (widget, 0);
	  if (widget->WSpec.textbox.CallBack[MODIFIED].fp)
	    (*(widget->WSpec.textbox.CallBack[MODIFIED].fp)) (widget,
							      widget->WSpec.
							      textbox.
							      CallBack
							      [MODIFIED].
							      dptr);

	  break;

	case MWKEY_KP4:
	case MWKEY_LEFT:
	  /*Left Arrow key */
	  if (widget->WSpec.textbox.currpos)
	    {
	      --(widget->WSpec.textbox.currpos);
	      if (widget->WSpec.textbox.currpos <=
		  widget->WSpec.textbox.viewstart
		  && widget->WSpec.textbox.viewstart&&widget->WSpec.textbox.type==TN_SINGLE_LINE)
		{
		  --(widget->WSpec.textbox.viewstart);
		  GrClearWindow (widget->wid, GR_FALSE);
		  DrawTextBox (widget, 1);
		  break;
		}

	      str[0] =
		widget->WSpec.textbox.buffer[widget->WSpec.textbox.currpos];
	      GrGetGCTextSize (widget->gc, str, -1, GR_TFASCII, &textwd,
			       &textht, &textbase);
	      cursorx -= textwd;
	      
	      if (cursorx <= 3 && widget->WSpec.textbox.currpos)
		{
		  /*Now we are sure that this is a multiline textbox*/	
		  GrGetWindowInfo (widget->wid, &winfo);
		  cursory -= textht;
		  cursorx = winfo.width - 3;
		  if(cursory<=2)
		  {
			  tnLineUp(widget);
			  cursory=2;
		  }
		  SetCursorxy (widget, cursorx, cursory);
		}
	      else
		{
		  EraseCursor (widget);
		  DrawCursor (widget, cursorx, cursory);
		}
	    }
	  break;

	case MWKEY_KP6:
	case MWKEY_RIGHT:
	  /*Right Arrow key */

	  if (widget->WSpec.textbox.currpos <= widget->WSpec.textbox.lastpos)
	    {
	      ++(widget->WSpec.textbox.currpos);
	      GrClearWindow (widget->wid, GR_FALSE);
	      DrawTextBox (widget, 1);
	    }
	  break;

	case MWKEY_KP8:
	case MWKEY_UP:
	  /*Up arrow key */
	  if (widget->WSpec.textbox.type == TN_MULTI_LINE)
	    {
	      GrGetGCInfo(widget->gc,&gcinfo);
	      GrGetFontInfo(gcinfo.font,&finfo);
		    
	      if(widget->WSpec.textbox.cursory>2)
		      SetCursorxy(widget,widget->WSpec.textbox.cursorx,widget->WSpec.textbox.cursory-finfo.height);
	      else
	      {
		      tnLineUp(widget);
		      SetCursorxy(widget,widget->WSpec.textbox.cursorx,2);
	      }
	      
	    }
	  break;

	case MWKEY_KP2:
	case MWKEY_DOWN:
	  /*Down arrow key */
	  if (widget->WSpec.textbox.type == TN_MULTI_LINE)
	    {
		    GrGetGCInfo(widget->gc,&gcinfo);
   		    GrGetFontInfo(gcinfo.font,&finfo);
    		    if(widget->WSpec.textbox.cursory<((widget->WSpec.textbox.lines-1)*finfo.height)+2)
	  		    SetCursorxy(widget,widget->WSpec.textbox.cursorx,widget->WSpec.textbox.cursory+finfo.height);
		    else
			    tnLineDown(widget);      

	    }
	  break;

	case MWKEY_KP7:
	case MWKEY_HOME:
	  /*Home */
	  if (widget->WSpec.textbox.type == TN_SINGLE_LINE)
	    {
	      widget->WSpec.textbox.currpos = 0;
	      widget->WSpec.textbox.viewstart = 0;
	      GrClearWindow (widget->wid, GR_FALSE);
	      DrawTextBox (widget, 1);
	    }
	  else
	    {
	      SetCursorxy (widget, 3, widget->WSpec.textbox.cursory);
	    }
	  break;

	case MWKEY_END:
	case MWKEY_KP1:

	  /*End */
	  if (widget->WSpec.textbox.type == TN_SINGLE_LINE)
	    {
	      widget->WSpec.textbox.currpos =
		widget->WSpec.textbox.lastpos + 1;
	      widget->WSpec.textbox.viewstart = 0;
	      GrClearWindow (widget->wid, GR_FALSE);
	      DrawTextBox (widget, 1);
	    }
	  else
	    {
	      GrGetWindowInfo (widget->wid, &winfo);
	      SetCursorxy (widget, winfo.width - 3,
			   widget->WSpec.textbox.cursory);
	    }
	  break;

	case MWKEY_DELETE:
	case MWKEY_KP_PERIOD:

	  /*Delete */
	  if (++widget->WSpec.textbox.currpos <=
	      widget->WSpec.textbox.lastpos + 1)
	    {
	      ShiftCharsLeft (widget);
	      GrClearWindow (widget->wid, GR_FALSE);
	      DrawTextBox (widget, 1);
	      if (widget->WSpec.textbox.CallBack[MODIFIED].fp)
		(*(widget->WSpec.textbox.CallBack[MODIFIED].fp))
		  (widget, widget->WSpec.textbox.CallBack[MODIFIED].dptr);
	    }
	  else
	    --widget->WSpec.textbox.currpos;


	  break;

	case MWKEY_INSERT:
	case MWKEY_KP0:
	  /*Overwrite */
	  widget->WSpec.textbox.overwrite ^= 1;
	  break;

	case MWKEY_UNKNOWN:
	case MWKEY_TAB:
	case MWKEY_ESCAPE:
	case MWKEY_PAGEUP:
	case MWKEY_PAGEDOWN:
	case MWKEY_NUMLOCK:
	case MWKEY_CAPSLOCK:
	case MWKEY_SCROLLOCK:
	case MWKEY_LSHIFT:
	case MWKEY_RSHIFT:
	case MWKEY_LCTRL:
	case MWKEY_RCTRL:
	case MWKEY_LALT:
	case MWKEY_RALT:
	case MWKEY_LMETA:
	case MWKEY_RMETA:
	case MWKEY_F1:
	case MWKEY_F2:
	case MWKEY_F3:
	case MWKEY_F4:
	case MWKEY_F5:
	case MWKEY_F6:
	case MWKEY_F7:
	case MWKEY_F8:
	case MWKEY_F9:
	case MWKEY_F10:
	case MWKEY_F11:
	case MWKEY_F12:
	  break;		/* Ignore These Keys */

	case MWKEY_KP_DIVIDE:
	  AddCharToBuffer (widget, '/');
	  GrClearWindow (widget->wid, GR_FALSE);
	  DrawTextBox (widget, 1);
	  if (widget->WSpec.textbox.CallBack[MODIFIED].fp)
	    (*(widget->WSpec.textbox.CallBack[MODIFIED].fp)) (widget,
							      widget->WSpec.
							      textbox.
							      CallBack
							      [MODIFIED].
							      dptr);

	  break;
	case MWKEY_KP_MULTIPLY:
	  AddCharToBuffer (widget, '*');
	  GrClearWindow (widget->wid, GR_FALSE);
	  DrawTextBox (widget, 1);
	  if (widget->WSpec.textbox.CallBack[MODIFIED].fp)
	    (*(widget->WSpec.textbox.CallBack[MODIFIED].fp)) (widget,
							      widget->WSpec.
							      textbox.
							      CallBack
							      [MODIFIED].
							      dptr);

	  break;
	case MWKEY_KP_MINUS:
	  AddCharToBuffer (widget, '-');
	  GrClearWindow (widget->wid, GR_FALSE);
	  DrawTextBox (widget, 1);
	  if (widget->WSpec.textbox.CallBack[MODIFIED].fp)
	    (*(widget->WSpec.textbox.CallBack[MODIFIED].fp)) (widget,
							      widget->WSpec.
							      textbox.
							      CallBack
							      [MODIFIED].
							      dptr);

	  break;
	case MWKEY_KP_PLUS:
	  AddCharToBuffer (widget, '+');
	  GrClearWindow (widget->wid, GR_FALSE);
	  DrawTextBox (widget, 1);
	  if (widget->WSpec.textbox.CallBack[MODIFIED].fp)
	    (*(widget->WSpec.textbox.CallBack[MODIFIED].fp)) (widget,
							      widget->WSpec.
							      textbox.
							      CallBack
							      [MODIFIED].
							      dptr);

	  break;



	case MWKEY_ENTER:
	case MWKEY_KP_ENTER:
	  if(widget->WSpec.textbox.type==TN_SINGLE_LINE)
		  break;
	  AddCharToBuffer (widget, '\r');
	  GrClearWindow (widget->wid, GR_FALSE);
	  DrawTextBox (widget, 1);
	  if (widget->WSpec.textbox.CallBack[MODIFIED].fp)
	    (*(widget->WSpec.textbox.CallBack[MODIFIED].fp)) (widget,
							      widget->WSpec.
							      textbox.
							      CallBack
							      [MODIFIED].
							      dptr);

	  break;


	}
    }

}



void
DrawTextBox (TN_WIDGET * textbox, int eraseall)
{
  if (textbox->WSpec.textbox.type == TN_SINGLE_LINE)
    DrawTextEntry (textbox);
  else
    DrawTextArea (textbox, eraseall);
}

void
DrawTextEntry (TN_WIDGET * textbox)
{
  GR_WINDOW_INFO winfo;
  GR_FONT_INFO fontinfo;
  GR_GC_INFO gcinfo;
  int i, j;
  int textwidth, textheight, textbase;
  char *viewbuff =
    (char *) malloc ((textbox->WSpec.textbox.size + 1) * sizeof (char));
  int xshow;
  int firstchars;
  int caretpos;

  GrClearWindow(textbox->wid,GR_FALSE);
  GrGetWindowInfo (textbox->wid, &winfo);
  GrLine (textbox->wid, textbox->gc, 0, winfo.height, winfo.width,
	  winfo.height);
  GrLine (textbox->wid, textbox->gc, winfo.width, 0, winfo.width,
	  winfo.height);
  GrSetGCForeground (textbox->gc, 0);
  GrLine (textbox->wid, textbox->gc, 0, 0, 0, winfo.height);
  GrLine (textbox->wid, textbox->gc, 0, 0, winfo.width, 0);
  GrSetGCBackground (textbox->gc, GR_RGB (255, 255, 255));
  GrSetGCForeground (textbox->gc, textbox->WSpec.textbox.FGColor);
  GrGetGCInfo (textbox->gc, &gcinfo);
  GrGetFontInfo (gcinfo.font, &fontinfo);

  xshow = 3;
  do
    {
      firstchars =
	(textbox->WSpec.textbox.currpos - textbox->WSpec.textbox.viewstart);

      memcpy (viewbuff,
	      (textbox->WSpec.textbox.buffer +
	       textbox->WSpec.textbox.viewstart), firstchars * sizeof (char));
      viewbuff[firstchars] = '\0';
      GrGetGCTextSize (textbox->gc, viewbuff, strlen (viewbuff), GR_TFASCII,
		       &textwidth, &textheight, &textbase);
      if (textwidth >= winfo.width - 3)
	textbox->WSpec.textbox.viewstart++;
    }
  while (textwidth >= winfo.width - 3);

  xshow += textwidth;
  caretpos = xshow;
  i = firstchars;
  for (j = textbox->WSpec.textbox.currpos;
       j <= textbox->WSpec.textbox.lastpos; j++)
    {
      viewbuff[i] = textbox->WSpec.textbox.buffer[j];
      viewbuff[++i] = '\0';
      textbox->WSpec.textbox.buffer[textbox->WSpec.textbox.lastpos + 1] =
	'\0';
      GrGetGCTextSize (textbox->gc, viewbuff, strlen (viewbuff), GR_TFASCII,
		       &textwidth, &textheight, &textbase);
      xshow = textwidth;
      if (xshow > winfo.width - 3)
	{
	  viewbuff[--i] = '\0';
	  break;
	}
    }

  GrText (textbox->wid, textbox->gc, 3, 1, viewbuff, strlen (viewbuff),
	  GR_TFASCII | GR_TFTOP);
  if (textbox->WSpec.textbox.hasfocus)
    DrawCursor (textbox, caretpos, 2);
  return;
}

void
DrawTextArea (TN_WIDGET * textbox, int eraseall)
{
  GR_WINDOW_INFO winfo;
  GR_FONT_INFO fontinfo;
  GR_GC_INFO gcinfo;
  int i, j;
  int charheight, textwidth, textheight, textbase;
  int xshow, yshow;
  int caretposx, caretposy, position;
  int newlineoff=0;
  int drawflag;
  char currchar[2] = { "A" };

  char **linebuff =
    (char **) malloc (textbox->WSpec.textbox.lines * sizeof (char *));
  for (i = 0; i < textbox->WSpec.textbox.lines; i++)
    linebuff[i] = (char *) malloc (256 * sizeof (char));
  
  if(textbox->WSpec.textbox.lastpos==-1)
  	  GrClearWindow(textbox->wid,GR_FALSE);
  if(textbox->WSpec.textbox.currpos<=textbox->WSpec.textbox.viewstart)
  {
	  textbox->WSpec.textbox.viewstart=textbox->WSpec.textbox.currpos;
	  eraseall=1;
  }
	 
  if(eraseall)
  	  GrClearWindow(textbox->wid,GR_FALSE);
  GrGetWindowInfo (textbox->wid, &winfo);
  GrLine (textbox->wid, textbox->gc, 0, winfo.height, winfo.width,
	  winfo.height);
  GrLine (textbox->wid, textbox->gc, winfo.width, 0, winfo.width,
	  winfo.height);
  GrSetGCForeground (textbox->gc, 0);
  GrLine (textbox->wid, textbox->gc, 0, 0, 0, winfo.height);
  GrLine (textbox->wid, textbox->gc, 0, 0, winfo.width, 0);
  GrSetGCBackground (textbox->gc, GR_RGB (255, 255, 255));
  GrSetGCForeground (textbox->gc, textbox->WSpec.textbox.FGColor);
  GrGetGCInfo (textbox->gc, &gcinfo);
  GrGetFontInfo (gcinfo.font, &fontinfo);
  charheight = fontinfo.height;

  drawflag=eraseall;

  while (1)
    {
      yshow = 2;
      caretposy = yshow;
      caretposx = 2;
      position = textbox->WSpec.textbox.viewstart;
      for (i = 0;
	   i < textbox->WSpec.textbox.lines
	   && position <= textbox->WSpec.textbox.lastpos; i++)
	{
	  xshow = 0;
	  for (j = 0; j < 256 && position <= textbox->WSpec.textbox.lastpos;
	       j++)
	    {
	      linebuff[i][j] = textbox->WSpec.textbox.buffer[position++];
	      linebuff[i][j + 1] = '\0';
	      GrGetGCTextSize (textbox->gc, linebuff[i], -1,
			       GR_TFASCII | GR_TFTOP, &textwidth, &textheight,
			       &textbase);
	      if (position == textbox->WSpec.textbox.currpos)
		{
		  caretposx = textwidth + 3;
		  caretposy = yshow;
		  if (!eraseall)
		    {
		      /*
		       * Dont erase all lines
		       * Only erase invalid lines
		       */
		      drawflag = 0;
		      GrSetGCForeground (textbox->gc, GR_RGB (255, 255, 255));
		      GrFillRect (textbox->wid, textbox->gc, caretposx,
				  caretposy, winfo.width - caretposx - 2,
				  fontinfo.height);
		      GrFillRect (textbox->wid, textbox->gc, 1,
				  caretposy + fontinfo.height,
				  winfo.width - 2,
				  winfo.height - caretposy - 2);
		      GrSetGCForeground (textbox->gc,
				 textbox->WSpec.textbox.FGColor);
		    }

		  drawflag = 1;
		}

	      if (linebuff[i][j] == '\r')
		{
		  if (position == textbox->WSpec.textbox.currpos)
		    {
		      caretposx = 2;
		      caretposy = yshow + textheight;
		    }
		  
		  break;
		}

	      if (textwidth >= (winfo.width - 6))
		{

		  position -= 1;
		  linebuff[i][j] = '\0';
		  /*if ((position+1) == textbox->WSpec.textbox.currpos)
	  	 	 EraseCursor(textbox);*/
		  break;
		}

	      currchar[0] = linebuff[i][j];
	      if (drawflag)
		GrText (textbox->wid, textbox->gc, xshow + 3, yshow, currchar,
			-1, GR_TFASCII | GR_TFTOP);
	      xshow = textwidth;
	    }
	  if (i == 0)
	    newlineoff = position;
	  yshow += charheight;
	}
      if (position >= textbox->WSpec.textbox.currpos)
	break;
      GrClearWindow (textbox->wid, GR_FALSE);
      textbox->WSpec.textbox.lastline = textbox->WSpec.textbox.viewstart;
      textbox->WSpec.textbox.viewstart = newlineoff;

    }
  
  if (textbox->WSpec.textbox.hasfocus)
  {
       DrawCursor (textbox, caretposx, caretposy);
  }

  return;
}

void
AddStringToBuffer (TN_WIDGET * widget, char *str)
{
  while (*str)
    AddCharToBuffer (widget, *str++);
  return;
}

void
AddCharToBuffer (TN_WIDGET * widget, char ch)
{

  char *newbuff;
  if (!widget->WSpec.textbox.overwrite)
    {
      if ((ShiftRemainder (widget)) < 0)
	return;
      widget->WSpec.textbox.buffer[widget->WSpec.textbox.currpos] = ch;
      widget->WSpec.textbox.currpos++;
    }
  else
    {
      if (widget->WSpec.textbox.currpos + 1 >= widget->WSpec.textbox.lastpos)
	{
	  ++widget->WSpec.textbox.lastpos;
	  if (widget->WSpec.textbox.lastpos >= widget->WSpec.textbox.size)
	    {
	      widget->WSpec.textbox.size += MAXBUFFER;
	      newbuff =
		(char *) realloc (widget->WSpec.textbox.buffer,
				  widget->WSpec.textbox.size);
	      if (newbuff == NULL)
		{
		  fputs
		    ("tnWidgets: Cannot allocate more memory for textbox\n",
		     stderr);
		  widget->WSpec.textbox.lastpos--;
		  return;
		}
	      widget->WSpec.textbox.buffer = newbuff;
	    }
	}
      widget->WSpec.textbox.buffer[widget->WSpec.textbox.currpos] = ch;
      widget->WSpec.textbox.currpos++;

    }
}

int
ShiftRemainder (TN_WIDGET * widget)
{
  char *newbuff;
  int offset = widget->WSpec.textbox.currpos;
  char repchar='\0', oldchar='\0';

  widget->WSpec.textbox.lastpos++;
  if (widget->WSpec.textbox.lastpos >= widget->WSpec.textbox.size)
    {
      widget->WSpec.textbox.size += MAXBUFFER;
      newbuff =
	(char *) realloc (widget->WSpec.textbox.buffer,
			  widget->WSpec.textbox.size);
      if (newbuff == NULL)
	{
	  fputs ("tnWidgets: Cannot allocate more memory for textbox\n",
		 stderr);
	  widget->WSpec.textbox.lastpos--;
	  widget->WSpec.textbox.currpos--;
	  return -1;
	}
      widget->WSpec.textbox.buffer = newbuff;
    }

  while (offset <= widget->WSpec.textbox.lastpos)
    {
      oldchar = widget->WSpec.textbox.buffer[offset];
      widget->WSpec.textbox.buffer[offset++] = repchar;
      repchar = oldchar;
    }
  return 1;
}


void
ShiftCursor (TN_WIDGET * widget, int direction)
{
  return;
}

void
ShiftCharsLeft (TN_WIDGET * widget)
{
  int i;
  for (i = widget->WSpec.textbox.currpos;
       i <= widget->WSpec.textbox.lastpos; ++i)
    widget->WSpec.textbox.buffer[i - 1] = widget->WSpec.textbox.buffer[i];

  --(widget->WSpec.textbox.currpos);
  if (widget->WSpec.textbox.currpos < widget->WSpec.textbox.viewstart)
    widget->WSpec.textbox.viewstart = 0;
  --(widget->WSpec.textbox.lastpos);

  return;
}


char *
tnGetText (TN_WIDGET * widget)
{
  int len = widget->WSpec.textbox.lastpos + 1;
  char *s = (char *) malloc ((len + 1) * sizeof (char));
  if (s)
    {
      memcpy (s, widget->WSpec.textbox.buffer, len * sizeof (char));
      s[len] = '\0';
    }
  return s;
}

int
tnSetText (TN_WIDGET * widget, char *s)
{
  if(widget->type!=TN_TEXTBOX || !s)
	  return -1;
  widget->WSpec.textbox.viewstart = 0;
  widget->WSpec.textbox.currpos = 0;
  widget->WSpec.textbox.lastpos = -1;
  widget->WSpec.textbox.size = 0;

  if (widget->WSpec.textbox.buffer)
    free (widget->WSpec.textbox.buffer);
  widget->WSpec.textbox.buffer = NULL;
  AddStringToBuffer (widget, s);
  GrClearWindow (widget->wid, GR_FALSE);
  DrawTextBox (widget, 1);

  return 1;
}

void
SetCursor (TN_WIDGET * widget, GR_EVENT * event)
{
  if (widget->WSpec.textbox.type == TN_SINGLE_LINE)
    SetCursorLine (widget, event);
  else
    SetCursorArea (widget, event);
}

void
SetCursorLine (TN_WIDGET * widget, GR_EVENT * event)
{
  char s[256];
  int cursorx=widget->WSpec.textbox.cursorx;
  int offset = widget->WSpec.textbox.viewstart, i = 0, height, width =
    0, base;
  int targetx = event->button.x;
  
  if(widget->WSpec.textbox.buffer)  
	  while (offset <= widget->WSpec.textbox.lastpos +1 )
	    {
	      s[i++] = widget->WSpec.textbox.buffer[offset++];
	      s[i] = '\0';
	      cursorx = width;
	      GrGetGCTextSize (widget->gc, s, -1, GR_TFASCII, &width, &height, &base);
	      if (width >= targetx)
		break;
	    }
  else
	  cursorx=0;
  EraseCursor (widget);
  DrawCursor (widget, cursorx + 3, 2);
  widget->WSpec.textbox.currpos = (offset) ? offset - 1 : offset;
}

void
EraseCursor (TN_WIDGET * widget)
{
  GR_GC_INFO gcinfo;
  GR_FONT_INFO fontinfo;
  if (!widget->WSpec.textbox.hasfocus)
    return;
  GrGetGCInfo (widget->gc, &gcinfo);
  GrGetFontInfo (gcinfo.font, &fontinfo);

  GrSetGCForeground (widget->gc, GR_RGB (255, 255, 255));
  GrSetGCMode (widget->gc, GR_MODE_XOR);
  GrLine (widget->wid, widget->gc, widget->WSpec.textbox.cursorx,
	  widget->WSpec.textbox.cursory, widget->WSpec.textbox.cursorx,
	  widget->WSpec.textbox.cursory + fontinfo.height - 2);
  GrSetGCMode (widget->gc, GR_MODE_SET);
  GrSetGCForeground (widget->gc, widget->WSpec.textbox.FGColor);
  return;
}

void
DrawCursor (TN_WIDGET * widget, int x, int y)
{
  GR_GC_INFO gcinfo;
  GR_FONT_INFO fontinfo;
  GrGetGCInfo (widget->gc, &gcinfo);
  GrGetFontInfo (gcinfo.font, &fontinfo);
  GrSetGCForeground (widget->gc, GR_RGB (255, 255, 255));
  GrSetGCMode (widget->gc, GR_MODE_XOR);
  GrLine (widget->wid, widget->gc, x, y, x, y + fontinfo.height - 2);
  GrSetGCMode (widget->gc, GR_MODE_SET);
  widget->WSpec.textbox.cursorx = x;
  widget->WSpec.textbox.cursory = y;

  return;
}


void
SetCursorArea (TN_WIDGET * textbox, GR_EVENT * event)
{
  SetCursorxy (textbox, event->button.x, event->button.y);
}

void
SetCursorxy (TN_WIDGET * textbox, int xtarget, int ytarget)
{
  int i, j;
  int textheight, textwidth, textbase;
  int targetline;
  int position;
  int cursorx = 3, cursory = 2;
  char **linebuff =
    (char **) malloc (textbox->WSpec.textbox.lines * sizeof (char *));
  GR_WINDOW_INFO winfo;
  GR_FONT_INFO fontinfo;
  GR_GC_INFO gcinfo;

  for (i = 0; i < textbox->WSpec.textbox.lines; i++)
    linebuff[i] = (char *) malloc (256 * sizeof (char));

  GrGetWindowInfo (textbox->wid, &winfo);
  GrGetGCInfo (textbox->gc, &gcinfo);
  GrGetFontInfo (gcinfo.font, &fontinfo);
  textheight = fontinfo.height;
  targetline = ytarget / textheight;

  position = textbox->WSpec.textbox.viewstart;
  for (i = 0;
       i <= textbox->WSpec.textbox.lines
       && position <= textbox->WSpec.textbox.lastpos; i++)
    {
      for (j = 0; j < 256 && position <= textbox->WSpec.textbox.lastpos; j++)
	{
	  linebuff[i][j] = textbox->WSpec.textbox.buffer[position++];
	  linebuff[i][j + 1] = '\0';
	  if (linebuff[i][j] == '\r')
	  {
	    break;
	  }
	  GrGetGCTextSize (textbox->gc, linebuff[i], -1,
			   GR_TFASCII | GR_TFTOP, &textwidth, &textheight,
			   &textbase);


	  if (((textwidth + 3) >= xtarget && i >= targetline)
	      || (i > targetline))
	    break;
	  else
	    cursorx = textwidth + 3;

	  if (textwidth >= (winfo.width - 6))
	    {
	      position -= 1;
	      linebuff[i][j] = '\0';
	      break;
	    }

	}
      if ((i >= targetline && (textwidth + 3) >= xtarget) || (i > targetline))
	{
	  position--;
	  textbox->WSpec.textbox.currpos = position;
	  if ((textwidth + 3) > xtarget)
	    {
	      linebuff[i][j] = '\0';
	      GrGetGCTextSize (textbox->gc, linebuff[i], -1,
			       GR_TFASCII | GR_TFTOP, &textwidth, &textheight,
			       &textbase);
	    }

	  if (i > targetline)
	    {
	      cursory -= textheight;
	      if(textbox->WSpec.textbox.buffer[position-1]=='\r')
		      textbox->WSpec.textbox.currpos--;
	      GrGetGCTextSize (textbox->gc, linebuff[i - 1], -1,
			       GR_TFASCII | GR_TFTOP, &textwidth, &textheight,
			       &textbase);
	    }


	  cursorx = textwidth + 3;
	  break;
	}
      cursory += textheight;
    }
  if (position >= textbox->WSpec.textbox.lastpos)
    {
      textbox->WSpec.textbox.currpos = textbox->WSpec.textbox.lastpos + 1;
      cursory -= textheight;
      cursorx = textwidth + 3;
    }

  EraseCursor (textbox);
  DrawCursor (textbox, cursorx, cursory);

}

void
tnLineUp (TN_WIDGET * textbox)
{
	//int position;
	char str[256];
	int i=0;
	GR_FONT_INFO fontinfo;
	GR_GC_INFO gcinfo;
	GR_WINDOW_INFO winfo;
	int textwidth,textheight,textbase;
	
	GrGetWindowInfo(textbox->wid,&winfo);
	GrGetGCInfo(textbox->gc,&gcinfo);
	GrGetFontInfo(gcinfo.font,&fontinfo);

	//position=textbox->WSpec.textbox.viewstart;
	for(i=0;i<255;i++)
	{
		str[i]=textbox->WSpec.textbox.buffer[textbox->WSpec.textbox.viewstart-i-1];
		str[i+1]='\0';
		strrev(str);
		GrGetGCTextSize(textbox->gc,str, -1,				                              GR_TFASCII | GR_TFTOP, &textwidth, &textheight,
			      &textbase);
		printf("TextWidth=%d/%d\tpos=%d\tstr=%s\n",textwidth,winfo.width-6,textbox->WSpec.textbox.viewstart-i-1,str);fflush(stdout);
		strrev(str);
		if( (textwidth>=(winfo.width-6))|| (textbox->WSpec.textbox.buffer[textbox->WSpec.textbox.viewstart-i-1] == '\r')||(textbox->WSpec.textbox.viewstart-i-1 == 0))
		{		
			textbox->WSpec.textbox.viewstart-=i;
			if(textbox->WSpec.textbox.viewstart-i-1 == 0)
				textbox->WSpec.textbox.viewstart--;
			textbox->WSpec.textbox.currpos=textbox->WSpec.textbox.viewstart+1;
			break;
		}

	}
	DrawTextBox(textbox,1);
		
}

void
tnLineDown (TN_WIDGET * textbox)
{

/*  int xshow, yshow;
  int i, j;
  int textheight, textwidth, textbase;
  int position;
  char **linebuff =
    (char **) malloc (textbox->WSpec.textbox.lines * sizeof (char *));
  GR_WINDOW_INFO winfo;

  for (i = 0; i < textbox->WSpec.textbox.lines; i++)
    linebuff[i] = (char *) malloc (256 * sizeof (char));

  GrGetWindowInfo (textbox->wid, &winfo);

  yshow = 2;
  xshow = 2;
  position = textbox->WSpec.textbox.viewstart;
  for (i = 0;
       i <= textbox->WSpec.textbox.lines
       && position <= textbox->WSpec.textbox.lastpos; i++)
    {
      for (j = 0; j < 256 && position <= textbox->WSpec.textbox.lastpos; j++)
	{
	  linebuff[i][j] = textbox->WSpec.textbox.buffer[position++];
	  linebuff[i][j + 1] = '\0';
	  if (linebuff[i][j] == '\r')
	    break;
	  GrGetGCTextSize (textbox->gc, linebuff[i], -1,
			   GR_TFASCII | GR_TFTOP, &textwidth, &textheight,
			   &textbase);

	  xshow = textwidth;
	  if (position == textbox->WSpec.textbox.currpos)
	    break;

	  if (xshow > winfo.width - 8)
	    {
	      xshow = 2;
	      break;
	    }
	}
      if (position == textbox->WSpec.textbox.currpos)
	{
	  break;
	}
      yshow += textheight;
    }

  if (position == textbox->WSpec.textbox.currpos)
    SetCursorxy (textbox, xshow, yshow + textheight);*/

}

void
DestroyTextBox (TN_WIDGET * widget)
{
  DeleteFromRegistry (widget);
  return;
}

void strrev(char *s)
{
	int i,j;
	char temp;
	for(i=0,j=strlen(s)-1;i<j;i++,j--)
	{
		temp=s[j];
		s[j]=s[i];
		s[i]=temp;
	}
}
