/*
 * Copyright (C) 1999, 2000, Wei Yongming.
 * Portions Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Listbox for Microwindows win32 api.
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
**  LISTBOX control is written by Wei Yongming from scratch.
**
** Modify records:
**
**  Who             When        Where       For What                Status
**-----------------------------------------------------------------------------
**  Wei Yongming    1999/10/18  Tsinghua    Item Additional Data    Finished
**  Wei Yongming    1999/10/31  Tsinghua    Space bar for checkmark Finished
**  Wei Yongming    1999/10/31  Tsinghua    Character match item    Finished
**  Wei Yongming    1999/11/07  Tsinghua    Character match item    Bug fixing
**  WEI Yongming    2000/01/20  Tsinghua    Thumb dragging          Finished
**  WEI Yongming    2000/02/24  Tsinghua    Add MPL License         Finished
**  Kevin Tseng     2000/05/26  gv          port to microwin        ported
**  Greg Haerr      2000/06/15  Utah        3d look, bug fixes      Finished
**  Kevin Tseng     2000/06/22  gv          port to mw-nanox        ported
**  Kevin Tseng     2000/06/22  gv          fixed bug if no item    Finished
**  Kevin Tseng     2000/08/08  gv          enable scrollbar(V)     porting
**  Kevin Tseng     2000/08/10  gv          enable scrollbar(V)     ported
**  Kevin Tseng     2000/08/10  gv          WM_CHAR, WM_KEYDOWN     ported
**
** TODO:
** 1. Multiple columns support.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MWINCLUDECOLORS
#include "windows.h"
#include "wintools.h" 	/* Draw3dBox */
#include "device.h" 	/* GdGetTextSize */

#define FixStrAlloc(n)	malloc((n)+1)
#define FreeFixStr(p)	free(p)

#define LBIF_NORMAL         0x0000L
#define LBIF_SELECTED       0x0001L
#define LBIF_CHECKED        0x0010L
#define LBIF_PARTCHECKED    0x0020L
#define LBIF_CHECKMARKMASK  0x00F0L

#define CMFLAG_BLANK        0
#define CMFLAG_CHECKED      1
#define CMFLAG_PARTCHECKED  2
typedef struct _LISTBOXITEMINFO {
    int     insPos;         /* insert position */
    char*   string;         /* item string */
    int     cmFlag;         /* check mark flag */
    HICON   hIcon;          /* handle of icon */
} LISTBOXITEMINFO, *PLISTBOXITEMINFO;

typedef struct _LISTBOXITEM {
    char*   key;                /* item sort key */
    DWORD   dwFlags;            /* item flags */
    DWORD   dwData;             /* item data */
    DWORD   dwAddData;          /* item additional data */
    struct  _LISTBOXITEM* next;  /* next item */
} LISTBOXITEM, *PLISTBOXITEM;

#define DEF_LB_BUFFER_LEN       5

#define LBF_FOCUS               0x0001
#define LBF_NOTHINGSELECTED	0x0002

typedef struct _LISTBOXDATA {
    DWORD dwFlags;          /* listbox flags */

    int itemCount;          /* items count */
    int itemTop;            /* start display item */
    int itemVisibles;       /* number of visible items */

    int itemHilighted;      /* current hilighted item */
    int itemHeight;         /* item height */

    LISTBOXITEM* head;      /* items linked list head */

    int buffLen;            /* buffer length */
    LISTBOXITEM* buffStart; /* buffer start */
    LISTBOXITEM* buffEnd;   /* buffer end */
    LISTBOXITEM* freeList;  /* free list in buffer */
} LISTBOXDATA, *PLISTBOXDATA;

void ListboxControlCleanup ();
static LRESULT CALLBACK
ListboxCtrlProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#define ITEM_BOTTOM(x)  (x->itemTop + x->itemVisibles - 1)

#define LST_WIDTH_CHECKMARK     11
#define LST_HEIGHT_CHECKMARK    11
#define LST_INTER_BMPTEXT       2

int WINAPI MwRegisterListboxControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;
#if 0
    static BITMAP sg_bmpCheckMark;
    if (!LoadSystemBitmap (&sg_bmpCheckMark, "checkmark")) {
        fprintf (stderr, "Load ListBox Check Mark Bitmap failure!\n");
        return FALSE;
    }
#endif
	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)ListboxCtrlProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground= GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "LISTBOX";

	return RegisterClass(&wc);
}

void ListboxControlCleanup ()
{
#if 0
    UnloadBitmap (&sg_bmpCheckMark);
#endif
}

static LRESULT NotifyParent (HWND hwnd, int id, int code)
{
    return SendMessage (GetParent (hwnd), WM_COMMAND, 
                 (WPARAM) MAKELONG (id, code), (LPARAM)hwnd);
}

static BOOL lstInitListBoxData (HWND hwnd,LISTBOXDATA* pData, int len)
{
    int i, xw, xh, xb;
    PLISTBOXITEM plbi;
    HDC hdc;
    
    memset (pData, 0, sizeof (LISTBOXDATA));
#if 0
    pData->itemHeight = GetSysCharHeight ();
#else
    hdc=GetDC(hwnd);
#if MWCLIENT	/* nanox client */
    GrSetGCFont(hdc->gc,hdc->font->fontid);
    GrGetGCTextSize(hdc->gc,"X",1,
		MWTF_ASCII,&xw,&xh,&xb);
#else
    GdSetFont(hdc->font->pfont);
    GdGetTextSize(hdc->font->pfont,"X",1,
		&xw,&xh,&xb,MWTF_ASCII);
#endif
    ReleaseDC(hwnd,hdc);
    pData->itemHeight=xh + 1; 
#endif
    pData->itemHilighted = 0;
    pData->dwFlags = LBF_NOTHINGSELECTED;

    /* init item buffer. */
    if (!(pData->buffStart = malloc (len * sizeof (LISTBOXITEM))))
        return FALSE;

    pData->buffLen = len;
    pData->buffEnd = pData->buffStart + len;
    pData->freeList = pData->buffStart;

    plbi = pData->freeList;
    for (i = 0; i < len - 1; i++) {
        plbi->next = plbi + 1;
        plbi ++;
    }
    plbi->next = NULL;

    return TRUE;
}

static void lstListBoxCleanUp (LISTBOXDATA* pData)
{
    PLISTBOXITEM plbi;
    PLISTBOXITEM next;

    plbi = pData->head;
    while (plbi) {
        FreeFixStr (plbi->key);
        next = plbi->next;
        if (plbi < pData->buffStart || plbi > pData->buffEnd)
            free (plbi);

        plbi = next;
    }
    
    free (pData->buffStart);
}

static void lstResetListBoxContent (PLISTBOXDATA pData)
{
    int i;
    PLISTBOXITEM plbi, next;

    pData->itemCount = 0;
    pData->itemTop = 0;
    pData->itemHilighted = 0;
#if 0
    pData->itemVisibles = 0;
#endif

    plbi = pData->head;
    while (plbi) {
        FreeFixStr (plbi->key);
        next = plbi->next;
        if (plbi < pData->buffStart || plbi > pData->buffEnd)
            free (plbi);

        plbi = next;
    }

    pData->head = NULL;
    pData->freeList = pData->buffStart;

    plbi = pData->freeList;
    for (i = 0; i < pData->buffLen - 1; i++) {
        plbi->next = plbi + 1;
        plbi ++;
    }
    plbi->next = NULL;
}

static PLISTBOXITEM lstAllocItem (PLISTBOXDATA pData)
{
    PLISTBOXITEM plbi;

    if (pData->freeList) {
        plbi = pData->freeList;
        pData->freeList = plbi->next;
    }
    else
        plbi = (PLISTBOXITEM) malloc (sizeof (LISTBOXITEM));
    
    return plbi;
}

static void lstFreeItem (PLISTBOXDATA pData, PLISTBOXITEM plbi)
{
    if (plbi < pData->buffStart || plbi > pData->buffEnd)
        free (plbi);
    else {
        plbi->next = pData->freeList;
        pData->freeList = plbi;
    }
}

static int lstAddNewItem (DWORD dwStyle, 
        PLISTBOXDATA pData, PLISTBOXITEM newItem, int pos)
{
    PLISTBOXITEM plbi;
    PLISTBOXITEM insPosItem = NULL;
    int insPos = 0;

    newItem->next = NULL;
    if (!pData->head)
        insPosItem = NULL;
    else if (dwStyle & LBS_SORT) {
        plbi = pData->head;

        if (strcmp (newItem->key, plbi->key) < 0) {
            insPosItem = NULL;
            insPos = 0;
        }
        else {
            while (plbi->next) {
                if (strcmp (newItem->key, plbi->next->key) <= 0)
                    break;
            
                plbi = plbi->next;
                insPos ++;
            }
            insPosItem = plbi;
        }
    }
    else {
        plbi = pData->head;

        if (pos < 0) {
            while (plbi->next) {
                plbi = plbi->next;
                insPos ++;
            }
            insPosItem = plbi;
        }
        else if (pos > 0) {
            int index = 0;

            while (plbi->next) {
                if (pos == index)
                    break;
                plbi = plbi->next;
                index ++;
                insPos ++;
            }
            insPosItem = plbi;
        }
    }

    if (insPosItem) {
        plbi = insPosItem->next;
        insPosItem->next = newItem;
        newItem->next = plbi;

        insPos ++;
    }
    else {
        plbi = pData->head;
        pData->head = newItem;
        newItem->next = plbi;
    }

    pData->itemCount ++;
    return insPos;
}

static PLISTBOXITEM lstRemoveItem (PLISTBOXDATA pData, int* pos)
{
    int index = 0;
    PLISTBOXITEM plbi, prev;

    if (!pData->head)
        return NULL;

    if (*pos < 0) {
        prev = pData->head;
        plbi = pData->head;
        while (plbi->next) {
            prev = plbi;
            plbi = plbi->next;
            index ++;
        }

        if (plbi == pData->head) {
            pData->head = pData->head->next;
            *pos = 0;
            return plbi;
        }
        else {
            prev->next = plbi->next;
            *pos = index;
            return plbi;
        }
    }
    else if (*pos == 0) {
        plbi = pData->head;
        pData->head = plbi->next;
        return plbi;
    }
    else {
        index = 0;
        prev = pData->head;
        plbi = pData->head;
        while (plbi->next) {
            if (*pos == index)
                break;

            prev = plbi;
            plbi = plbi->next;
            index ++;
        }

        if (plbi == pData->head) {
            pData->head = pData->head->next;
            *pos = 0;
            return plbi;
        }
        else {
            prev->next = plbi->next;
            *pos = index;
            return plbi;
        }
    }

    return NULL;
}

static void lstGetItemsRect (PLISTBOXDATA pData, int start, int end, RECT* prc)
{
    if (start < 0)
        start = 0;

    prc->top = (start - pData->itemTop)*pData->itemHeight;

    if (end >= 0)
        prc->bottom = (end - pData->itemTop + 1)*pData->itemHeight;

}

static void lstInvalidateItem (HWND hwnd, PLISTBOXDATA pData, int pos,BOOL fEBk)
{
    RECT rcInv;
    
    if (pos < pData->itemTop || pos > (pData->itemTop + pData->itemVisibles))
        return;
    
    GetClientRect (hwnd, &rcInv);
    rcInv.top = (pos - pData->itemTop)*pData->itemHeight;
    rcInv.bottom = rcInv.top + pData->itemHeight;

    InvalidateRect (hwnd, &rcInv, fEBk);
}

static BOOL lstInvalidateUnderItem (HWND hwnd, PLISTBOXDATA pData, int pos)
{
    RECT rcInv;
    
    if (pos > (pData->itemTop + pData->itemVisibles))
        return FALSE;

    if (pos <= pData->itemTop) {
        InvalidateRect (hwnd, NULL, TRUE);
        return TRUE;
    }
    
    GetClientRect (hwnd, &rcInv);

    lstGetItemsRect (pData, pos, -1, &rcInv);

    if (rcInv.top < rcInv.bottom)
        InvalidateRect (hwnd, &rcInv, TRUE);

    return TRUE;
}

static PLISTBOXITEM lstGetItem (PLISTBOXDATA pData, int pos)
{
    int i;
    PLISTBOXITEM plbi;

    plbi = pData->head;
    for (i=0; i < pos && plbi; i++)
        plbi = plbi->next;

    return plbi;
}

static int lstFindItem (PLISTBOXDATA pData, int start, char* key, BOOL bExact)
{
    PLISTBOXITEM plbi;
    int keylen = strlen (key);
  
    if (start >= (pData->itemCount - 1))
        start = 0;

    plbi = lstGetItem (pData, start);

    while (plbi)
    {
        if (bExact && (keylen != strlen (plbi->key))) {
            plbi = plbi->next;
            start ++;
            continue;
        }

        if (strncasecmp (key, plbi->key, keylen) == 0)
            return start;
            
        plbi = plbi->next;
        start ++;
    }

    return LB_ERR;
}

static void lstOnDrawSListBoxItems (HDC hdc, DWORD dwStyle, 
                PLISTBOXDATA pData, int width)
{
    PLISTBOXITEM plbi;
    int i;
    int x = 0, y = 0;
    int offset;
    RECT rc;
    COLORREF bk;

    plbi = lstGetItem (pData, pData->itemTop);
    
    for (i = 0; plbi && i < (pData->itemVisibles + 1); i++) {

        if (plbi->dwFlags & LBIF_SELECTED) {
            SetBkColor (hdc, bk = BLUE);
            SetTextColor (hdc, WHITE);
        }
        else {
            SetBkColor (hdc, bk = WHITE);
            SetTextColor (hdc, BLACK);
        }
	rc.left = 0;
	rc.top = y;
	rc.right = width;
	rc.bottom = y + pData->itemHeight;
	FastFillRect(hdc, &rc, bk);

        if (dwStyle & LBS_CHECKBOX) {
            x = LST_INTER_BMPTEXT;
            if (plbi->dwFlags & LBIF_CHECKED)
                offset = 0;
            else if (plbi->dwFlags & LBIF_PARTCHECKED)
                offset = LST_WIDTH_CHECKMARK << 1;
            else
                offset = LST_WIDTH_CHECKMARK;
#if 0	/* fix: no bitmap */
            FillBoxWithBitmapPart (hdc, 
                x, y + ((pData->itemHeight - LST_HEIGHT_CHECKMARK)>>1),
                LST_WIDTH_CHECKMARK, LST_HEIGHT_CHECKMARK,
                0, 0,
                &sg_bmpCheckMark,
                offset, 0);
#endif
            x += LST_WIDTH_CHECKMARK + LST_INTER_BMPTEXT;
        }
#if 0	/* fix: no icon */
        if (dwStyle & LBS_USEICON && plbi->dwData) {
            DrawIcon (hdc, 
                x, y, pData->itemHeight, pData->itemHeight, 
                (HICON) plbi->dwData);
            x += pData->itemHeight + LST_INTER_BMPTEXT;
        }
#endif

/* jmt: should be SYSTEM_FIXED_FONT because of minigui's GetSysCharXXX() */
#if 0
	SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
#endif
	SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
        TextOut (hdc, x+2, y, plbi->key,-1);

        y += pData->itemHeight;
        plbi = plbi->next;
    }
}

static int lstSelectItem (DWORD dwStyle, PLISTBOXDATA pData, int newSel)
{
    PLISTBOXITEM plbi, newItem;
    int index;
    
    newItem = lstGetItem (pData, newSel);
#if 1	/* jmt: fixed if no item added */
    if (!newItem) return -1;
#endif 
#ifdef _DEBUG
    if (!newItem)
        fprintf (stderr, "ASSERT failed: return value of lstGetItem"
                         " in lstSelectItem.\n");
#endif

    if (dwStyle & LBS_MULTIPLESEL) {
        newItem->dwFlags ^= LBIF_SELECTED;
        return newSel;
    }

    index = 0;
    plbi = pData->head;
    while (plbi) {
        if (plbi->dwFlags & LBIF_SELECTED) {
            if (index != newSel) {
                plbi->dwFlags &= ~LBIF_SELECTED;
                newItem->dwFlags |= LBIF_SELECTED;
                return index;
            }
            break;
        }

        plbi = plbi->next;
        index ++;
    }

    newItem->dwFlags |= LBIF_SELECTED;
    return -1;
}

static void lstDrawFocusRect (HDC hdc, PLISTBOXDATA pData, RECT* rc)
{
    HGDIOBJ oldbrush,oldpen;

    if (pData->itemHilighted < pData->itemTop
            || pData->itemHilighted > (pData->itemTop + pData->itemVisibles))
        return;

    if (pData->dwFlags & LBF_FOCUS) {
        lstGetItemsRect (pData, pData->itemHilighted, pData->itemHilighted, rc);
#if 0
        InflateRect (rc, -1, -1);

        FocusRect (hdc, rc->left - 1, rc->top, rc->right, rc->bottom);
#else
	oldbrush=SelectObject(hdc, GetStockObject(NULL_BRUSH));
	oldpen=SelectObject(hdc, CreatePen(PS_SOLID, 1,
				GetSysColor(COLOR_BTNHIGHLIGHT)));
#if 0
	GdSetMode(MWMODE_XOR);
#endif
        Rectangle (hdc, rc->left, rc->top, rc->right, rc->bottom);
#if 0
	GdSetMode(MWMODE_SET);
#endif
	SelectObject(hdc,oldbrush);
	DeleteObject(SelectObject(hdc,oldpen));
#endif
    }
}

static void lstCalcParams (const RECT* rcClient, PLISTBOXDATA pData)
{
#define RECTHP(prc)  (prc->bottom - prc->top)
    pData->itemVisibles = (RECTHP (rcClient)) / pData->itemHeight;

#if 1	/* test calculation of itemVisibles */
    if( ((RECTHP (rcClient)) % pData->itemHeight) )
    	pData->itemVisibles++;
#endif
}

extern BOOL SetScrollPos (HWND hWnd, int iSBar, int iNewPos);
extern BOOL EnableScrollBar (HWND hWnd, int iSBar, BOOL bEnable);

static void lstSetVScrollInfo (HWND hwnd, PLISTBOXDATA pData, BOOL fRedraw)
{
    SCROLLINFO si;

    if (pData->itemVisibles >= pData->itemCount) 
    {
        SetScrollPos (hwnd, SB_VERT, 0);
        EnableScrollBar (hwnd, SB_VERT, FALSE);
        return;
    }
    
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMax = pData->itemCount - 1;
    si.nMin = 0;
    si.nPage = min (pData->itemVisibles, (pData->itemCount - pData->itemTop));
    si.nPos = pData->itemTop;

    SetScrollInfo (hwnd, SB_VERT, &si, fRedraw);
    EnableScrollBar (hwnd, SB_VERT, TRUE);
}

LRESULT CALLBACK
ListboxCtrlProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC             hdc;
    HWND	    pCtrl;
    PLISTBOXDATA    pData;
    DWORD           dwStyle;
    
    pCtrl   = hwnd;
    dwStyle = pCtrl->style;

    switch (message)
    {
        case WM_CREATE:
            pData = (LISTBOXDATA*) malloc (sizeof(LISTBOXDATA));
            if (pData == NULL) 
                return -1;

            pCtrl->userdata = (DWORD)pData;
            if (!lstInitListBoxData (hwnd, pData, DEF_LB_BUFFER_LEN)) {
                free (pData);
                return -1;
            }
        break;

	case WM_SIZE:
	{
	    RECT rc;

            pData = (PLISTBOXDATA)pCtrl->userdata;
	    GetClientRect(hwnd, &rc);
            lstCalcParams (&rc, pData);
	}
        break;

        case WM_DESTROY:
            pData = (PLISTBOXDATA)pCtrl->userdata;
            lstListBoxCleanUp (pData);
            free (pData);
        break;

        case LB_RESETCONTENT:
            pData = (PLISTBOXDATA)pCtrl->userdata;
            lstResetListBoxContent (pData);
            InvalidateRect (hwnd, NULL, TRUE);
        break;
        
        case LB_ADDSTRING:
        case LB_INSERTSTRING:
        {
            char* string = NULL;
            PLISTBOXITEMINFO plbii = NULL;
            PLISTBOXITEM newItem;
            int pos;
           
            if (dwStyle & LBS_CHECKBOX || dwStyle & LBS_USEICON) {
                plbii = (PLISTBOXITEMINFO)lParam;
                if (!plbii)
                    return LB_ERR;

                string = plbii->string;
            }
            else {
                string = (char*)lParam;
                if (string == NULL || string [0] == '\0')
                    return LB_ERR;
            }

            pData = (PLISTBOXDATA)pCtrl->userdata;
            newItem = lstAllocItem (pData);
            if (!newItem) {
                NotifyParent (hwnd, pCtrl->id, LBN_ERRSPACE);
                return LB_ERRSPACE;
            }

            newItem->key = FixStrAlloc (strlen (string));
            strcpy (newItem->key, string);
            newItem->dwFlags = LBIF_NORMAL;
            if (plbii) {
                switch (plbii->cmFlag) {
                    case CMFLAG_CHECKED:
                        newItem->dwFlags |= LBIF_CHECKED;
                    break;
                    case CMFLAG_PARTCHECKED:
                        newItem->dwFlags |= LBIF_PARTCHECKED;
                    break;
                }
                
                if (dwStyle & LBS_USEICON)
                    newItem->dwData = (DWORD)plbii->hIcon;
                else
                    newItem->dwData = 0L;
            }
            newItem->dwAddData = 0L;

            if (message == LB_ADDSTRING)
                pos = lstAddNewItem (dwStyle, pData, newItem, -1);
            else
                pos = lstAddNewItem (dwStyle, pData, newItem, (int)wParam);

            lstInvalidateUnderItem (hwnd, pData, pos);

            lstSetVScrollInfo (hwnd, pData, TRUE);

            return pos;
        }
        break;
        
        case LB_DELETESTRING:
        {
            PLISTBOXITEM removed;
            int delete;

            delete = (int)wParam;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            removed = lstRemoveItem (pData, &delete);
            if (removed) {
                FreeFixStr (removed->key);
                lstFreeItem (pData, removed);
                
                pData->itemCount --;

                if (pData->itemTop != 0 
                        && pData->itemCount <= pData->itemVisibles) {
                    pData->itemTop = 0;
                    InvalidateRect (hwnd, NULL, TRUE);
                }
                else {
                    lstInvalidateUnderItem (hwnd, pData, delete);
                    if (delete <= pData->itemTop) {
                        pData->itemTop --;
                        if (pData->itemTop < 0)
                            pData->itemTop = 0;
                    }
                }

                if (pData->itemHilighted >= pData->itemCount) {
                    pData->itemHilighted = pData->itemCount - 1;
                    if (pData->itemHilighted < 0)
                        pData->itemHilighted = 0;
                }

                if (pData->itemHilighted < pData->itemTop)
                    pData->itemHilighted = pData->itemTop;
                if (pData->itemHilighted > ITEM_BOTTOM (pData))
                    pData->itemHilighted = ITEM_BOTTOM (pData);
             
                lstSetVScrollInfo (hwnd, pData, TRUE);
            }
        }
        break;

        case LB_FINDSTRING:
            if( *(char*)lParam == '\0' )
                return LB_ERR;
                
            pData = (PLISTBOXDATA)pCtrl->userdata;
            return lstFindItem(pData, (int)wParam, (char*)lParam, FALSE);

        case LB_FINDSTRINGEXACT:
            if( *(char*)lParam == '\0' )
                return LB_ERR;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            return lstFindItem(pData, (int)wParam, (char*)lParam, TRUE);
    
        case LB_SETTOPINDEX:
        {
            int newTop = (int) wParam;
            
            pData = (PLISTBOXDATA)pCtrl->userdata;

            if (newTop <0)
                newTop = 0;
            else if (newTop > pData->itemCount - pData->itemVisibles)
                newTop = pData->itemCount - pData->itemVisibles;
                
            if (pData->itemTop != newTop) {
                pData->itemTop = newTop;

                if (pData->itemHilighted < pData->itemTop)
                    pData->itemHilighted = pData->itemTop;
                if (pData->itemHilighted > ITEM_BOTTOM (pData))
                    pData->itemHilighted = ITEM_BOTTOM (pData);

                lstSetVScrollInfo (hwnd, pData, TRUE);

                InvalidateRect (hwnd, NULL, TRUE);
            }
        }
        break;
    
        case LB_SETCURSEL:
        case LB_SETCARETINDEX:
        {
            int new = (int)wParam;
            int old, newTop;
            
            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (new < 0 || new > pData->itemCount - 1)
                return LB_ERR;

            old = pData->itemHilighted;
            if (new >= 0 && new != old) {
                if (pData->itemCount - new >= pData->itemVisibles)
                    newTop = new;
                else
                    newTop = max (pData->itemCount - pData->itemVisibles, 0);

                pData->itemTop = newTop;
                pData->itemHilighted = new;
                lstSetVScrollInfo (hwnd, pData, TRUE);
            }

            if (!(dwStyle & LBS_MULTIPLESEL))
                lstSelectItem (dwStyle, pData, new);
            InvalidateRect (hwnd, NULL, TRUE);

            return old;
        }
        break;
    
        case LB_GETCOUNT:
            pData = (PLISTBOXDATA)pCtrl->userdata;
            return pData->itemCount;
        break;
    
        case LB_GETCURSEL:
        {
            PLISTBOXITEM plbi;
            int index = 0;
            
            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (dwStyle & LBS_MULTIPLESEL)
                return pData->itemHilighted;

            plbi = pData->head;
            while (plbi) {
                if (plbi->dwFlags & LBIF_SELECTED)
                    return index;

                index ++;
                plbi = plbi->next;
           }
           
           return LB_ERR;
        }
        break;
    
        case LB_GETSELCOUNT:
        {
            int nSel;
            PLISTBOXITEM plbi;
            
            pData = (PLISTBOXDATA)pCtrl->userdata;

            nSel = 0;
            plbi = pData->head;
            while (plbi) {
                if (plbi->dwFlags & LBIF_SELECTED)
                    nSel ++;
                plbi = plbi->next;
            }
            
            return nSel;
        }
        break;
    
        case LB_GETTOPINDEX:
            pData = (PLISTBOXDATA)pCtrl->userdata;
            return pData->itemTop;
        break;
    
        case LB_GETCARETINDEX:
            pData = (PLISTBOXDATA)pCtrl->userdata;
            return pData->itemHilighted;
        break;

        case LB_GETTEXTLEN:
        {
            PLISTBOXITEM plbi;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            plbi = lstGetItem (pData, (int)wParam);
            if (plbi)
                return strlen (plbi->key);
            else
                return LB_ERR;
        }
        break;
        
        case LB_GETTEXT:
        {
            PLISTBOXITEM plbi;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            plbi = lstGetItem (pData, (int)wParam);
            if (plbi)
                strcpy ((char*)lParam, plbi->key);
            else
                return LB_ERR;
        }
        break;

        case LB_SETTEXT:
        {
            PLISTBOXITEM plbi;
            char* newStr;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            plbi = lstGetItem (pData, (int)wParam);
            if (plbi) {
                newStr = FixStrAlloc (strlen ((char*)lParam));
                if (newStr) {
                    FreeFixStr (plbi->key);
                    plbi->key = newStr;
                    strcpy (plbi->key, (char*)lParam);
                    lstInvalidateItem (hwnd, pData, (int)wParam, TRUE);
                }
                else
                    return LB_ERR;
            }
            else
                return LB_ERR;
        }
        break;
    
        case LB_GETITEMDATA:
        {
            PLISTBOXITEM plbi;
            PLISTBOXITEMINFO plbii;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (!(plbi = lstGetItem (pData, (int)wParam)))
                return LB_ERR;

            if (!(dwStyle & LBS_CHECKBOX || dwStyle & LBS_USEICON)) {
                return plbi->dwData;
            }
            
            plbii = (PLISTBOXITEMINFO)lParam;
            if (!plbii)
                return LB_ERR;

            if (plbi->dwFlags & LBIF_CHECKED)
                plbii->cmFlag = CMFLAG_CHECKED;
            else if (plbi->dwFlags & LBIF_PARTCHECKED)
                plbii->cmFlag = CMFLAG_PARTCHECKED;
            else
                plbii->cmFlag = CMFLAG_BLANK;
            plbii->hIcon = (HICON)plbi->dwData;

            return LB_OKAY;
        }
        break;
    
        case LB_SETITEMDATA:
        {
            PLISTBOXITEM plbi;
            PLISTBOXITEMINFO plbii;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (!(plbi = lstGetItem (pData, (int)wParam)))
                return LB_ERR;

            if (!(dwStyle & LBS_CHECKBOX || dwStyle & LBS_USEICON)) {
                plbi->dwData = (DWORD)lParam;
                return LB_OKAY;
            }

            plbii = (PLISTBOXITEMINFO)lParam;
            if (!plbii)
                return LB_ERR;

            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
            switch (plbii->cmFlag) {
                case CMFLAG_CHECKED:
                    plbi->dwFlags |= LBIF_CHECKED;
                break;
                case CMFLAG_PARTCHECKED:
                    plbi->dwFlags |= LBIF_PARTCHECKED;
                break;
            }
                
            if (dwStyle & LBS_USEICON)
                plbi->dwData = (DWORD)plbii->hIcon;
            else
                plbi->dwData = 0;
            
            lstInvalidateItem (hwnd, pData, (int)wParam, TRUE);

            return LB_OKAY;
        }
        break;
        
        case LB_GETITEMADDDATA:
        {
            PLISTBOXITEM plbi;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (!(plbi = lstGetItem (pData, (int)wParam)))
                return LB_ERR;

            return plbi->dwAddData;
        }
        break;
    
        case LB_SETITEMADDDATA:
        {
            PLISTBOXITEM plbi;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (!(plbi = lstGetItem (pData, (int)wParam)))
                return LB_ERR;

            plbi->dwAddData = (DWORD)lParam;

            return LB_OKAY;
        }
        break;
        
        case LB_GETCHECKMARK:
        {
            PLISTBOXITEM plbi;

            if (!(dwStyle & LBS_CHECKBOX))
                return LB_ERR;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (!(plbi = lstGetItem (pData, (int)wParam)))
                return LB_ERR;

            if (plbi->dwFlags & LBIF_CHECKED)
                return CMFLAG_CHECKED;

            if (plbi->dwFlags & LBIF_PARTCHECKED)
                return CMFLAG_PARTCHECKED;

            return CMFLAG_BLANK;
        }
        break;

        case LB_SETCHECKMARK:
        {
            PLISTBOXITEM plbi;

            if (!(dwStyle & LBS_CHECKBOX))
                return LB_ERR;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (!(plbi = lstGetItem (pData, (int)wParam)))
                return LB_ERR;

            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
            switch (lParam) {
                case CMFLAG_CHECKED:
                    plbi->dwFlags |= LBIF_CHECKED;
                break;
                case CMFLAG_PARTCHECKED:
                    plbi->dwFlags |= LBIF_PARTCHECKED;
                break;
            }
                
            lstInvalidateItem (hwnd, pData, (int)wParam, TRUE);

            return LB_OKAY;
        }
        break;

        case LB_GETSELITEMS:
        {
            int  nItem;
            int  nSel = 0;
            int  index = 0;
            int* pInt;
            PLISTBOXITEM plbi;

            nItem = (int)wParam;
            pInt  = (int*)lParam;
            
            pData = (PLISTBOXDATA)pCtrl->userdata;
            plbi = pData->head;
            while (plbi) {

                if (plbi->dwFlags & LBIF_SELECTED) {
                    if (pInt) {
                        if (nSel < nItem)
                            *(pInt + nSel) = index;
                        else 
                            return nItem;
                    }
                    nSel ++;
                }
                
                plbi = plbi->next;
                index ++;
            }
            
            return nSel;
        }
        break;
    
        case LB_GETSEL:
        {
            PLISTBOXITEM plbi;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            plbi = lstGetItem (pData, (int)wParam);
            if (plbi)
                return plbi->dwFlags & LBIF_SELECTED;
            else
                return LB_ERR;
        }
        break;

        case LB_SETSEL:
        {
            PLISTBOXITEM plbi;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            plbi = lstGetItem (pData, (int)lParam);
            if (plbi) {
		pData->dwFlags &= ~LBF_NOTHINGSELECTED;
                if (wParam == -1)
                    plbi->dwFlags ^= LBIF_SELECTED;
                else if (wParam == 0)
                    plbi->dwFlags &= ~LBIF_SELECTED;
                else
                    plbi->dwFlags |= LBIF_SELECTED;

                lstInvalidateItem (hwnd, pData, (int)lParam, FALSE);
            }
            else
                return LB_ERR;
        }
        break;
    
        case LB_GETITEMHEIGHT:
            pData = (PLISTBOXDATA)pCtrl->userdata;
            return pData->itemHeight;
        break;
    
        case LB_SETITEMHEIGHT:
            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (pData->itemHeight != LOWORD (lParam)) {
                RECT rcClient;
                
                pData->itemHeight = LOWORD (lParam);
                GetClientRect (hwnd, &rcClient);
                lstCalcParams (&rcClient, pData);
                
                lstSetVScrollInfo (hwnd, pData, TRUE);
                InvalidateRect (hwnd, NULL, TRUE);
            }
        break;

        case WM_SETFOCUS:
        {
            pData = (PLISTBOXDATA)pCtrl->userdata;

            if (pData->dwFlags & LBF_FOCUS)
                break;
                
            pData->dwFlags |= LBF_FOCUS;
	    InvalidateRect(hwnd, NULL, TRUE);

            NotifyParent (hwnd, pCtrl->id, LBN_SETFOCUS);
        }
        break;
        
        case WM_KILLFOCUS:
        {
            pData = (PLISTBOXDATA)pCtrl->userdata;

            pData->dwFlags &= ~LBF_FOCUS;
	    InvalidateRect(hwnd, NULL, TRUE);

            NotifyParent (hwnd, pCtrl->id, LBN_KILLFOCUS);
        }
        break;

        case WM_GETDLGCODE:
            return DLGC_WANTARROWS | DLGC_WANTCHARS;

        case WM_GETTEXTLENGTH:
        case WM_GETTEXT:
        case WM_SETTEXT:
            return -1;
#if 0            
	case WM_SETFONT:
        break;
        
	case WM_GETFONT:
        break;
#endif        
	case WM_NCCALCSIZE:
	{
		LPNCCALCSIZE_PARAMS lpnc;

		/* calculate client rect from passed window rect in rgrc[0]*/
		lpnc = (LPNCCALCSIZE_PARAMS)lParam;
		if(GetWindowLong(hwnd, GWL_STYLE) & WS_BORDER)
			InflateRect(&lpnc->rgrc[0], -2, -2);
	}
		break;

        case WM_NCPAINT:
	{
	    RECT rc;

            hdc = wParam? (HDC)wParam: GetWindowDC (hwnd);
	    GetWindowRect(hwnd, &rc);

            if (dwStyle & WS_BORDER)
		Draw3dInset(hdc, rc.left, rc.top,
			rc.right-rc.left, rc.bottom-rc.top);

            if (!wParam)
                ReleaseDC (hwnd, hdc);
	}
            break;

        case WM_PAINT:
        {
            RECT rc;
	    PAINTSTRUCT ps;
            
            hdc = BeginPaint (hwnd,&ps);
            pData = (PLISTBOXDATA)pCtrl->userdata;

	    /*
	     * If this is the first paint and there's nothing
	     * selected, then auto select the topmost displayed item.
	     */
	    if (pData->dwFlags & LBF_NOTHINGSELECTED) {
		    lstSelectItem (hwnd->style, pData, pData->itemTop);
		    pData->dwFlags &= ~LBF_NOTHINGSELECTED;
	    }
            GetClientRect (hwnd, &rc);
            lstOnDrawSListBoxItems (hdc, dwStyle, pData, rc.right-rc.left);
            lstDrawFocusRect (hdc, pData, &rc);

            EndPaint (hwnd, &ps);
        }
        break;
    
        case WM_LBUTTONDBLCLK:
            if (dwStyle & LBS_NOTIFY)
                NotifyParent (hwnd, pCtrl->id, LBN_DBLCLK);
        break;
    
        case WM_LBUTTONDOWN:
        {
            int oldSel, mouseX, mouseY, hit;
            RECT rcInv;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            if (pData->itemCount == 0)
                break;

            mouseX = LOWORD (lParam);
            mouseY = HIWORD (lParam);
            hit = mouseY / pData->itemHeight;
            hit += pData->itemTop;
            
            if (hit >= pData->itemCount)
                break;

            GetClientRect (hwnd, &rcInv);
            oldSel = lstSelectItem (dwStyle, pData, hit);
            if ((dwStyle & LBS_NOTIFY) && (oldSel != hit))
                NotifyParent (hwnd, pCtrl->id, LBN_SELCHANGE);
            if (oldSel >= 0) {
                if (oldSel >= pData->itemTop 
                        && (oldSel <= pData->itemTop + pData->itemVisibles)) {
                    lstGetItemsRect (pData, oldSel, oldSel, &rcInv);
                    InvalidateRect (hwnd, &rcInv, TRUE);
                }
            }

            lstGetItemsRect (pData, hit, hit, &rcInv);
            InvalidateRect (hwnd, &rcInv, TRUE);

            if (pData->itemHilighted != hit) 
	    {
                hdc = GetDC(hwnd); /* hdc = GetClientDC (hwnd); */

                lstDrawFocusRect (hdc, pData, &rcInv);
                ReleaseDC (hwnd,hdc);
            }
            pData->itemHilighted = hit;

            if (dwStyle & LBS_CHECKBOX) {
                if (mouseX > 0 && mouseX < LST_WIDTH_CHECKMARK) {
                    NotifyParent (hwnd, pCtrl->id, LBN_CLICKCHECKMARK);
                    
                    if (dwStyle & LBS_AUTOCHECK) {
                        PLISTBOXITEM plbi;

                        plbi = lstGetItem (pData, hit);

                        switch (plbi->dwFlags & LBIF_CHECKMARKMASK) {
                        case LBIF_CHECKED:
                            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
                            break;
                        default:
                            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
                            plbi->dwFlags |= LBIF_CHECKED;
                            break;
                        }
                
                        lstInvalidateItem (hwnd, pData, hit, TRUE);
                    }
                }
            }
            
            lstSetVScrollInfo (hwnd, pData, TRUE);
        }
        break;
    
        case WM_LBUTTONUP:
        break;
    
        case WM_MOUSEMOVE:
        break;

        case WM_KEYDOWN:
        {
            int oldSel, newSel, newTop;
            RECT rcInv;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            newTop = pData->itemTop;
            newSel = pData->itemHilighted;
            switch (LOWORD (wParam))
            {
                case VK_HOME:	/* SCANCODE_HOME: */
                    newSel = 0;
                    newTop = 0;
                break;
                
                case VK_END:	/* SCANCODE_END: */
                    newSel = pData->itemCount - 1;
                    if (pData->itemCount > pData->itemVisibles)
                        newTop = pData->itemCount - pData->itemVisibles;
                    else
                        newTop = 0;
                break;
                
                case VK_DOWN:	/* SCANCODE_CURSORBLOCKDOWN: */
                    newSel ++;
                    if (newSel >= pData->itemCount)
                        return 0;
                    if (newSel > ITEM_BOTTOM (pData))
                        newTop ++;
                break;
                
                case VK_UP:	/* SCANCODE_CURSORBLOCKUP: */
                    newSel --;
                    if (newSel < 0)
                        return 0;
                    if (newSel < pData->itemTop)
                        newTop --;
                break;
                
                case VK_NEXT:	/* SCANCODE_PAGEDOWN: */
                    newSel += pData->itemVisibles;
                    if (newSel >= pData->itemCount)
                        newSel = pData->itemCount - 1;
                        
                    if (pData->itemCount - newSel >= pData->itemVisibles)
                        newTop = newSel;
                    else
                        newTop = max (pData->itemCount-pData->itemVisibles, 0);
                break;

                case VK_PRIOR:	/* SCANCODE_PAGEUP: */
                    newSel -= pData->itemVisibles;
                    if (newSel < 0)
                        newSel = 0;
                        
                    newTop -= pData->itemVisibles;
                    if (newTop < 0)
                        newTop = 0;
                break;
                
                default:
                return 0;
            }

            GetClientRect (hwnd, &rcInv);
            if (pData->itemHilighted != newSel) {
                if (pData->itemTop != newTop) {
                    pData->itemTop = newTop;
                    pData->itemHilighted = newSel;
                    if (!(dwStyle & LBS_MULTIPLESEL)) {
                        oldSel = lstSelectItem (dwStyle, pData, newSel);
                        if ((dwStyle & LBS_NOTIFY) && (oldSel != newSel))
                            NotifyParent (hwnd, pCtrl->id, LBN_SELCHANGE);
                    }
                    InvalidateRect (hwnd, NULL, TRUE);
                }
                else {
                    if (!(dwStyle & LBS_MULTIPLESEL)) {
                        oldSel = lstSelectItem (dwStyle, pData, newSel);
                        if ((dwStyle & LBS_NOTIFY) && (oldSel != newSel))
                            NotifyParent (hwnd, pCtrl->id, LBN_SELCHANGE);
                        if (oldSel >= 0) {
                            if (oldSel >= pData->itemTop 
                                    && oldSel <= (ITEM_BOTTOM (pData) + 1)) {
                                lstGetItemsRect (pData, oldSel, oldSel, &rcInv);
                                InvalidateRect (hwnd, &rcInv, TRUE);
                            }
                        }
                        
                        if (newSel < newTop) {
                            pData->itemHilighted = newSel;
                            break;
                        }
                            
                        lstGetItemsRect (pData, pData->itemHilighted,
                                                pData->itemHilighted, &rcInv);

                	hdc = GetDC(hwnd); /* hdc = GetClientDC (hwnd); */

                        lstDrawFocusRect (hdc, pData, &rcInv);
                        ReleaseDC (hwnd,hdc);
                        
                        pData->itemHilighted = newSel;
                        lstGetItemsRect (pData, newSel, newSel, &rcInv);
                        InvalidateRect (hwnd, &rcInv, TRUE);
                    }
                    else 
		    {
                	hdc = GetDC(hwnd); /* hdc = GetClientDC (hwnd); */

                        lstDrawFocusRect (hdc, pData, &rcInv);
                        pData->itemHilighted = newSel;
                        GetClientRect (hwnd, &rcInv);
                        lstDrawFocusRect (hdc, pData, &rcInv);
                        ReleaseDC (hwnd,hdc);
                    }
                }
                lstSetVScrollInfo (hwnd, pData, TRUE);
            }
        }
        break;

        case WM_CHAR:
        {
            char head [2];
            int index;
            int newTop;
            
            head [0] = (char) (wParam);
            head [1] = '\0';

            pData = (PLISTBOXDATA)pCtrl->userdata;

            if (head[0] == ' ') {
                if (dwStyle & LBS_MULTIPLESEL) {
                    RECT rcInv;
                    
                    GetClientRect (hwnd, &rcInv);
                    lstSelectItem (dwStyle, pData, pData->itemHilighted);
                    lstGetItemsRect (pData, 
                            pData->itemHilighted, 
                            pData->itemHilighted,
                            &rcInv);
                    InvalidateRect (hwnd, &rcInv, TRUE);
                }
                else if (dwStyle & LBS_CHECKBOX) {
                    NotifyParent (hwnd, pCtrl->id, LBN_CLICKCHECKMARK);
                    
                    if (dwStyle & LBS_AUTOCHECK) {
                        PLISTBOXITEM plbi;

                        plbi = lstGetItem (pData, pData->itemHilighted);

                        switch (plbi->dwFlags & LBIF_CHECKMARKMASK) {
                        case LBIF_CHECKED:
                            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
                            break;
                        default:
                            plbi->dwFlags &= ~LBIF_CHECKMARKMASK;
                            plbi->dwFlags |= LBIF_CHECKED;
                            break;
                        }
                
                        lstInvalidateItem (hwnd, pData, 
                                pData->itemHilighted, TRUE);
                    }
                }
                break;
            }

            index = lstFindItem (pData, pData->itemHilighted + 1, head, FALSE);
            if (index < 0) {
                index = lstFindItem (pData, 0, head, FALSE);
            }

            if (index >= 0) {
                if (pData->itemCount - index >= pData->itemVisibles)
                    newTop = index;
                else
                    newTop = max (pData->itemCount - pData->itemVisibles, 0);

                pData->itemTop = newTop;
                pData->itemHilighted = index;
                if (!(dwStyle & LBS_MULTIPLESEL))
                    lstSelectItem (dwStyle, pData, index);
                InvalidateRect (hwnd, NULL, TRUE);

                lstSetVScrollInfo (hwnd, pData, TRUE);
            }
        }
        break;
 
        case WM_VSCROLL:
        {
            int newTop;
            int scrollHeight = 0;

            pData = (PLISTBOXDATA)pCtrl->userdata;
            newTop = pData->itemTop;
            switch(wParam)
            {
                case SB_LINEDOWN:

#if 0	/* test itemVisibles */
		    printf("itemVisibles:%d\n",pData->itemVisibles);
		    printf("SB_LINEDOWN:(%d:%d)\n",
                    	ITEM_BOTTOM (pData),(pData->itemCount - 1 )); 
#endif
                    if (ITEM_BOTTOM (pData) < (pData->itemCount - 1 )) 
		    {
                        newTop ++;
                        scrollHeight = -pData->itemHeight;	/* for ScrollWindow() */
                    }
                break;
                
                case SB_LINEUP:
                    if (pData->itemTop > 0) 
		    {
                        newTop --;
                        scrollHeight = pData->itemHeight;
                    }
                break;
                
                case SB_PAGEDOWN:
                    if ((pData->itemTop + (pData->itemVisibles << 1)) <=
                            pData->itemCount)
                        newTop += pData->itemVisibles;
                    else
                        newTop = pData->itemCount - pData->itemVisibles;

                    if (newTop < 0)
                        return 0;

                    scrollHeight = -(newTop - pData->itemTop)
                                    *pData->itemHeight;
                break;

                case SB_PAGEUP:
                    if (pData->itemTop >= pData->itemVisibles)
                        newTop -= pData->itemVisibles;
                    else
                        newTop = 0;

                    scrollHeight = (pData->itemTop - newTop)*pData->itemHeight;
                break;

                case SB_THUMBTRACK:
                    newTop = (int)lParam;
                    scrollHeight = (pData->itemTop - newTop)*pData->itemHeight;
                break;
            }
            
            if (scrollHeight) 
	    {
                pData->itemTop = newTop;
#if 0	/* !!: fix: no scroll */
                ScrollWindow (hwnd, 0, scrollHeight, NULL, NULL);
#endif
                SendMessage (hwnd, WM_PAINT, 0, 0);

                lstSetVScrollInfo (hwnd, pData, TRUE);

                return 0;
            }
        }
        break;

	case WM_HSCROLL:
            pData = (PLISTBOXDATA)pCtrl->userdata;
            switch (wParam)
            {
                case SB_LINERIGHT:
                break;
                
                case SB_LINELEFT:
                break;
                
                case SB_PAGELEFT:
                break;
                
                case SB_PAGERIGHT:
                break;
            }
        break;

        default:
    		return DefWindowProc (hwnd, message, wParam, lParam);
    }
    return 0;
}
