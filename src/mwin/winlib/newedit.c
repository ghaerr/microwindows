/*
 *
 * NEW "EDIT control" for Microwindows win32 api.
 *
 * Copyright (C) 2003, Gabriele Brugnoni
 * <gabrielebrugnoni@dveprojects.com>
 * DVE Prog. El. - Varese, Italy
 *
 * Based on microwindows edit.c
 *  Copyright (C) 1999, 2000, Wei Yongming.
 *  Portions Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 */

/* Note:
**
** Create date: 2003/09/02
**
** Modify records:
**
**  Who             When        Where       For What                Status
**-----------------------------------------------------------------------------
**
** TODO:
**    * BIG5 font and cinese support
**    * Multiline.
**    * UNICODE ??.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MWINCLUDECOLORS
#include "windows.h"	/* windef.h, winuser.h */
#include "wintools.h"
#include "device.h" 	/* GdGetTextSize */


// NOTE: 
#if HAVE_HZK_SUPPORT || HAVE_BIG5_SUPPORT
#define USE_BIG5
#define DEFAULT_FONT	SYSTEM_FIXED_FONT
#else
#define DEFAULT_FONT	DEFAULT_GUI_FONT
#endif


#define WIDTH_EDIT_BORDER       2
#define MARGIN_EDIT_LEFT        1
#define MARGIN_EDIT_TOP         1
#define MARGIN_EDIT_RIGHT       2
#define MARGIN_EDIT_BOTTOM      1

#define LEN_SLEDIT_BUFFER       256
#define LEN_SLEDIT_ADDBUFFER    256
#define LEN_SLEDIT_REMOVEBUFFER 1024	// if great than this, realloc
#define LEN_SLEDIT_UNDOBUFFER   1024

#define EST_FOCUSED     	0x00000001L
#define EST_MODIFY      	0x00000002L
#define EST_READONLY    	0x00000004L
#define EST_REPLACE     	0x00000008L
#define EST_CTRL		0x00000010L
#define EST_SHIFT		0x00000020L
#define EST_SELSCROLLLEFT	0x00000040L
#define EST_SELSCROLLRIGHT	0x00000080L

#define EDIT_OP_NONE    	0x00
#define EDIT_OP_DELETE  	0x01
#define EDIT_OP_INSERT  	0x02
#define EDIT_OP_REPLACE 	0x03

//  Timers
#define IDTM_SELSCROLLLEFT	0x101
#define IDTM_SELSCROLLRIGHT	0x102
#define TM_SELSCROLL		60	// timeout for scrolling

char mwDefEditCaretSize		= 1;
char mwDefEditCaretSizeIns	= 3;



typedef char		EDITCHAR;
#define SZEDITCHAR	sizeof(char)


typedef struct tagSLEDITDATA {
    HFONT   hFont;          /* hFont used */
    int     bufferLen;      /* length of buffer */

    int     dataEnd;        /* data end position */
    int     editPos;        /* current edit position */
    int     caretX;         /* caret X position in box */
    int	    scrollX;	    /* X scrolling offset */
    
    int     selStart;       /* selection start position */
    int     selEnd;         /* selection end position */
    int     selCenter;      /* selection cursor center */
    
    int     passwdChar;     /* password character */

    int	    charHeight;	    /* height of character */
    
    int     leftMargin;     /* left margin */
    int     topMargin;      /* top margin */
    int     rightMargin;    /* right margin */
    int     bottomMargin;   /* bottom margin */
    
    int     hardLimit;      /* hard limit */

    int     lastOp;         /* last operation */
    int     lastPos;        /* last operation position */
    int     affectedLen;    /* affected len of last operation */
    int     undoBufferLen;  /* undo buffer len */
    EDITCHAR undoBuffer [LEN_SLEDIT_UNDOBUFFER];	/* Undo buffer; */
    EDITCHAR *buffer;	    /* buffer */
} SLEDITDATA, *PSLEDITDATA;


//  For future implementation of unicode...
#define edit_memcpy		memcpy
#define memcpy_fromedit		memcpy
#define neTextOut		TextOut


#define neIsWord(c)		( isalpha(c) || isdigit(c) )



static int neGetTextHeight ( HWND hWnd, HDC hdc );
static int neGetTextWith ( HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData,
			   const EDITCHAR *txt, int len );

static int neCharPressed ( HWND hWnd, WPARAM wParam, LPARAM lParam );


//  Clipboard for cut and paste.
static EDITCHAR *neClipboard = NULL;
static int neClipboardSize = 0;


/*
 *  Create new edit control
 */
static int neCreate ( HWND hwnd )
{
    PSLEDITDATA pSLEditData;
    HWND	pCtrl = hwnd;
    int len;

    if( !(pSLEditData = malloc (sizeof (SLEDITDATA))) ) {
        fprintf (stderr, "EDIT: malloc error!\n");
        return -1;
    }

    len = 1 + strlen ( pCtrl->szTitle );
    if( len < LEN_SLEDIT_BUFFER )
	len = LEN_SLEDIT_BUFFER;

    pCtrl->userdata2 = (DWORD) pSLEditData;
    pCtrl->userdata  = 0;

#ifdef USE_BIG5
    pSLEditData->hFont      	= CreateFont ( 12,
				    0,0,0,0,0,0,0,0,0,0,0,
				    FF_DONTCARE|DEFAULT_PITCH,
				    "HZXFONT" );
#else
    pSLEditData->hFont      	= GetStockObject(DEFAULT_FONT);
#endif

    pSLEditData->bufferLen      = len;
    pSLEditData->editPos        = 0;
    pSLEditData->caretX			= 0;
    pSLEditData->scrollX        = 0;

    pSLEditData->selStart       = 0;
    pSLEditData->selEnd         = 0;
    pSLEditData->passwdChar     = '*';
    pSLEditData->leftMargin     = MARGIN_EDIT_LEFT;
    pSLEditData->topMargin      = MARGIN_EDIT_TOP;
    pSLEditData->rightMargin    = MARGIN_EDIT_RIGHT;
    pSLEditData->bottomMargin   = MARGIN_EDIT_BOTTOM;

    pSLEditData->hardLimit      = -1;

    pSLEditData->buffer 		= (EDITCHAR*) malloc ( SZEDITCHAR * pSLEditData->bufferLen );

    /* undo information */
    pSLEditData->lastOp         = EDIT_OP_NONE;
    pSLEditData->lastPos        = 0;
    pSLEditData->affectedLen    = 0;
    pSLEditData->undoBufferLen  = LEN_SLEDIT_UNDOBUFFER;
    pSLEditData->undoBuffer [0] = '\0';

    pSLEditData->dataEnd        = strlen (pCtrl->szTitle);
    edit_memcpy ( pSLEditData->buffer, pCtrl->szTitle, len );

    pSLEditData->charHeight	= neGetTextHeight ( hwnd, NULL );
    return 0;
}


/*
 *  Destroy an edit control
 */
static void neDestroy ( HWND hwnd )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hwnd->userdata2);

    free( pSLEditData->buffer );
    free ( pSLEditData );
    hwnd->userdata2 = 0;
    DestroyCaret ();
}


/*
 *  Reallocate the buffer with the new size
 */
static BOOL neReallocateBuffer ( PSLEDITDATA pSLEditData, int len )
{
    EDITCHAR *newbuff;

    if( len < pSLEditData->dataEnd )
		len = pSLEditData->dataEnd;

    newbuff = (EDITCHAR *)malloc ( len * SZEDITCHAR );
    if( newbuff == NULL )
		{
		fprintf ( stderr, "Unable to allocate buffer for EDIT control.\n" );
		return FALSE;
		}

    memcpy ( newbuff, pSLEditData->buffer, len * SZEDITCHAR );
    free ( pSLEditData->buffer );
    pSLEditData->bufferLen = len;
    pSLEditData->buffer = newbuff;
	return TRUE;
}


/*
 *  Check if it should reduce allocated buffer
 */
static void neCheckBufferSize ( HWND hWnd, PSLEDITDATA pSLEditData )
{
    if( (pSLEditData->bufferLen - pSLEditData->dataEnd) > LEN_SLEDIT_REMOVEBUFFER )
		neReallocateBuffer ( pSLEditData, pSLEditData->dataEnd + LEN_SLEDIT_ADDBUFFER );
}

/*
 *  Increment dataEnd and check if buffer should reallocated
 */
static BOOL neIncDataEnd ( PSLEDITDATA pSLEditData, int delta )
{
    if( (pSLEditData->hardLimit >= 0) &&
		(pSLEditData->dataEnd+delta >= pSLEditData->hardLimit) )
    	return FALSE;

    pSLEditData->dataEnd += delta;
    if( pSLEditData->dataEnd > pSLEditData->bufferLen )
    	{
		if( !neReallocateBuffer(pSLEditData,
			pSLEditData->bufferLen + LEN_SLEDIT_ADDBUFFER) )
	    	{
			pSLEditData->dataEnd -= delta;
			return FALSE;
			}
    	}

    return TRUE;
}

/*
 *  Copy selection to clipboard
 */
static BOOL neCopyToCliboard ( HWND hWnd, PSLEDITDATA pSLEditData )
{
    if( pSLEditData->selStart < pSLEditData->selEnd )
		{
		int count = pSLEditData->selEnd - pSLEditData->selStart;
		if( count + pSLEditData->selStart > pSLEditData->dataEnd )
		    count = pSLEditData->dataEnd - pSLEditData->selStart;
		if( count > 0 )
			{
			if( neClipboard != NULL ) free ( neClipboard );
			neClipboard = (EDITCHAR*) malloc ( count * SZEDITCHAR );
			if( neClipboard == NULL )
			return FALSE;
			memcpy ( neClipboard,
				pSLEditData->buffer + pSLEditData->selStart,
				count * SZEDITCHAR );
			neClipboardSize = count;
			return TRUE;
			}
		}

    return FALSE;
}

/*
 *  Parse WM_SETTEXT command
 */
static BOOL neSetText ( HWND hWnd, const char *text )
{
    DWORD dwStyle = hWnd->style;
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
    int len;

    len = strlen ( text );

    if( pSLEditData->hardLimit >= 0 )
		len = min ( len, pSLEditData->hardLimit );

    if( len > pSLEditData->bufferLen )
    	{
		if( !neReallocateBuffer(pSLEditData, len) )
			return FALSE;
    	}

    pSLEditData->dataEnd = len;
    edit_memcpy ( pSLEditData->buffer, text, len );
    neCheckBufferSize ( hWnd, pSLEditData );

    pSLEditData->editPos        = 0;
    pSLEditData->scrollX	= 0;
    pSLEditData->caretX 	= 0;

    if( (hWnd->userdata & EST_FOCUSED) )
		SetCaretPos ( pSLEditData->caretX, pSLEditData->topMargin );

    InvalidateRect (hWnd, NULL, FALSE);
    return TRUE;
}


/*
 *  Set FONT
 */
static void neSetFont ( HWND hWnd, HFONT hFont, BOOL bRedraw )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

    pSLEditData->hFont = (HFONT)hFont;
    ShowWindow(hWnd, SW_HIDE);
    ShowWindow(hWnd, SW_SHOWNA);
    pSLEditData->charHeight = neGetTextHeight ( hWnd, NULL );
    if( bRedraw )
		InvalidateRect ( hWnd, NULL, TRUE );
}



/*
 *  Calculate the password chr with
 */
static int neGetPasswdCharWith ( HDC hdc, EDITCHAR pwdChar )
{
    int xw, xh, xb;

    GdSetFont(hdc->font->pfont);
    GdGetTextSize ( hdc->font->pfont, &pwdChar, 1,
    		    	&xw, &xh, &xb, MWTF_ASCII );
    return xw;
}


/*
 * Get height of character
 */
static int neGetTextHeight ( HWND hWnd, HDC hdc )
{
    int xw, xh, xb;
    BOOL bRelDC = FALSE;

    if( hdc == NULL ) hdc = GetDC(hWnd), bRelDC=TRUE;
    SelectObject ( hdc, ((PSLEDITDATA) (hWnd->userdata2))->hFont );
    GdSetFont(hdc->font->pfont);
    GdGetTextSize ( hdc->font->pfont, "X", 1,
    		    	&xw, &xh, &xb, MWTF_ASCII );
    if( bRelDC ) ReleaseDC ( hWnd, hdc );
    return xh;
}

/*
 *  Calculate a string with
 */
static int neGetTextWith ( HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData, const EDITCHAR *txt, int len )
{
    int xw, xh, xb;
    DWORD dwStyle = hWnd->style;
    BOOL bRelDC = FALSE;

    if( dwStyle & dwStyle & ES_PASSWORD )
		return len * neGetPasswdCharWith ( hdc, pSLEditData->passwdChar );

    if( hdc == NULL ) hdc = GetDC(hWnd), bRelDC=TRUE;
    SelectObject ( hdc, pSLEditData->hFont );
    GdSetFont(hdc->font->pfont);
    GdGetTextSize ( hdc->font->pfont, txt, len, &xw, &xh, &xb, MWTF_ASCII );
    if( bRelDC ) ReleaseDC ( hWnd, hdc );
    return xw;
}



/*
 *  Calculate client area width
 */
static int edtGetOutWidth (const HWND pCtrl)
{
    return pCtrl->clirect.right - pCtrl->clirect.left
            - ((PSLEDITDATA)(pCtrl->userdata2))->leftMargin
            - ((PSLEDITDATA)(pCtrl->userdata2))->rightMargin;
}



/*
 *  Output a string in a PASSWORD EDIT
 */
static void neTextOutPwd ( HDC hdc, int x, int y, EDITCHAR pwdChar, int len )
{
    int i, xs;

    xs = neGetPasswdCharWith ( hdc, pwdChar );

    for ( i=0; i < len; i++ )
    	{
		TextOut ( hdc, x, y, &pwdChar, 1 );
		x += xs;
		}
}



/*
 *  Determine the index position based on coords
 *  When exit, the pPoint is set to the position
 *  that the caret should be moved on.
 */
static int neIndexFromPos ( HWND hWnd, POINT *pPoint )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
    HDC hdc = GetDC ( hWnd );
    int i, txts;

    SelectObject ( hdc, pSLEditData->hFont );
    pPoint->x -= pSLEditData->leftMargin + 2;
    pPoint->y -= pSLEditData->topMargin;

    for ( i=0, txts=0; i <= pSLEditData->dataEnd; i++ )
    	{
		txts = neGetTextWith ( hWnd, hdc, pSLEditData, pSLEditData->buffer, i );
		if( txts >= (pPoint->x+pSLEditData->scrollX) )
			break;
    	}

    if( i > pSLEditData->dataEnd ) i = pSLEditData->dataEnd;

    pPoint->x = txts - pSLEditData->scrollX + pSLEditData->leftMargin;
    ReleaseDC ( hWnd, hdc );
    return i;
}


/*
 *  Check if scroll pos should be changed.
 */
static BOOL neRecalcScrollPos ( HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData )
{
    BOOL reldc = FALSE;
    int txts, xs;
    int lastscroll = pSLEditData->scrollX;

    if( hdc == NULL )
        {
		hdc = GetDC ( hWnd );
		reldc = TRUE;
        }

    SelectObject ( hdc, pSLEditData->hFont );
    xs = edtGetOutWidth ( hWnd );
    txts = neGetTextWith ( hWnd, hdc, pSLEditData, pSLEditData->buffer, pSLEditData->editPos );

    if( txts < pSLEditData->scrollX )
		pSLEditData->scrollX = txts;
    else
    if( (txts - pSLEditData->scrollX) > xs )
        {
    	int scrollStep = (txts - pSLEditData->scrollX) - xs;
		if( (pSLEditData->editPos < pSLEditData->dataEnd) &&
			(scrollStep < (xs/2)) &&
			!(hWnd->userdata & (EST_SELSCROLLLEFT | EST_SELSCROLLRIGHT))
		  ) scrollStep = xs/2;

		pSLEditData->scrollX += scrollStep;
		}

    pSLEditData->caretX = txts - pSLEditData->scrollX + pSLEditData->leftMargin;

    if( reldc )
		ReleaseDC ( hWnd, hdc );

    return (lastscroll != pSLEditData->scrollX);
}


/*
 *  Draw the text in the client area
 */
static void neDrawText ( HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData )
{
    DWORD dwStyle = hWnd->style;
    BOOL bRelDC = FALSE;
    RECT rc;
    int bkcol, fgcol;
    EDITCHAR *pTxt, *pEnd;
    int tot = 0;

    if( GetFocus() == hWnd ) HideCaret ( hWnd );

    if( hdc == NULL )
		{
		hdc = GetDC ( hWnd );
		bRelDC = TRUE;
		}

    if( dwStyle & WS_DISABLED )
        bkcol = GetSysColor(COLOR_INACTIVECAPTIONTEXT);
    else
		bkcol = GetSysColor(COLOR_INFOBK);

    fgcol = GetSysColor(COLOR_INFOTEXT);

    SelectObject ( hdc, pSLEditData->hFont );

    rc.left = pSLEditData->leftMargin - pSLEditData->scrollX;
    rc.top = pSLEditData->topMargin;

    pTxt = pSLEditData->buffer;
    pEnd = pTxt + pSLEditData->dataEnd;
    tot = 0;
    SetTextColor ( hdc, fgcol );
    SetBkColor ( hdc, bkcol );
    SetBkMode ( hdc, OPAQUE );

    while ( pTxt < pEnd )
		{
		int count;

		if( (pSLEditData->selStart < pSLEditData->selEnd) &&
			(tot < pSLEditData->selStart) )
			count = pSLEditData->selStart;
		else
		if( (pSLEditData->selStart < pSLEditData->selEnd) &&
			(tot >= pSLEditData->selStart) && (tot < pSLEditData->selEnd) )
			{
			count = pSLEditData->selEnd - pSLEditData->selStart;
			SetTextColor ( hdc, bkcol );
			SetBkColor ( hdc, RGB(0,0,255) );
			}
		else
			{
			count = pSLEditData->dataEnd - tot;
			SetTextColor ( hdc, fgcol );
			SetBkColor ( hdc, bkcol );
			}

		if( count > 0 )
			{
			if( dwStyle & ES_PASSWORD )
			neTextOutPwd ( hdc, rc.left, rc.top, pSLEditData->passwdChar, count );
			else
			neTextOut ( hdc, rc.left, rc.top, pTxt, count );

			rc.left += neGetTextWith ( hWnd, hdc, pSLEditData, pTxt, count );
			pTxt += count;
			tot += count;
			}
		}

    if( bRelDC )
		ReleaseDC ( hWnd, hdc );

    if( GetFocus() == hWnd )
		ShowCaret ( hWnd );
}


/*
 *  neNCPaint
 */
static void neNCPaint ( HWND hWnd, WPARAM wParam )
{
    HDC         hdc;
    RECT	rc;
    DWORD 	dwStyle = hWnd->style;

    hdc = wParam? (HDC)wParam: GetWindowDC (hWnd);
	GetClientRect (hWnd, &rc);
    if( dwStyle & WS_DISABLED )
        FastFillRect(hdc, &rc, GetSysColor(COLOR_INACTIVECAPTIONTEXT));
    else
		FastFillRect(hdc, &rc, GetSysColor(COLOR_INFOBK));
    
	GetWindowRect(hWnd, &rc);

    if (dwStyle & WS_BORDER)
		Draw3dInset(hdc, rc.left, rc.top,
					rc.right-rc.left, rc.bottom-rc.top);

    if (!wParam)
		ReleaseDC (hWnd, hdc);
}



/*
 *  PAINT the EDIT CLIENT AREA
 */
static void nePaint ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    RECT rect, rc;
    PAINTSTRUCT ps;
    DWORD dwStyle = hWnd->style;
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
    HDC hdc;

    hdc = BeginPaint ( hWnd, &ps );
    GetClientRect ( hWnd, &rect );

    if( dwStyle & WS_DISABLED )
        {
		rc.left=0; rc.top=0; rc.bottom=rect.bottom; rc.right=rect.right;
		FillRect(hdc,&rc,GetStockObject(LTGRAY_BRUSH));
        SetBkColor (hdc, LTGRAY/*COLOR_lightgray*/);
        }
    else
		{
		rc.left=0; rc.top=0; rc.bottom=rect.bottom; rc.right=rect.right;
		FillRect(hdc,&rc,GetStockObject(WHITE_BRUSH));
		SetBkColor (hdc, WHITE/*COLOR_lightwhite*/);
    	}

    SetTextColor ( hdc, BLACK/*COLOR_black*/ );
    SelectObject ( hdc, pSLEditData->hFont );


    neDrawText ( hWnd, hdc, pSLEditData );

    EndPaint (hWnd, &ps);
}




/*
 *  Invalidate client area
 */
static void neInvalidateClient  ( HWND hWnd )
{
    RECT InvRect;
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

    InvRect.left = pSLEditData->leftMargin;
    InvRect.top = pSLEditData->topMargin;
    InvRect.right = hWnd->clirect.right - hWnd->clirect.left;
    InvRect.bottom = hWnd->clirect.bottom - hWnd->clirect.top;
    InvalidateRect (hWnd, &InvRect, FALSE);
}


/*
 *  Set the focus, create caret
 */
static void neSetFocus ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

    if( (hWnd->userdata & EST_FOCUSED) != 0 )
        return;

    hWnd->userdata |= EST_FOCUSED;

    pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

    if( (hWnd->userdata & EST_REPLACE) )
		CreateCaret ( hWnd, NULL, mwDefEditCaretSizeIns, pSLEditData->charHeight );
    else
		CreateCaret ( hWnd, NULL, mwDefEditCaretSize, pSLEditData->charHeight );

    SetCaretPos ( pSLEditData->caretX, pSLEditData->topMargin);
    ShowCaret ( hWnd );

    SendMessage (GetParent(hWnd), WM_COMMAND,
       			 (WPARAM) MAKELONG (hWnd->id, EN_SETFOCUS), (LPARAM) hWnd);
}



/*
 * Kill focus
 */
static void neKillFocus ( HWND hWnd )
{
    hWnd->userdata &= ~EST_FOCUSED;
    HideCaret ( hWnd );
    DestroyCaret ();
    SendMessage ( GetParent(hWnd), WM_COMMAND,
	 	  		  (WPARAM) MAKELONG (hWnd->id, EN_KILLFOCUS), (LPARAM)hWnd);
}

/*
 *  Move selection with new cursor position
 */
static BOOL neMoveSelection ( PSLEDITDATA pSLEditData )
{
    int s = pSLEditData->selStart;
    int e = pSLEditData->selEnd;

    if( pSLEditData->editPos < pSLEditData->selCenter )
		{
		pSLEditData->selStart = pSLEditData->editPos;
		pSLEditData->selEnd = pSLEditData->selCenter;
		}
    else
		{
		pSLEditData->selEnd = pSLEditData->editPos;
		pSLEditData->selStart = pSLEditData->selCenter;
		}

    return ( (s != pSLEditData->selStart) || (e != pSLEditData->selEnd) );
}


/*
 *  Handle mouse down
 */
static void neMouseLButtonDown ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    int i;
    POINT pt;
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);
    i = neIndexFromPos ( hWnd, &pt );

    //  If a selection was present, remove and redraw
    if( pSLEditData->selStart < pSLEditData->selEnd )
		neInvalidateClient ( hWnd );

    pSLEditData->editPos = i;
    pSLEditData->selStart = i;
    pSLEditData->selCenter = i;
    pSLEditData->selEnd = i;
    pSLEditData->caretX = pt.x;
    SetCaretPos ( pSLEditData->caretX, pSLEditData->topMargin );
    SetCapture ( hWnd );
}



/*
 *  Handle mouse move
 */
static void neMouseMove ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
    POINT pt;
    int i;

    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);

    if( GetCapture() != hWnd ) return;

    ScreenToClient ( hWnd, &pt );


    do  {
		if( pt.x < pSLEditData->leftMargin )
			{
			if( !(hWnd->userdata & EST_SELSCROLLLEFT) )
				{
				hWnd->userdata |= EST_SELSCROLLLEFT;
				SetTimer ( hWnd, IDTM_SELSCROLLLEFT, TM_SELSCROLL, NULL );
				pt.x = pSLEditData->leftMargin;
				break;
				}
			return;
			}

		if( pt.x > pSLEditData->leftMargin + edtGetOutWidth(hWnd) )
			{
			if( !(hWnd->userdata & EST_SELSCROLLRIGHT) )
				{
				hWnd->userdata |= EST_SELSCROLLRIGHT;
				SetTimer ( hWnd, IDTM_SELSCROLLRIGHT, TM_SELSCROLL, NULL );
				pt.x = pSLEditData->leftMargin + edtGetOutWidth ( hWnd );
				break;
				}
			return;
			}

		if( (hWnd->userdata & EST_SELSCROLLLEFT) )
			KillTimer ( hWnd, IDTM_SELSCROLLLEFT );
		if( (hWnd->userdata & EST_SELSCROLLRIGHT) )
			KillTimer ( hWnd, IDTM_SELSCROLLRIGHT );
		hWnd->userdata &= ~(EST_SELSCROLLLEFT | EST_SELSCROLLRIGHT);
		}
	while ( 0 );

    i = neIndexFromPos ( hWnd, &pt );

    pSLEditData->editPos = i;
    neMoveSelection ( pSLEditData );

    pSLEditData->caretX = pt.x;
    SetCaretPos ( pSLEditData->caretX, pSLEditData->topMargin );
    neDrawText ( hWnd, NULL, pSLEditData );
}



/*
 *  Handle mouse LBUTTUP
 */
static void neMouseLButtonUp ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    if( GetCapture() != hWnd ) return;
    ReleaseCapture ();
    if( (hWnd->userdata & EST_SELSCROLLLEFT) )
		KillTimer ( hWnd, IDTM_SELSCROLLLEFT );
    if( (hWnd->userdata & EST_SELSCROLLRIGHT) )
		KillTimer ( hWnd, IDTM_SELSCROLLRIGHT );
}




/*
 *  Handle mouse DOUBLE CLICK
 */
static void neMouseDblClk ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

    pSLEditData->selStart = 0;
    pSLEditData->selEnd = pSLEditData->dataEnd;
    neInvalidateClient ( hWnd );
}




/*
 *  Hadle WM_TIMER message for selection scrolling
 */
static void neTimerMessage ( HWND hWnd, WPARAM wParam )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
    int lastpos = pSLEditData->editPos;

    if( wParam == IDTM_SELSCROLLLEFT )
		{
		if( pSLEditData->editPos > 0 )
			pSLEditData->editPos--;
		}
    else
    if( wParam == IDTM_SELSCROLLRIGHT )
		{
		if( pSLEditData->editPos < pSLEditData->dataEnd )
			pSLEditData->editPos++;
		}

    if( lastpos != pSLEditData->editPos )
		{
		neMoveSelection ( pSLEditData );
		if( neRecalcScrollPos(hWnd, NULL, pSLEditData) )
			neInvalidateClient ( hWnd );
		}
}



/*
 *  Delete the text in the selection
 */
static BOOL neCutSelectedText ( HWND hWnd, BOOL bCopyToClipb )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
    int count;

    if( bCopyToClipb )
		neCopyToCliboard ( hWnd, pSLEditData );

    if( pSLEditData->selStart < pSLEditData->selEnd )
		{
		if( pSLEditData->selStart < 0 )
			pSLEditData->selStart = 0;
		if( pSLEditData->selEnd > pSLEditData->dataEnd )
			pSLEditData->selEnd = pSLEditData->dataEnd;
		count = pSLEditData->selEnd - pSLEditData->selStart;
		memmove ( pSLEditData->buffer + pSLEditData->selStart,
				  pSLEditData->buffer + pSLEditData->selEnd,
				  pSLEditData->dataEnd - pSLEditData->selEnd );
		pSLEditData->dataEnd -= count;
		pSLEditData->editPos = pSLEditData->selStart;
		if( pSLEditData->editPos > pSLEditData->dataEnd )
			pSLEditData->editPos = pSLEditData->dataEnd;

		neRecalcScrollPos ( hWnd, NULL, pSLEditData );
		neInvalidateClient ( hWnd );
		pSLEditData->selStart = pSLEditData->selEnd = 0;
		neCheckBufferSize ( hWnd, pSLEditData );
		return TRUE;
		}

    return FALSE;
}



/*
 *  A Control character has been pressed
 */
static int neKeyDown ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
    int lastPos = pSLEditData->editPos;
    RECT InvRect;
    BOOL bRedraw = FALSE;
    BOOL onWord = FALSE;

    //printf ( "KEYDOWN: %08X %08X\n", (int)wParam, (int)lParam );

    //  If key isn't a control key, exit
    if( !(lParam & (1 << 24) ) && (wParam != VK_BACK) )
		return 0;

    switch ( wParam )
    	{
		case VK_LEFT:
			if( pSLEditData->editPos > 0 )
			pSLEditData->editPos--;
			if( (hWnd->userdata & EST_CTRL) && (pSLEditData->editPos < pSLEditData->dataEnd) )
				{
				onWord = neIsWord(pSLEditData->buffer[pSLEditData->editPos]);
				while ( (pSLEditData->editPos > 0) &&
						(neIsWord(pSLEditData->buffer[pSLEditData->editPos]) ==
					onWord)
					) pSLEditData->editPos--;
				}

		    if( (hWnd->userdata & EST_SHIFT) )
				bRedraw = neMoveSelection ( pSLEditData );
			else
				{
				bRedraw = (pSLEditData->selStart < pSLEditData->selEnd);
				pSLEditData->selStart = pSLEditData->selEnd = pSLEditData->editPos;
				}
			break;

		case VK_RIGHT:
			if( pSLEditData->editPos < pSLEditData->dataEnd )
				pSLEditData->editPos++;
			if( (hWnd->userdata & EST_CTRL) && (pSLEditData->editPos < pSLEditData->dataEnd) )
				{
				onWord = neIsWord(pSLEditData->buffer[pSLEditData->editPos]);
				while ( (pSLEditData->editPos < pSLEditData->dataEnd) &&
						(neIsWord(pSLEditData->buffer[pSLEditData->editPos]) == onWord)
					  ) pSLEditData->editPos++;
				}

			if( (hWnd->userdata & EST_SHIFT) )
				bRedraw = neMoveSelection ( pSLEditData );
			else
				{
				bRedraw = (pSLEditData->selStart < pSLEditData->selEnd);
				pSLEditData->selStart = pSLEditData->selEnd = pSLEditData->editPos;
				}
			break;

		case VK_HOME:
			pSLEditData->editPos = 0;
			if( (hWnd->userdata & EST_SHIFT) )
				bRedraw = neMoveSelection ( pSLEditData );
			else
				{
				bRedraw = (pSLEditData->selStart < pSLEditData->selEnd);
				pSLEditData->selStart = pSLEditData->selEnd = pSLEditData->editPos;
				}
			break;

		case VK_END:
			pSLEditData->editPos = pSLEditData->dataEnd;
			if( (hWnd->userdata & EST_SHIFT) )
				bRedraw = neMoveSelection ( pSLEditData );
			else
				{
				bRedraw = (pSLEditData->selStart < pSLEditData->selEnd);
				pSLEditData->selStart = pSLEditData->selEnd = pSLEditData->editPos;
				}
			break;

		case VK_BACK:
			if( neCutSelectedText(hWnd, FALSE) )
				break;
			if( pSLEditData->editPos > 0 )
				{
				pSLEditData->editPos--;
				memmove ( pSLEditData->buffer+pSLEditData->editPos,
						pSLEditData->buffer+pSLEditData->editPos+1,
						(pSLEditData->dataEnd - pSLEditData->editPos) * SZEDITCHAR );
				pSLEditData->dataEnd--;
				neCheckBufferSize ( hWnd, pSLEditData );
				bRedraw = TRUE;
				}
			break;

		case VK_DELETE:
			if( neCutSelectedText(hWnd, ((hWnd->userdata & EST_SHIFT) != 0)) )
				break;
			if( pSLEditData->editPos < pSLEditData->dataEnd )
				{
				memmove ( pSLEditData->buffer+pSLEditData->editPos,
						pSLEditData->buffer+pSLEditData->editPos+1,
						(pSLEditData->dataEnd - pSLEditData->editPos) * SZEDITCHAR );
				pSLEditData->dataEnd--;
				neCheckBufferSize ( hWnd, pSLEditData );
				bRedraw = TRUE;
				}
			break;

		case VK_INSERT:
			if( (hWnd->userdata & EST_SHIFT) )
				neCharPressed ( hWnd, -1, -1 );
			else
			if( (hWnd->userdata & EST_CTRL) )
				neCopyToCliboard ( hWnd, pSLEditData );
			else
				{
				hWnd->userdata ^= EST_REPLACE;
				if( GetFocus() == hWnd )
					{
					DestroyCaret ();
					if( (hWnd->userdata & EST_REPLACE) )
						CreateCaret ( hWnd, NULL, 3, pSLEditData->charHeight );
					else
						CreateCaret ( hWnd, NULL, 1, pSLEditData->charHeight );
					SetCaretPos ( pSLEditData->caretX, pSLEditData->topMargin );
					ShowCaret ( hWnd );
					}
				}
			break;

		case VK_LCONTROL:
		case VK_RCONTROL:
		case VK_CONTROL:
			hWnd->userdata |= EST_CTRL;
			break;

		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_SHIFT:
			if( GetCapture() != hWnd )
				{
				hWnd->userdata |= EST_SHIFT;
				pSLEditData->selCenter = pSLEditData->editPos;
				}
			break;
		}

	if( (lastPos != pSLEditData->editPos) || bRedraw )
		{
		if( neRecalcScrollPos(hWnd, NULL, pSLEditData) || bRedraw )
			{
			InvRect.left = pSLEditData->leftMargin;
			InvRect.top = pSLEditData->topMargin;
			InvRect.right = hWnd->clirect.right - hWnd->clirect.left;
			InvRect.bottom = hWnd->clirect.bottom - hWnd->clirect.top;
			InvalidateRect (hWnd, &InvRect, FALSE);
			}
		SetCaretPos ( pSLEditData->caretX, pSLEditData->topMargin );
		}

    return 0;
}


/*
 *  Key released
 */
static int neKeyUp ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    //  If key isn't a control key, exit
    if( !(lParam & (1 << 24) ) && (wParam != VK_BACK) )
		return 0;

    switch ( wParam )
    	{
		case VK_LCONTROL:
		case VK_RCONTROL:
			hWnd->userdata &= ~EST_CTRL;
			break;

		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_SHIFT:
			hWnd->userdata &= ~EST_SHIFT;
			break;
		}

    return 0;
}



/*
 *  A Character has been pressed
 */
static int neCharPressed ( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    EDITCHAR locBuffer[2];
    EDITCHAR *charBuffer = locBuffer;
    int  i, chars, inserting;
    PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
    DWORD dwStyle = hWnd->style;
    BOOL isPasting = (((long)wParam == -1) && ((long)lParam == -1));


    if( dwStyle & ES_READONLY )
        return 0;

    //printf ( "char: %08X - %08X\n", (int)wParam, (int)lParam );


    if( (wParam == 0xD) && (hWnd->userdata & EST_CTRL) )
	printf ( "CTRL+ENTER!!!!!\n" );

    //   check if called for pasting clipboard
    if( isPasting )
		{
		if( neClipboard == NULL )
			return 0;

		charBuffer = neClipboard;
		chars = neClipboardSize;
		}
    else
		{
		chars = 1;
		charBuffer[0] = (EDITCHAR) LOWORD(wParam);
		switch ( LOWORD(wParam) )
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

    //  If there is a selection, remove it.
    neCutSelectedText ( hWnd, FALSE );

    if( (hWnd->userdata & EST_REPLACE) && !isPasting )
		{
		if( pSLEditData->dataEnd == pSLEditData->editPos )
			inserting = chars;
		else
			inserting = 0;
    	}
    else
		inserting = chars;

    /* check space */
    if( (pSLEditData->hardLimit >= 0) &&
    	(pSLEditData->dataEnd + inserting > pSLEditData->hardLimit) )
    	{
		SendMessage (GetParent(hWnd), WM_COMMAND,
	    			 (WPARAM) MAKELONG (hWnd->id, EN_MAXTEXT), (LPARAM) hWnd);
		return 0;
        }

    //   Increase dataEnd and check buffer size
    if( !neIncDataEnd(pSLEditData, inserting) )
		{
		SendMessage ( GetParent(hWnd), WM_COMMAND,
					  (WPARAM) MAKELONG (hWnd->id, EN_MAXTEXT), (LPARAM) hWnd);
		return 0;
		}

    if( inserting == -1 )
		{
		for ( i=pSLEditData->editPos; i < pSLEditData->dataEnd-1; i++ )
		    pSLEditData->buffer [i] = pSLEditData->buffer [i + 1];
    	}
    else if( inserting > 0 )
		{
		for ( i=pSLEditData->dataEnd-1; i > pSLEditData->editPos-1; i--)
	    	pSLEditData->buffer[i] = pSLEditData->buffer [i - inserting];
    	}

    for ( i=0; i < chars; i++ ) {
		if( dwStyle & ES_UPPERCASE )
        	pSLEditData->buffer [pSLEditData->editPos + i] = toupper(charBuffer [i]);
		else
		if( dwStyle & ES_LOWERCASE )
        	pSLEditData->buffer [pSLEditData->editPos + i] = tolower(charBuffer [i]);
		else
        	pSLEditData->buffer [pSLEditData->editPos + i] = charBuffer [i];
	}

    pSLEditData->editPos += chars;
    pSLEditData->selCenter = pSLEditData->editPos;

    if( neRecalcScrollPos(hWnd, NULL, pSLEditData) )
		neInvalidateClient  ( hWnd );
    else
		neDrawText ( hWnd, NULL, pSLEditData );

    SetCaretPos ( pSLEditData->caretX, pSLEditData->topMargin );

    SendMessage ( GetParent(hWnd), WM_COMMAND,
		  		  (WPARAM) MAKELONG (hWnd->id, EN_CHANGE), (LPARAM) hWnd );
    return 0;
}







/*
 * WINDOW PROCEDURE
 */
LRESULT CALLBACK
SLEditCtrlProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND	pCtrl;
    DWORD       dwStyle;
    PSLEDITDATA pSLEditData;

    pCtrl       = hWnd;
    dwStyle     = pCtrl->style;

    switch (message)
		{
		case WM_CREATE:
			neCreate ( hWnd );
			break;

		case WM_DESTROY:
			neDestroy ( hWnd );
			break;

		case WM_SETFONT:
			neSetFont ( hWnd, (HFONT)wParam, (BOOL) LOWORD(lParam) );
			break;

		case WM_GETFONT:
			pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
			return (LRESULT)pSLEditData->hFont;

		case WM_KILLFOCUS:
			neKillFocus ( hWnd );
			break;

		case WM_SETFOCUS:
			neSetFocus ( hWnd, wParam, lParam );
			break;

		case WM_ENABLE:
			if( (!(dwStyle & WS_DISABLED) && !wParam) ||
				((dwStyle & WS_DISABLED) && wParam) )
				{
				if( wParam )
					pCtrl->style &= ~WS_DISABLED;
				else
					pCtrl->style |=  WS_DISABLED;

				InvalidateRect (hWnd, NULL, FALSE);
				}
			break;

		case WM_NCCALCSIZE:
			{
			LPNCCALCSIZE_PARAMS lpnc;
			/* calculate client rect from passed window rect in rgrc[0]*/
			lpnc = (LPNCCALCSIZE_PARAMS)lParam;
			if( GetWindowLong(hWnd, GWL_STYLE) & WS_BORDER )
				InflateRect(&lpnc->rgrc[0], -2, -2);
			}
			break;

		case WM_NCPAINT:
			neNCPaint ( hWnd, wParam );
			break;

		case WM_PAINT:
			nePaint ( hWnd, wParam, lParam );
			break;

		case WM_KEYDOWN:
			return neKeyDown ( hWnd, wParam, lParam );

		case WM_KEYUP:
			return neKeyUp ( hWnd, wParam, lParam );

		case WM_CHAR:
			return neCharPressed ( hWnd, wParam, lParam );

		case WM_GETTEXTLENGTH:
			pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
			return pSLEditData->dataEnd;

		case WM_GETTEXT:
			{
			char*   buffer = (char*)lParam;
			int     len;
			pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
			len = min ((int)wParam, pSLEditData->dataEnd);
			memcpy_fromedit (buffer, pSLEditData->buffer, len);
			buffer [len] = '\0';
			return len;
			}

		case WM_SETTEXT:
			neSetText ( hWnd, (const char *)lParam );
			break;

		case WM_LBUTTONDBLCLK:
			neMouseDblClk ( hWnd, wParam, lParam );
			break;

		case WM_LBUTTONDOWN:
			neMouseLButtonDown ( hWnd, wParam, lParam );
			break;

		case WM_LBUTTONUP:
		case WM_NCLBUTTONUP:
			neMouseLButtonUp ( hWnd, wParam, lParam );
			break;

		case WM_NCMOUSEMOVE:
			neMouseMove ( hWnd, wParam, lParam );
			break;

		case WM_TIMER:
			neTimerMessage ( hWnd, wParam );
			break;

		case WM_GETDLGCODE:
			return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;

		case EM_SETREADONLY:
			if (wParam)
				pCtrl->style/*dwStyle*/ |= ES_READONLY;
			else
				pCtrl->style/*dwStyle*/ &= ~ES_READONLY;
			return 0;

		case EM_SETPASSWORDCHAR:
			pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
			if( pSLEditData->passwdChar != (int)wParam )
				{
				if( dwStyle & ES_PASSWORD )
					{
					pSLEditData->passwdChar = (int)wParam;
					InvalidateRect (hWnd, NULL, TRUE);
					}
				}
			return 0;

		case EM_GETPASSWORDCHAR:
			pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
			(int*) lParam = pSLEditData->passwdChar;
			return 0;

		case EM_LIMITTEXT:
			{
			int newLimit = (int)wParam;

			if( newLimit >= 0 )
				{
				pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
				if (pSLEditData->bufferLen < newLimit)
					pSLEditData->hardLimit = -1;
				else
					pSLEditData->hardLimit = newLimit;
				}
			}
			return 0;

		default:
			return DefWindowProc (hWnd, message, wParam, lParam);
			break;
		}

	return 0;
}




int WINAPI MwRegisterEditControl(HINSTANCE hInstance)
{
	WNDCLASS	wc;

	wc.style	= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc	= (WNDPROC)SLEditCtrlProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon	= NULL;
	wc.hCursor	= 0; /*LoadCursor(NULL, IDC_ARROW);*/
	wc.hbrBackground= GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= "EDIT";

	return RegisterClass(&wc);
}
