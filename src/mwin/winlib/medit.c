/*
 * Copyright (C) 1999, 2000, Wei Yongming.
 * Portions Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Multi Line Edit Control for Microwindows win32 api.
 */

/*
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Library General Public
**  License as published by the Free Software Foundation; either
**  version 2 of the License, or (at your option) any later version.
**
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Library General Public License for more details.
**
**  You should have received a copy of the GNU Library General Public
**  License along with this library; if not, write to the Free
**  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
**  MA 02111-1307, USA
*/

/*
**  Alternatively, the contents of this file may be used under the terms 
**  of the Mozilla Public License (the "MPL License") in which case the
**  provisions of the MPL License are applicable instead of those above.
*/

/* Note:
**  Although there was a version by Zhao Jianghua, this version of
**  EDIT control is written by Wei Yongming from scratch.
**
** Create date: 1999/8/26
**
** Modify records:
**
**  Who             When        Where       For What                Status
**-----------------------------------------------------------------------------
**  WEI Yongming    2000/02/24  Tsinghua    Add MPL License         Finished
**  Kevin Tseng     2000/08/30  gv          port to microwin        ported
**
**
** TODO:
**    * Selection.
**    * Undo.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#define MWINCLUDECOLORS
#include "windows.h"	/* windef.h, winuser.h */
#include "wintools.h"
#include "device.h" 	/* GdGetTextSize */

#define USE_BIG5

#define WIDTH_MEDIT_BORDER       2
#define MARGIN_MEDIT_LEFT        1
#define MARGIN_MEDIT_TOP         1
#define MARGIN_MEDIT_RIGHT       2
#define MARGIN_MEDIT_BOTTOM      1

#define LEN_MLEDIT_BUFFER       3000
#define LEN_MLEDIT_UNDOBUFFER   1024

#define EST_FOCUSED     0x00000001L
#define EST_MODIFY      0x00000002L
#define EST_READONLY    0x00000004L
#define EST_REPLACE     0x00000008L

#define MEDIT_OP_NONE    0x00
#define MEDIT_OP_DELETE  0x01
#define MEDIT_OP_INSERT  0x02
#define MEDIT_OP_REPLACE 0x03

typedef struct tagLINEDATA {
	int     lineNO;	                  /* 行号 */
	int     dataEnd; 
	struct  tagLINEDATA *previous;    /* 前一行 */
	struct  tagLINEDATA *next;        /* 后一行 */
	char    buffer[LEN_MLEDIT_BUFFER+1];
}LINEDATA;
typedef    LINEDATA*     PLINEDATA;

#define ATTENG 0	/* english */
#define ATTCHL 1	/* chinese left(1st) byte */
#define ATTCHR 2	/* chinese right(2nd) byte */
static char attr[LEN_MLEDIT_BUFFER];

typedef struct tagMLEDITDATA {
    int     totalLen;      /* length of buffer,可能没有用 */

    int     editPos;        /* current edit position */
    int     caretPos;       /* caret offset in box */
    int     editLine;		/* current eidt line */
    int     dispPos;        /* 开始显示的位置 */
    int     StartlineDisp;  /* start line displayed */
    int     EndlineDisp;    /* end line displayed */
    int     linesDisp;      /* 需要显示的行数 */
    int     lines;		    /* 总的行数` */
    int     MaxlinesDisp;    /* 最大显示的行数. */
							
    int     selStartPos;    /* selection start position */
    int     selStartLine;   /* selection start line */
    int     selEndPos;      /* selection end position */
    int     selEndLine;     /* selection end line */
    
    int     passwdChar;     /* password character */
    
    int     leftMargin;     /* left margin */
    int     topMargin;      /* top margin */ 
    int     rightMargin;    /* right margin */
    int     bottomMargin;   /* bottom margin */
    
    int     hardLimit;      /* hard limit */

    int     lastOp;         /* last operation */
    int     lastPos;        /* last operation position */
    int     lastLine;       /* last operation line */
    int     affectedLen;    /* affected len of last operation */
    int     undoBufferLen;  /* undo buffer len */
    char    undoBuffer [LEN_MLEDIT_UNDOBUFFER];
                            /* Undo buffer; */
    PLINEDATA   head;       /* buffer */
    PLINEDATA   tail;       /* 可能不需要 */
}MLEDITDATA;
typedef MLEDITDATA* PMLEDITDATA;

BOOL RegisterMLEditControl (void);

int MLEditCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam);

#define PIXEL_invalid (-1)
extern HWND sg_hCaretWnd;
extern HWND rootwp;

static int GetSysCharHeight (HWND hwnd) 
{
#ifndef USE_BIG5	    
	HDC 		hdc;
    	int xw, xh, xb;

    	hdc = GetDC(hwnd);
	SelectObject(hdc, GetStockObject(DEFAULT_FONT));
	GdSetFont(hdc->font->pfont);
    	GdGetTextSize(hdc->font->pfont,"X",1, &xw,&xh,&xb,MWTF_ASCII);
    	ReleaseDC(hwnd,hdc);

	return xh;
#else
	return 12;
#endif
}

static int GetSysCharWidth (HWND hwnd) 
{
#ifndef USE_BIG5	    
	HDC 		hdc;
    	int xw, xh, xb;

    	hdc = GetDC(hwnd);
	SelectObject(hdc, GetStockObject(DEFAULT_FONT));
	GdSetFont(hdc->font->pfont);
    	GdGetTextSize(hdc->font->pfont,"X",1, &xw,&xh,&xb,MWTF_ASCII);
    	ReleaseDC(hwnd,hdc);

	return xw;
#else
	return 6;
#endif
}

static int GetSysCCharWidth (HWND hwnd)
{
	return (2*GetSysCharWidth(hwnd));
}
char* GetWindowCaption (HWND hWnd)
{
    return hWnd->szTitle;
}

DWORD GetWindowAdditionalData (HWND hWnd)
{
        return hWnd->userdata;
}

DWORD SetWindowAdditionalData (HWND hWnd, DWORD newData)
{
    DWORD    oldOne = 0L;

    oldOne = hWnd->userdata;
    hWnd->userdata = newData;
    
    return oldOne;
}

DWORD GetWindowAdditionalData2 (HWND hWnd)
{
        return hWnd->userdata2;
}

DWORD SetWindowAdditionalData2 (HWND hWnd, DWORD newData)
{
    DWORD    oldOne = 0L;

    oldOne = hWnd->userdata2;
    hWnd->userdata2 = newData;
    
    return oldOne;
}

DWORD GetWindowStyle (HWND hWnd)
{
        return hWnd->style;
}

BOOL ExcludeWindowStyle (HWND hWnd, DWORD dwStyle)
{
    	if (hWnd == rootwp/*HWND_DESKTOP*/)
        	return FALSE;

        hWnd->style &= ~dwStyle;
        return TRUE;
}

BOOL IncludeWindowStyle (HWND hWnd, DWORD dwStyle)
{

    	if (hWnd == rootwp/*HWND_DESKTOP*/)
        	return FALSE;

        hWnd->style |= dwStyle;
        return TRUE;
}

int WINAPI MwRegisterMEditControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)MLEditCtrlProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; 
	wc.hbrBackground= GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "MEDIT";

	return RegisterClass(&wc);
}

static inline int edtGetOutWidth (HWND hWnd)
{
	PMLEDITDATA pMLEditData =(PMLEDITDATA) GetWindowAdditionalData2(hWnd);
	RECT rc;
	GetClientRect(hWnd,&rc);
    return rc.right - rc.left 
            - pMLEditData->leftMargin
            - pMLEditData->rightMargin;
}

static int edtGetStartDispPosAtEnd (HWND hWnd,
            PLINEDATA pLineData)
{
    int         nOutWidth = edtGetOutWidth (hWnd);
    int         endPos  = pLineData->dataEnd;
    PMLEDITDATA pMLEditData =(PMLEDITDATA) GetWindowAdditionalData2(hWnd);
    int         newStartPos = pMLEditData->dispPos;
    const char* buffer = pLineData->buffer;

    if(endPos < newStartPos)
		return 0;
    while (TRUE) 
    {
        if ((endPos - newStartPos) * GetSysCharWidth (hWnd) < nOutWidth)
            break;
        
        /* 1st:gb:a1-f7,big5:a1-f9 */
        if ((BYTE)buffer [newStartPos] > 0xA0)
	{
            newStartPos ++;
            if (newStartPos < pLineData->dataEnd) 
	    {
#ifndef USE_BIG5
                if ((BYTE)buffer [newStartPos] > 0xA0)
#else	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
                if ( ((BYTE)buffer [newStartPos] >= 0x40 && (BYTE)buffer[newStartPos] <= 0x7e) ||
                     ((BYTE)buffer [newStartPos] >= 0xa1 && (BYTE)buffer[newStartPos] <= 0xfe)) 
#endif
                    newStartPos ++;
            }
        }
        else
            newStartPos ++;
    }
    return newStartPos;
}

static int edtGetDispLen (HWND hWnd,PLINEDATA pLineData)
{
    int i, n = 0;
    int nOutWidth = edtGetOutWidth (hWnd);
    int nTextWidth = 0;
    PMLEDITDATA pMLEditData =(PMLEDITDATA) GetWindowAdditionalData2(hWnd);
    const char* buffer = pLineData->buffer;
    
    if(buffer[0]==0||pLineData->dataEnd<pMLEditData->dispPos)
		return 0;
	
    for (i = pMLEditData->dispPos; i < pLineData->dataEnd; i++) 
    {
        /* 1st:gb:a1-f7,big5:a1-f9 */
        if ((BYTE)buffer [i] > 0xA0)
   	{
            i++;
            if (i < pLineData->dataEnd) 
	    {
#ifndef USE_BIG5
                if ((BYTE)buffer [i] > 0xA0)	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
#else	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
                if ( ((BYTE)buffer [i] >= 0x40 && (BYTE)buffer[i] <= 0x7e) ||
                     ((BYTE)buffer [i] >= 0xa1 && (BYTE)buffer[i] <= 0xfe))
#endif
		{
                    nTextWidth += GetSysCCharWidth (hWnd);
                    n += 2;
                }
                else
                    i--;
            }
            else 
	    {
                nTextWidth += GetSysCharWidth (hWnd);
                n++;
            }
        }
        else 
	{
            nTextWidth += GetSysCharWidth (hWnd);
            n++;
        }

        if (nTextWidth > nOutWidth)
            break;
    }

    return n;
}

static int edtGetOffset (HWND hwnd,const MLEDITDATA* pMLEditData, PLINEDATA pLineData, int x)
{
    int i;
    int newOff = 0;
    int nTextWidth = 0;
    const char* buffer = pLineData->buffer;

    if(pLineData->dataEnd<pMLEditData->dispPos)
		return pLineData->dataEnd;

    x -= pMLEditData->leftMargin;
    for (i = pMLEditData->dispPos; i < pLineData->dataEnd; i++) {
        if ((nTextWidth + (GetSysCharWidth(hwnd) >> 1)) >= x)
            break;

        /* 1st:gb:a1-f7,big5:a1-f9 */
        if ((BYTE)buffer [i] > 0xA0)
	{
            i++;

   	    if (nTextWidth + GetSysCCharWidth(hwnd)/2 >= x)
		break;

            if (i < pLineData->dataEnd) 
	    {
#ifndef USE_BIG5
                if ((BYTE)buffer [i] > 0xA0)	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
#else	/* 2nd:gb:a1-fe,big5:40-7e,a1-fe */
                if ( ((BYTE)buffer [i] >= 0x40 && (BYTE)buffer[i] <= 0x7e) || 
                     ((BYTE)buffer [i] >= 0xa1 && (BYTE)buffer[i] <= 0xfe))
#endif
		{
                    nTextWidth += GetSysCCharWidth (hwnd);
                    newOff += 2;
                }
                else
                    i --;
            }
            else 
	    {
                nTextWidth += GetSysCharWidth (hwnd);
                newOff ++;
            }
        }
        else 
	{
            nTextWidth += GetSysCharWidth (hwnd);
            newOff ++;
        }
    }
    return newOff;
}

static int edtGetLineNO (HWND hwnd,const MLEDITDATA* pMLEditData, int x)
{
    int nline = 0;
	if(x>=0)
	{
	    nline = x / GetSysCharHeight(hwnd);
		if (nline <= pMLEditData->linesDisp)
   			return nline;
	}
	return -1;
}

static BOOL edtIsACCharAtPosition (const char* string, int len, int pos)
{
    if (pos > (len - 2))
        return FALSE;

/* 1st:gb:a1-f7,big5:a1-f9  2nd:gb:a1-fe,big5:40-7e,a1-fe */
#ifndef USE_BIG5
    if ((BYTE)string [pos] > 0xA0 && (BYTE)string [pos + 1] > 0xA0)
        return TRUE;
#else
    if ((BYTE)string [pos] > 0xA0)
    {
	if ( ((BYTE)string [pos + 1] >= 0x40 && (BYTE)string [pos + 1] <= 0x7e) ||
	     ((BYTE)string [pos + 1] >= 0xa1 && (BYTE)string [pos + 1] <= 0xfe)) {
	    /*fprintf(stderr,"true\n");
	    fflush(stderr);*/
	    return TRUE;
	}
    }
#endif

    return FALSE;
}

static void str2attr(const char* str,int len)
{
    int i=0;
    do
    {
	if (edtIsACCharAtPosition(str,len,i))
	{
		attr[i]=ATTCHL;
		attr[i+1]=ATTCHR;
		i+=2;
	}
	else
	{
		attr[i]=ATTENG;
		i++;
	}
    }while(i<len);
}

static BOOL edtIsACCharBeforePosition (const char* string,int len, int pos)
{
    if (pos < 2)
        return FALSE;

/* 1st:gb:a1-f7,big5:a1-f9  2nd:gb:a1-fe,big5:40-7e,a1-fe */
#ifndef USE_BIG5
    /* FIXME #ifdef GB2312?*/
    if ((BYTE)string [pos - 2] > 0xA0 && (BYTE)string [pos - 1] > 0xA0)
        return TRUE;
#else
#if 0
    if ((BYTE)string [pos - 2] > 0xA0)
    {
	if ( ((BYTE)string [pos - 1] >= 0x40 && (BYTE)string[pos - 1] <= 0x7e) ||
	     ((BYTE)string [pos - 1] >= 0xa1 && (BYTE)string[pos - 1] <= 0xfe))
            return TRUE;
    }
#else
    str2attr(string,len);
    if (attr[pos-1]==ATTENG) return FALSE;
    else return TRUE;
#endif
#endif

    return FALSE;
}


static BOOL edtIsACCharFromBegin(const char* string,int len,int pos)
{
	int i;
	if(pos == 0)
		return TRUE;
	if(len == 0)
		return FALSE;
	for(i=0;i<len;)
	{
		if( edtIsACCharAtPosition(string,len,i) )
			i += 2;
		else 
			i++;
		if(i==pos)
			return TRUE;
	}
	return FALSE;
}

int GetRETURNPos(char *str)
{
	int i;
	for(i=0;i<strlen(str);i++)
	{	
		if(str[i]==10)
			return i;
	}
	return -1;
}

void MLEditInitBuffer (PMLEDITDATA pMLEditData,char *spcaption)
{
	char *caption=spcaption; 
    int off1;
	int lineNO=0;
	PLINEDATA  pLineData;
	if (!(pMLEditData->head = malloc (sizeof (LINEDATA)))) {
		fprintf (stderr, "EDITLINE: malloc error!\n");
		return ;
	}
	pMLEditData->head->previous = NULL;
	pLineData=pMLEditData->head;	
	while ( (off1 = GetRETURNPos(caption)) != -1)
	{
		off1 = min(off1, LEN_MLEDIT_BUFFER);
		memcpy(pLineData->buffer,caption,off1);
		pLineData->buffer[off1] = '\0';
		caption+=min(off1,LEN_MLEDIT_BUFFER)+1;
		pLineData->lineNO  = lineNO;
		pMLEditData->dispPos = 0;
		pLineData->dataEnd = strlen(pLineData->buffer); 
		pLineData->next    = malloc (sizeof (LINEDATA));
		pLineData->next->previous = pLineData; 
		pLineData = pLineData->next;
		lineNO++;
	}	
	off1 = min(strlen(caption),LEN_MLEDIT_BUFFER);
	memcpy(pLineData->buffer,caption,off1);
	pLineData->buffer[off1] = '\0';
	pLineData->lineNO  = lineNO++;
	pMLEditData->dispPos = 0;
	pLineData->dataEnd = strlen(pLineData->buffer); 
	pLineData->next    = NULL; 
	pMLEditData->lines      = lineNO ; 
}

PLINEDATA GetLineData(PMLEDITDATA pMLEditData,int lineNO)
{
	PLINEDATA pLineData=pMLEditData->head;
	while(pLineData)
	{
		if(pLineData->lineNO==lineNO)
			return pLineData;
		pLineData = pLineData->next;
	}
	return NULL;
}

int MLEditCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{   
    DWORD       dwStyle;
	DWORD 		dw;
    HDC         hdc;
	PLINEDATA  pLineData;
	RECT		clientRect;
    PMLEDITDATA pMLEditData;
    dwStyle     = GetWindowStyle(hWnd);

    switch (message)
    {
        case WM_CREATE:
	{
            if (!(pMLEditData = malloc (sizeof (MLEDITDATA)))) {
                fprintf (stderr, "EDIT: malloc error!\n");
                return -1;
            }

            pMLEditData->totalLen      = LEN_MLEDIT_BUFFER;
            pMLEditData->editPos        = 0;
            pMLEditData->editLine       = 0;
            pMLEditData->caretPos       = 0;

	    MLEditInitBuffer(pMLEditData,GetWindowCaption(hWnd));	

	    GetClientRect(hWnd,&clientRect);            
	    pMLEditData->MaxlinesDisp   = (clientRect.bottom-clientRect.top)/GetSysCharHeight(hWnd);
	    pMLEditData->linesDisp		= min(pMLEditData->MaxlinesDisp,pMLEditData->lines);
	    pMLEditData->StartlineDisp  = 0;
	    pMLEditData->EndlineDisp    = pMLEditData->StartlineDisp + pMLEditData->linesDisp - 1;

            pMLEditData->selStartPos       = 0;
            pMLEditData->selEndPos         = 0;
            pMLEditData->passwdChar     = '*';
            pMLEditData->leftMargin     = MARGIN_MEDIT_LEFT;
            pMLEditData->topMargin      = MARGIN_MEDIT_TOP;
            pMLEditData->rightMargin    = MARGIN_MEDIT_RIGHT;
            pMLEditData->bottomMargin   = MARGIN_MEDIT_BOTTOM;

            pMLEditData->hardLimit      = -1;
            
            /* undo information */
            pMLEditData->lastOp         = MEDIT_OP_NONE;
            pMLEditData->lastPos        = 0;
            pMLEditData->affectedLen    = 0;
            pMLEditData->undoBufferLen  = LEN_MLEDIT_UNDOBUFFER;
            pMLEditData->undoBuffer [0] = '\0';
			SetWindowAdditionalData2(hWnd,(DWORD)pMLEditData);
			SetWindowAdditionalData(hWnd,(DWORD)0);
            break;
	}
        case WM_DESTROY:
        {
		PLINEDATA temp;
		pMLEditData =(PMLEDITDATA) GetWindowAdditionalData2(hWnd);
	    	DestroyCaret ();
		pLineData = pMLEditData->head;	
		while(pLineData)
		{
			/*printf("delete lineNO = %d,buffer=%s\n",pLineData->lineNO,pLineData->buffer);*/
			temp = pLineData->next;
			free(pLineData);
			pLineData = temp;
		}		
            	free(pMLEditData); 
	}
        break;
        
        case WM_SETFONT:
        break;
        
        case WM_GETFONT:
        break;

#if 0	/* fix: no WM_SETCURSOR */
        case WM_SETCURSOR:
            if (dwStyle & WS_DISABLED) {
                SetCursor (GetSystemCursor (IDC_ARROW));
                return 0;
            }
        break;

   	case WM_SIZECHANGED:
        {
        }
        return 0;
#endif
        case WM_KILLFOCUS:
	{
	    dw= GetWindowAdditionalData(hWnd);
            dw&= ~EST_FOCUSED;
	    SetWindowAdditionalData(hWnd,dw);

            HideCaret (hWnd);
	    DestroyCaret ();

            SendMessage (GetParent (hWnd), 
                         WM_COMMAND, 
                         (WPARAM) MAKELONG (GetDlgCtrlID(hWnd), EN_KILLFOCUS),
                         (LPARAM)hWnd);
	}
        break;
        
        case WM_SETFOCUS:
		{
			dw= GetWindowAdditionalData(hWnd);
            if (dw & EST_FOCUSED)
                return 0;
            
            dw |= EST_FOCUSED;
			SetWindowAdditionalData(hWnd,dw);

            pMLEditData =(PMLEDITDATA) GetWindowAdditionalData2(hWnd);

            /* only implemented for ES_LEFT align format. */

            CreateCaret (hWnd, NULL, 1, /*GetSysCharWidth(hWnd)+1,*/
		    hWnd->clirect.bottom-hWnd->clirect.top-2);
            SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                    + pMLEditData->leftMargin, pMLEditData->topMargin);
	    ShowCaret(hWnd);

            SendMessage (GetParent (hWnd), 
                         WM_COMMAND,
                         (WPARAM) MAKELONG (GetDlgCtrlID(hWnd), EN_SETFOCUS),
                         (LPARAM) hWnd);
		}
        break;
        
        case WM_ENABLE:
            if ( (!(dwStyle & WS_DISABLED) && !wParam)
                    || ((dwStyle & WS_DISABLED) && wParam) ) {
                if (wParam)
			ExcludeWindowStyle(hWnd,WS_DISABLED);
                else
			IncludeWindowStyle(hWnd,WS_DISABLED);

                InvalidateRect (hWnd, NULL, FALSE);
            }
        return 0;

        case WM_NCPAINT:
	{
	    RECT rc;
#if 0
            if (wParam)
                hdc = (HDC)wParam;
            else
                hdc = GetDC (hWnd);
#if 0	/* fix: no ClipRectIntersect() */
            if (lParam)
            ClipRectIntersect (hdc, (RECT*)lParam);
#endif
#else
            hdc = wParam? (HDC)wParam: GetWindowDC (hWnd);
	    GetWindowRect(hWnd, &rc);
#endif
            if (dwStyle & WS_BORDER)
	    {
#if 0
		RECT rc;
		GetWindowRect(hWnd,&rc);
                Draw3DDownFrame (hdc, 0, 0, 
                                      rc.right - rc.left - 1, 
                                      rc.bottom - rc.top - 1,
                                      PIXEL_invalid);
#else
		Draw3dInset(hdc, rc.left, rc.top,
			rc.right-rc.left, rc.bottom-rc.top);
#endif
	    }
            if (!wParam) {
		ReleaseDC(hWnd,hdc);
		}
	}
        return 0;

        case WM_PAINT:
        {
            int     dispLen,i;
            char*   dispBuffer;
            RECT    rect,rc;
	    PAINTSTRUCT ps;
	    HGDIOBJ oldfont=NULL;
            
            hdc = BeginPaint (hWnd,&ps);
            GetClientRect (hWnd, &rect);
    
            if (dwStyle & WS_DISABLED)
            {
		rc.left=0; rc.top=0; rc.bottom=rect.bottom; rc.right=rect.right;
		FillRect(hdc,&rc,GetStockObject(LTGRAY_BRUSH));
                SetBkColor (hdc, LTGRAY/*PIXEL_lightgray*/);
            }
            else {
		rc.left=0; rc.top=0; rc.bottom=rect.bottom; rc.right=rect.right;
		FillRect(hdc,&rc,GetStockObject(WHITE_BRUSH));
                SetBkColor (hdc, WHITE/*PIXEL_lightwhite*/);
            }

            SetTextColor (hdc, BLACK/*PIXEL_black*/);

            pMLEditData =(PMLEDITDATA) GetWindowAdditionalData2(hWnd);
			for(i = pMLEditData->StartlineDisp; i <= pMLEditData->EndlineDisp; i++)
			{
				pLineData= GetLineData(pMLEditData,i);
            	dispLen = edtGetDispLen (hWnd,pLineData);
         	    if (dispLen == 0 && pMLEditData->EndlineDisp >= pMLEditData->lines) {
                	continue;
           		 }

#ifdef _DEBUG
       	    	if (pMLEditData->dispPos > pLineData->dataEnd)
        	        fprintf (stderr, "ASSERT failure: %s.\n", "Edit Paint");
#endif
            
                dispBuffer = alloca (LEN_MLEDIT_BUFFER+1);

                if (dwStyle & ES_PASSWORD)
                    memset (dispBuffer, '*', pLineData->dataEnd);
				    memcpy (dispBuffer, 
                   	    pLineData->buffer,	/* +pMLEditData->dispPos, */
						pLineData->dataEnd);	/* - pMLEditData->dispPos); */
               		dispBuffer[pLineData->dataEnd] = '\0';

            /* only implemented ES_LEFT align format for single line edit. */
                rect.left = pMLEditData->leftMargin;
                rect.top = pMLEditData->topMargin ;
                rect.right = pMLEditData->rightMargin;
                rect.bottom = pMLEditData->bottomMargin;
#if 0
		printf("lineNO=%d,lines=%d,editLine=%d\n",pLineData->lineNO,pMLEditData->lines,
			pMLEditData->editLine);
		printf("--dispBuffer=%s--\n",dispBuffer);
                ClipRectIntersect (hdc, &rect);	/* fix: no ClipRectIntersect() */
#endif

#ifdef USE_BIG5	    
	    oldfont=SelectObject(hdc,CreateFont(12,
			0,0,0,0,0,0,0,0,0,0,0,
			FF_DONTCARE|DEFAULT_PITCH,
			"HZXFONT"));
#endif
                TextOut (hdc, 
				pMLEditData->leftMargin - pMLEditData->dispPos * GetSysCharWidth(hWnd) ,
				GetSysCharHeight(hWnd)*(pLineData->lineNO - pMLEditData->StartlineDisp) 
					+ pMLEditData->topMargin,
				dispBuffer,-1);
			}
#ifdef USE_BIG5	    
    	    DeleteObject(SelectObject(hdc,oldfont));
#endif
	    EndPaint (hWnd, &ps);
        }
        break;

        case WM_KEYDOWN:
        {
            BOOL    bChange = FALSE;
            int     i;
            int     deleted;
			PLINEDATA temp = NULL;
			char *  tempP = NULL;

            pMLEditData =(PMLEDITDATA) GetWindowAdditionalData2(hWnd);
        
            switch (LOWORD (wParam))
            {
				
                case VK_RETURN: 	/* SCANCODE_ENTER: */
				{
					pLineData = GetLineData(pMLEditData,pMLEditData->editLine);
					if (pMLEditData->editPos < pLineData->dataEnd)
						tempP = pLineData->buffer + pMLEditData->editPos;
				    temp = pLineData->next;
					pLineData->next = malloc( sizeof(LINEDATA) );
					pLineData->next->previous = pLineData;
					pLineData->next->next = temp;
					if(temp)
					{
						temp->previous = pLineData->next;
					}
					temp = pLineData->next;
					temp->lineNO  = pMLEditData->editLine + 1;
					if(tempP)
					{
						memcpy(temp->buffer,tempP,strlen(tempP));
						temp->dataEnd = strlen(tempP);
					}
					else
						temp->dataEnd = 0;
					temp->buffer[temp->dataEnd] = '\0'; 
					pLineData->dataEnd = pMLEditData->editPos;
					pLineData->buffer[pLineData->dataEnd]='\0';
					temp = temp->next;
					while (temp)
					{
						temp->lineNO++;
						temp = temp->next;
					}
					pMLEditData->editPos = 0;
					pMLEditData->caretPos= 0;
					pMLEditData->dispPos = 0;
					if(pMLEditData->linesDisp < pMLEditData->MaxlinesDisp)
					{
						pMLEditData->EndlineDisp++;
						pMLEditData->linesDisp++;
					}
					else if(pMLEditData->editLine == pMLEditData->EndlineDisp) 
					{
						pMLEditData->StartlineDisp++;
						pMLEditData->EndlineDisp++;
					}
					pMLEditData->editLine++;
					pMLEditData->lines++;
                    SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                            + pMLEditData->leftMargin, 
                        (pMLEditData->editLine - pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
							+pMLEditData->topMargin);
    	            InvalidateRect (hWnd, NULL, FALSE);
        	        return 0;
				}
                case VK_HOME: 	/* SCANCODE_HOME: */
				{
					PLINEDATA temp;
                    if (pMLEditData->editPos == 0)
                        return 0;

                    pMLEditData->editPos  = 0;
                    pMLEditData->caretPos = 0;

                    SetCaretPos (pMLEditData->leftMargin, 
                        (pMLEditData->editLine-pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
							+pMLEditData->topMargin);
					temp = GetLineData(pMLEditData,pMLEditData->editLine);	
                    if (pMLEditData->dispPos != 0)
					{
						pMLEditData->dispPos = 0;
					    InvalidateRect (hWnd, NULL, FALSE);
					}
               		return 0;
            	}
                case VK_END: /* SCANCODE_END: */
                {
                    int newStartPos;
                    pLineData = GetLineData(pMLEditData,pMLEditData->editLine);
                    if (pMLEditData->editPos == pLineData->dataEnd)
                        return 0;
                    newStartPos = edtGetStartDispPosAtEnd (hWnd, pLineData);
                    
                    pMLEditData->editPos = pLineData->dataEnd;
                    pMLEditData->caretPos = pMLEditData->editPos - newStartPos;
                    
                    SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd)
                            + pMLEditData->leftMargin, 
						(pMLEditData->editLine-pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
	                        + pMLEditData->topMargin);

                    if (pMLEditData->dispPos != newStartPos)
                        InvalidateRect (hWnd, NULL, FALSE);
					pMLEditData->dispPos = newStartPos;
                }
                return 0;

                case VK_LEFT: /* SCANCODE_CURSORBLOCKLEFT: */
                {
                    BOOL bScroll = FALSE;
                    int  scrollStep,newStartPos;
					PLINEDATA temp;
					pLineData = GetLineData(pMLEditData,pMLEditData->editLine);
                    if (pMLEditData->editPos == 0 )
					{
						temp = pLineData->previous;
						if(temp && pMLEditData->editLine > pMLEditData->StartlineDisp)
						{
							pMLEditData->editLine --;						
							pMLEditData->editPos = temp->dataEnd; 
		                    newStartPos = edtGetStartDispPosAtEnd (hWnd, temp);
                    		pMLEditData->caretPos = pMLEditData->editPos - newStartPos;
                    		if (pMLEditData->dispPos != newStartPos)
							{
								pMLEditData->dispPos = newStartPos;
								bScroll = TRUE;
							}
						}
						else
	                        return 0;
					}
					else
                    {	if (edtIsACCharBeforePosition (pLineData->buffer, 
                                    pLineData->dataEnd,
                        	    pMLEditData->editPos)) {
                        	scrollStep = 2;
                        	pMLEditData->editPos -= 2;
                    	}
                    	else {
                       	 	scrollStep = 1;
                        	pMLEditData->editPos --;
                    	}

                    	pMLEditData->caretPos -= scrollStep;
                    	if (pMLEditData->caretPos == 0 
                        	    && pMLEditData->editPos != 0) {

                        	bScroll = TRUE;

                        	if (edtIsACCharBeforePosition (pLineData->buffer, 
                                    pLineData->dataEnd,
                            	    pMLEditData->editPos)) {
                           		pMLEditData->dispPos -= 2;
                            	pMLEditData->caretPos = 2;
                        	}
                        	else {
								pMLEditData->dispPos--;							
	                            pMLEditData->caretPos = 1;
    	                    }
        	            }
            	        else if (pMLEditData->caretPos < 0) {
							pMLEditData->dispPos = 0;
                    	    pMLEditData->caretPos = 0;
                    	}
                    }    
                    SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                            + pMLEditData->leftMargin, 
						(pMLEditData->editLine - pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
                        	+ pMLEditData->topMargin);

                    if (bScroll)
                        InvalidateRect (hWnd, NULL, FALSE);
                }
                return 0;
                
		case VK_RIGHT: /* SCANCODE_CURSORBLOCKRIGHT: */
                {
                    BOOL bScroll = FALSE;
                    int  scrollStep, moveStep;
					PLINEDATA temp;

					pLineData = GetLineData(pMLEditData,pMLEditData->editLine);
                    if (pMLEditData->editPos == pLineData->dataEnd)
					{
						temp = pLineData->next;
						if(temp)
						{
							pMLEditData->editLine++;
							pMLEditData->editPos  = 0;
							pMLEditData->caretPos = 0;	
							if(pMLEditData->dispPos !=0)
							{
								pMLEditData->dispPos  = 0;
								bScroll = TRUE;
							}
						}
						else
	                        return 0;
					}
					else
                    {	
						if (edtIsACCharAtPosition (pLineData->buffer, 
                        	        pLineData->dataEnd,
                            	    pMLEditData->dispPos)) {
              	            if (edtIsACCharAtPosition (pLineData->buffer, 
                                    pLineData->dataEnd,
                                    pMLEditData->editPos)) {
                      	        scrollStep = 2;
                        	    moveStep = 2;
                            	pMLEditData->editPos  += 2;
                        	}
                        	else {
                            	scrollStep = 2;
	                            moveStep = 1;
    	                        pMLEditData->editPos ++;
        	                }
            	        }
                	    else {
                    	    if (edtIsACCharAtPosition (pLineData->buffer, 
                        	            pLineData->dataEnd,
                            	        pMLEditData->editPos)) {
                                    
	                            if (edtIsACCharAtPosition (pLineData->buffer, 
    	                                pLineData->dataEnd,
        	                            pMLEditData->dispPos + 1))
            	                    scrollStep = 3;
                	            else
                    	            scrollStep = 2;
	
    	                        moveStep = 2;
        	                    pMLEditData->editPos += 2;
            	            }
                	        else {
                    	        scrollStep = 1;
                        	    moveStep = 1;
                            	pMLEditData->editPos ++;
	                        }
    	                }

        	            pMLEditData->caretPos += moveStep;
            	        if (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                	            > edtGetOutWidth (hWnd)) {
                    	    bScroll = TRUE;
                        	pMLEditData->dispPos += scrollStep;
	                        pMLEditData->caretPos = 
    	                        pMLEditData->editPos - pMLEditData->dispPos;
        	            }
					}
                    SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                            + pMLEditData->leftMargin, 
                        (pMLEditData->editLine - pMLEditData->StartlineDisp) * GetSysCharHeight (hWnd) 
                        	+ pMLEditData->topMargin);

                    if (bScroll)
                        InvalidateRect (hWnd, NULL, FALSE);
                }
                return 0;
               	
		case VK_UP: /* SCANCODE_CURSORBLOCKUP: */
		{
                    BOOL bScroll = FALSE;
                    int  newStartPos;
					PLINEDATA temp;
					pLineData = GetLineData(pMLEditData,pMLEditData->editLine);
					temp = pLineData->previous; 
					if(pMLEditData->editLine == 0)
						return 0;
					else if (pMLEditData->editLine == pMLEditData->StartlineDisp)
					{
						bScroll = TRUE;
						pMLEditData->StartlineDisp--;
						pMLEditData->EndlineDisp--;	
					}
					pMLEditData->editLine--;

					if( pMLEditData->editPos >= temp->dataEnd ) 
					{
						pMLEditData->editPos = temp->dataEnd;
						pMLEditData->dispPos = 0;
	                   	newStartPos = edtGetStartDispPosAtEnd (hWnd, temp);
               			pMLEditData->dispPos =  newStartPos;
						pMLEditData->caretPos = pMLEditData->editPos - newStartPos;
						bScroll = TRUE;
					}				
					else 
					{
       			   		newStartPos = edtGetOffset(hWnd, pMLEditData,temp,
                   				pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                       			+ pMLEditData->leftMargin);
						if(!edtIsACCharFromBegin(temp->buffer,temp->dataEnd,
								pMLEditData->dispPos))
						{
							bScroll = TRUE;
							pMLEditData->dispPos--;
        			   		newStartPos = edtGetOffset(hWnd, pMLEditData,temp,
                      				pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                          			+ pMLEditData->leftMargin+GetSysCharWidth(hWnd)/2);
						}
	   	        	    pMLEditData->editPos = newStartPos + pMLEditData->dispPos;
   		    	        pMLEditData->caretPos = newStartPos;
					}
                    SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                            + pMLEditData->leftMargin, 
					    (pMLEditData->editLine - pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
                            + pMLEditData->topMargin);
					if(bScroll)
						InvalidateRect(hWnd,NULL,FALSE);	
				}
				break;
		case VK_DOWN: /* SCANCODE_CURSORBLOCKDOWN: */
		{
                    BOOL bScroll = FALSE;
                    int  newStartPos;
					PLINEDATA temp;
					pLineData = GetLineData(pMLEditData,pMLEditData->editLine);
					temp = pLineData->next; 
					if(pMLEditData->editLine == pMLEditData->lines-1)
						return 0;
					else if (pMLEditData->editLine == pMLEditData->EndlineDisp)
					{
						bScroll = TRUE;
						pMLEditData->StartlineDisp++;
						pMLEditData->EndlineDisp++;	
					}
					pMLEditData->editLine++;

					if( pMLEditData->editPos >= temp->dataEnd ) 
					{
						pMLEditData->editPos = temp->dataEnd;
						pMLEditData->dispPos = 0;
	                   	newStartPos = edtGetStartDispPosAtEnd (hWnd, temp);
               			pMLEditData->dispPos =  newStartPos;
						pMLEditData->caretPos = pMLEditData->editPos - newStartPos;
						bScroll = TRUE;
					}				
					else 
					{
       			   		newStartPos = edtGetOffset(hWnd, pMLEditData,temp,
                   				pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                       			+ pMLEditData->leftMargin);
						if(!edtIsACCharFromBegin(temp->buffer,temp->dataEnd,
								pMLEditData->dispPos))
						{
							bScroll = TRUE;
							pMLEditData->dispPos--;
           			   		newStartPos = edtGetOffset(hWnd, pMLEditData,temp,
                       				pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                           			+ pMLEditData->leftMargin+GetSysCharWidth(hWnd)/2);
						}
	   	        	    pMLEditData->editPos = newStartPos + pMLEditData->dispPos;
   		    	        pMLEditData->caretPos = newStartPos;
					}
                    SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                            + pMLEditData->leftMargin, 
					    (pMLEditData->editLine - pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
                            + pMLEditData->topMargin);
					if(bScroll)
						InvalidateRect(hWnd,NULL,FALSE);	
		
				}
				break;
                case VK_INSERT: /* SCANCODE_INSERT: */
					dw = GetWindowAdditionalData(hWnd);	
                    dw ^= EST_REPLACE;
					SetWindowAdditionalData(hWnd,dw);
                break;

                case VK_DELETE: /* SCANCODE_REMOVE: */
				{
					PLINEDATA temp;
					int leftLen;
					pLineData = GetLineData(pMLEditData,pMLEditData->editLine);
                    if ((GetWindowAdditionalData(hWnd) & EST_READONLY) ){
#if 0	/* fix: no ping() */
                        ping ();
#endif
                        return 0;
                    }
                   	temp = pLineData->next; 
					if (pLineData->dataEnd == pMLEditData->editPos && temp)
					{
						if(pLineData->dataEnd + temp->dataEnd <= LEN_MLEDIT_BUFFER)
						{
							memcpy(pLineData->buffer+pLineData->dataEnd,temp->buffer,temp->dataEnd);	
							pLineData->dataEnd += temp->dataEnd;
							pLineData->buffer[pLineData->dataEnd] = '\0';
							if(temp->next)
							{
							    pLineData->next = temp->next;
								temp->next->previous = pLineData;	
							}
							else
								pLineData->next = NULL;
							free(temp);
							temp = pLineData->next;
							while (temp)
							{
								temp->lineNO--;
								temp = temp->next;
							}
							if(pMLEditData->lines <= pMLEditData->MaxlinesDisp)
							{
								pMLEditData->EndlineDisp--;
								pMLEditData->linesDisp--;
							}
							if(pMLEditData->EndlineDisp >= pMLEditData->lines-1)		
							{
								pMLEditData->EndlineDisp--;
								if(pMLEditData->StartlineDisp !=0)
									pMLEditData->StartlineDisp--;
								else
									pMLEditData->linesDisp--;
							}
							pMLEditData->lines--;
						}
						else if (temp->dataEnd > 0)
						{
							leftLen = LEN_MLEDIT_BUFFER - pLineData->dataEnd;
							memcpy(pLineData->buffer+pLineData->dataEnd,temp->buffer,leftLen);
							pLineData->dataEnd +=leftLen;
							pLineData->buffer[pLineData->dataEnd] = '\0';
							memcpy(temp->buffer,temp->buffer+leftLen,temp->dataEnd-leftLen);  
							temp->dataEnd -=leftLen;
							temp->buffer[temp->dataEnd] = '\0';
						}
					}
					else if (pMLEditData->editPos != pLineData->dataEnd)
					{	
	                    if (edtIsACCharAtPosition (pLineData->buffer, 
    	                                pLineData->dataEnd,
        	                            pMLEditData->editPos))
	                        deleted = 2;
    	                else
        	                deleted = 1;
                        
            	        for (i = pMLEditData->editPos; 
                	            i < pLineData->dataEnd - deleted;
                    	        i++)
 	                       pLineData->buffer [i] 
    	                        = pLineData->buffer [i + deleted];

        	            pLineData->dataEnd -= deleted;
						pLineData->buffer[pLineData->dataEnd] = '\0';
					}
                	bChange = TRUE;
                    InvalidateRect (hWnd, NULL,FALSE);
				}
                break;

                case VK_BACK: /* SCANCODE_BACKSPACE: */
				{
					PLINEDATA temp;
					int leftLen,tempEnd;
                    if ((GetWindowAdditionalData(hWnd) & EST_READONLY) ){
#if 0	 /* fix: no Ping() */
                        Ping ();
#endif
                        return 0;
                    }
					pLineData = GetLineData(pMLEditData,pMLEditData->editLine);
					temp = pLineData->previous;	
					if (pMLEditData->editPos == 0 && temp)
					{
						tempEnd = temp->dataEnd;
						if(pLineData->dataEnd + temp->dataEnd <= LEN_MLEDIT_BUFFER)	
						{
							memcpy(temp->buffer+temp->dataEnd,pLineData->buffer,pLineData->dataEnd);	
							temp->dataEnd +=pLineData->dataEnd;
							temp->buffer[temp->dataEnd] = '\0';
							if(pLineData->next)
							{
							    temp->next = pLineData->next;
								pLineData->next->previous = temp;
							}
							else
								temp->next = NULL;
							free(pLineData);
							pLineData = temp;
							temp = temp->next;
							while(temp)
							{
								temp->lineNO--;
								temp = temp->next;
							}
							if(pMLEditData->StartlineDisp == pMLEditData->editLine
									&& pMLEditData->StartlineDisp != 0)
							{
								pMLEditData->StartlineDisp--;	
								if(pMLEditData->EndlineDisp == pMLEditData->lines)
									pMLEditData->EndlineDisp--;
							}
							if(pMLEditData->lines <= pMLEditData->MaxlinesDisp)
							{
								pMLEditData->linesDisp--;
								pMLEditData->EndlineDisp--;
							}
							pMLEditData->lines--;
						}
						else if (pLineData->dataEnd > 0)
						{
							leftLen = LEN_MLEDIT_BUFFER - temp->dataEnd;
							memcpy(temp->buffer+temp->dataEnd,pLineData->buffer,leftLen);
							temp->dataEnd +=leftLen;
							temp->buffer[temp->dataEnd] = '\0';
							memcpy(pLineData->buffer,pLineData->buffer+leftLen,pLineData->dataEnd-leftLen);  
							pLineData->dataEnd -=leftLen;
							pLineData->buffer[pLineData->dataEnd] = '\0';
						}
						pMLEditData->editLine--;
						pMLEditData->editPos = tempEnd;
						pMLEditData->dispPos = tempEnd;
						/* 当编辑位置不为0,caret位置为0的时候,移动caret位置. */
           		        if (pMLEditData->caretPos == 0 
                	            && pMLEditData->editPos != 0) {
        	                if (edtIsACCharBeforePosition (pLineData->buffer, 
                                    pLineData->dataEnd,
            	                    pMLEditData->editPos)) {
                	           	pMLEditData->dispPos -= 2;
                    	        pMLEditData->caretPos = 2;
                   		    }
                        	else {
								pMLEditData->dispPos--;							
                           		pMLEditData->caretPos = 1;
                    	    }
                   	 	}
                    	else if (pMLEditData->caretPos < 0) {
							pMLEditData->dispPos = 0;
                        	pMLEditData->caretPos = 0;
                    	}
					}
					else if (pMLEditData->editPos != 0 )
					{
	                    if (edtIsACCharBeforePosition (pLineData->buffer, 
                                        pLineData->dataEnd,
    	                                pMLEditData->editPos))
        	                deleted = 2;
            	        else
                	        deleted = 1;
                        
    	                for (i = pMLEditData->editPos; 
        	                    i < pLineData->dataEnd;
            	                i++)
                	        pLineData->buffer [i - deleted] 
                    	        = pLineData->buffer [i];

  	    	            pLineData->dataEnd -= deleted;
    	                pMLEditData->editPos -= deleted;
                	    pMLEditData->caretPos -= deleted;
	                    if (pMLEditData->caretPos == 0 
    	                        && pMLEditData->editPos != 0) {
        	                if (edtIsACCharBeforePosition (pLineData->buffer, 
                                    pLineData->dataEnd,
            	                    pMLEditData->editPos)) {
                	            pMLEditData->dispPos -= 2;
                    	        pMLEditData->caretPos = 2;
       		                 }
            	            else {
                	            pMLEditData->dispPos --;
                    	        pMLEditData->caretPos = 1;
                    	    }
                        
                	    }
					}
            	    bChange = TRUE;
                    SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                            + pMLEditData->leftMargin, 
					    (pMLEditData->editLine - pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
                            + pMLEditData->topMargin);
                    InvalidateRect (hWnd, NULL, FALSE);
				}
                break;

                default:
                break;
            }
       
            if (bChange)
                SendMessage (GetParent (hWnd), WM_COMMAND, 
                        (WPARAM) MAKELONG (GetDlgCtrlID(hWnd), EN_CHANGE),
                        (LPARAM) hWnd);
            return 0;
        }

        case WM_CHAR:
        {
            char charBuffer [2];
            int  i, chars, scrollStep, inserting;
			UINT format;
	
            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 

			pLineData = GetLineData(pMLEditData,pMLEditData->editLine);

            if (dwStyle & ES_READONLY) {
#if 0	 /* fix: no Ping() */
                Ping ();
#endif
                return 0;
            }
            if (HIBYTE (wParam)) {
                charBuffer [0] = LOBYTE (wParam);
                charBuffer [1] = HIBYTE (wParam);
                chars = 2;
            }
            else {
                charBuffer [0] = LOBYTE (wParam);
                chars = 1;
            }
            
            if (chars == 1) {
                switch (charBuffer [0])
                {
                    case 0x00:  /* NULL */
                    case 0x07:  /* BEL */
                    case 0x08:  /* BS */
                    case 0x09:  /* HT */ 
                    case 0x0A:  /* LF */
                    case 0x0B:  /* VT */
                    case 0x0C:  /* FF */
                    case 0x0D:  /* CR */
                    case 0x1B:  /* Escape */
                    return 0;
                }
            }
            if (GetWindowAdditionalData(hWnd) & EST_REPLACE) {
                if (pLineData->dataEnd == pMLEditData->editPos)
                    inserting = chars;
                else if (edtIsACCharAtPosition (pLineData->buffer, 
                                pLineData->dataEnd,
                                pMLEditData->editPos)) {
                    if (chars == 2)
                        inserting = 0;        
                    else
                        inserting = -1;       
                }
                else {
                    if (chars == 2)
                        inserting = 1;		  	
                    else
                        inserting = 0;        
                }
            }
            else
                inserting = chars;

            /* check space */
            if (pLineData->dataEnd + inserting > pMLEditData->totalLen) {
#if 0	/* fix no ping */
                Ping ();
#endif
                SendMessage (GetParent (hWnd), WM_COMMAND,
                            (WPARAM) MAKELONG (GetDlgCtrlID(hWnd), EN_MAXTEXT),
                            (LPARAM) hWnd);
                return 0;
            }
            else if ((pMLEditData->hardLimit >= 0) 
                        && ((pLineData->dataEnd + inserting) 
                            > pMLEditData->hardLimit)) {
#if 0	/* fix no ping */
                Ping ();
#endif
                SendMessage (GetParent (hWnd), WM_COMMAND,
                            (WPARAM) MAKELONG (GetDlgCtrlID(hWnd), EN_MAXTEXT),
                            (LPARAM) hWnd);
                return 0;
            }
            if (inserting == -1) {
                for (i = pMLEditData->editPos; i < pLineData->dataEnd-1; i++)
                    pLineData->buffer [i] = pLineData->buffer [i + 1];
            }
            else if (inserting > 0) {
                for (i = pLineData->dataEnd + inserting - 1; 
                        i > pMLEditData->editPos + inserting - 1; 
                        i--)
                    pLineData->buffer [i] 
                            = pLineData->buffer [i - inserting];
            }
            for (i = 0; i < chars; i++)
                    pLineData->buffer [pMLEditData->editPos + i] 
                        = charBuffer [i];
            
            pMLEditData->editPos += chars;
            pMLEditData->caretPos += chars;
            pLineData->dataEnd += inserting;
			pLineData->buffer[pLineData->dataEnd] = '\0';
            if (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                            > edtGetOutWidth (hWnd))
            {
                if (edtIsACCharAtPosition (pLineData->buffer, 
                                pLineData->dataEnd,
                                pMLEditData->dispPos))
                    scrollStep = 2;
                else {
                    if (chars == 2) {
                        if (edtIsACCharAtPosition (pLineData->buffer, 
                                pLineData->dataEnd,
                                pMLEditData->dispPos + 1))
                            scrollStep = 3;
                        else
                            scrollStep = 2;
                    }
                    else
                        scrollStep = 1;
                }
                    
                pMLEditData->dispPos += scrollStep;

                pMLEditData->caretPos = 
                            pMLEditData->editPos - pMLEditData->dispPos;

            }
            SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
                            + pMLEditData->leftMargin, 
					    (pMLEditData->editLine - pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
                            + pMLEditData->topMargin);
            InvalidateRect (hWnd, NULL,FALSE);
			format = DT_NOPREFIX;
            SendMessage (GetParent (hWnd), WM_COMMAND,
                    (WPARAM) MAKELONG (GetDlgCtrlID(hWnd), EN_CHANGE),
                    (LPARAM) hWnd);
        }
        return 0;

        case WM_GETTEXTLENGTH:
		{
			PLINEDATA temp;
			int    lineNO = (int)wParam;
            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 
			temp = pMLEditData->head;
			while(temp)
			{
				if (temp->lineNO == lineNO)
					return  temp->dataEnd;
				temp = temp->next;
			}
        return -1;
        }
		case WM_GETTEXT:
		{
			PLINEDATA temp;
			int len,total = 0,lineNO;
			char * buffer = (char*)lParam;
            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 
			len = (int)wParam;
            lineNO = (int)wParam;
			temp = pMLEditData->head;
			while (temp && total + temp->dataEnd < len)  
			{
				memcpy(buffer+total,temp->buffer,temp->dataEnd);
				total += temp->dataEnd;
				temp = temp->next;
			}	
					
		}
		return 0;
/* can i add it to message define ? */
#if 0
        case WM_GETLINETEXT:
        {
			PLINEDATA temp;
            char*   buffer = (char*)lParam;
            int     lineNO,len;

            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 
            lineNO = (int)wParam;
			temp = GetLineData(pMLEditData,lineNO);
			if(temp)
			{
				len = min ((int)wParam, temp->dataEnd);
		    	memcpy (buffer, temp->buffer,len);
		        buffer [len] = '\0';
				return 0;
			}
            return -1;
        }
        break;
		case WM_SETTEXT:
		{
			MLEditInitBuffer(pMLEditData,(char *)lParam);
		}
		return 0;
#endif
/* can i add it to message defined? */
#if 0
        case WM_SETLINETEXT:
        {
            int len,lineNO;
			PLINEDATA temp;

            if (dwStyle & ES_READONLY)
                return 0;

            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 
            
            len = strlen ((char*)lParam);
			lineNO = (int)wParam;
			temp = pMLEditData->head;
            len = min (len, pMLEditData->totalLen);
            
            if (pMLEditData->hardLimit >= 0)
                len = min (len, pMLEditData->hardLimit);
          	while (temp)
			{
				if(temp->lineNO == lineNO)
				{
     		        temp->dataEnd = len;
            	    memcpy (temp->buffer, (char*)lParam, len);
				}
				temp = temp->next;
			}
            pMLEditData->editPos        = 0;
            pMLEditData->caretPos       = 0;
            pMLEditData->dispPos        = 0;
            InvalidateRect (hWnd, NULL, FALSE);
        }
        return 0;
#endif
        case WM_LBUTTONDBLCLK:
        break;
        
        case WM_LBUTTONDOWN:
        {
            int newOff,lineNO;
			PLINEDATA temp;
			BOOL bScroll = FALSE;
            
            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 
            lineNO = edtGetLineNO (hWnd,pMLEditData, HIWORD (lParam));
			if ( lineNO < 0 )
				return 0;
			lineNO += pMLEditData->StartlineDisp;
			if (lineNO <= pMLEditData->EndlineDisp && lineNO <= pMLEditData->lines-1 )
			{
				temp = GetLineData(pMLEditData,lineNO);		
   		        newOff = edtGetOffset (hWnd,pMLEditData,temp, LOWORD (lParam));
				if(!edtIsACCharFromBegin(temp->buffer,temp->dataEnd,pMLEditData->dispPos))
				{
					bScroll = TRUE;
					pMLEditData->dispPos--;
   	        		newOff = edtGetOffset (hWnd,pMLEditData,temp, LOWORD(lParam)+GetSysCharWidth(hWnd)/2);
				}
    	        if (newOff != pMLEditData->caretPos || lineNO != pMLEditData->editLine) {
					pMLEditData->editLine = temp->lineNO;
   	        	    pMLEditData->editPos = newOff +pMLEditData->dispPos;
        	        pMLEditData->caretPos = newOff;
   	        	    SetCaretPos (pMLEditData->caretPos * GetSysCharWidth (hWnd) 
           	            + pMLEditData->leftMargin, 
					(pMLEditData->editLine - pMLEditData->StartlineDisp) * GetSysCharHeight(hWnd)
                	        	+ pMLEditData->topMargin);
           		}
				if(bScroll)
					InvalidateRect(hWnd,NULL,FALSE);
			}
        }
        break;

        case WM_LBUTTONUP:
        break;
        
        case WM_MOUSEMOVE:
        break;
        
        case WM_GETDLGCODE:
        return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;
        
        case EM_SETREADONLY:
            if (wParam)
				IncludeWindowStyle(hWnd,ES_READONLY);
            else
				ExcludeWindowStyle(hWnd,ES_READONLY);
        return 0;
        
        case EM_SETPASSWORDCHAR:
            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 

            if (pMLEditData->passwdChar != (int)wParam) {
                if (dwStyle & ES_PASSWORD) {
                    pMLEditData->passwdChar = (int)wParam;
                    InvalidateRect (hWnd, NULL, TRUE);
                }
            }
        return 0;
    
        case EM_GETPASSWORDCHAR:
        {
            int* passwdchar;
            
            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 
            passwdchar = (int*) lParam;

            *passwdchar = pMLEditData->passwdChar;
        }
        return 0;
    
        case EM_LIMITTEXT:
        {
            int newLimit = (int)wParam;
            
            if (newLimit >= 0) {
            pMLEditData = (PMLEDITDATA)GetWindowAdditionalData2(hWnd); 
                if (pMLEditData->totalLen < newLimit)
                    pMLEditData->hardLimit = -1;
                else
                    pMLEditData->hardLimit = newLimit;
            }
        }
        return 0;
    
        default:
    		return DefWindowProc (hWnd, message, wParam, lParam);
        break;
    } 

    return 0;	/* !DefaultControlProc (hWnd, message, wParam, lParam); */
}

