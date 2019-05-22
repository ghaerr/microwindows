/*
 * NEW "EDIT control" for Microwindows win32 api.
 *
 * Copyright (C) 2003, Gabriele Brugnoni
 * <gabrielebrugnoni@dveprojects.com>
 * DVE Prog. El. - Varese, Italy
 *
 * Based on microwindows edit.c
 *  Copyright (C) 1999, 2000, Wei Yongming.
 *  Portions Copyright (c) 2000, 2019 Greg Haerr <greg@censoft.com>
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
**    * BIG5 font and chinese support
**    * Multiline.
**    * UNICODE ??.
*/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MWINCLUDECOLORS
#include "windows.h"		/* windef.h, winuser.h */
#include "wintern.h"
#include "wintools.h"
#include "device.h"		/* GdGetTextSize */
#include "intl.h"


// NOTE: 
#if HAVE_HZK_SUPPORT | HAVE_BIG5_SUPPORT
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

char mwDefEditCaretSize = 1;
char mwDefEditCaretSizeIns = 3;



typedef unsigned short EDITCHAR;
#define SZEDITCHAR	sizeof(EDITCHAR)



/*
 * Edit structure
 */
typedef struct tagSLEDITDATA {
	HFONT hFont;		/* hFont used */
	int bufferLen;		/* length of buffer */

	int dataEnd;		/* data end position */
	int editPos;		/* current edit position */
	int caretX;		/* caret X position in box */
	int caretRow;		/* caret Y position in box */
	int scrollX;		/* X scrolling offset */
	int scrollRow;		/* Y scrolling offset */
	int epX, epY;		/* coordinates of edit position */
	int epFirstIdx;		/* index of the first character in line with editPos */
	int epLineCount;	/* count of characters in line with editPos */
	int epLineOX;		/* X origin of current line. Typically > 0 when right aligned */
	int epLineAlign;	/* Alignement of current line. 0 = left align, 1 = right align */

	int selStart;		/* selection start position */
	int selEnd;		/* selection end position */
	int selCenter;		/* selection cursor center */

	int passwdChar;		/* password character */

	int charHeight;		/* height of character */

	int leftMargin;		/* left margin */
	int topMargin;		/* top margin */
	int rightMargin;	/* right margin */
	int bottomMargin;	/* bottom margin */

	int hardLimit;		/* hard limit */

	int lastOp;		/* last operation */
	int lastPos;		/* last operation position */
	int affectedLen;	/* affected len of last operation */
	int undoBufferLen;	/* undo buffer len */
	EDITCHAR undoBuffer[LEN_SLEDIT_UNDOBUFFER];	/* Undo buffer; */
	EDITCHAR *buffer;	/* buffer */
	int cLines;		/* count of allocated (visible) lines info */
} SLEDITDATA, *PSLEDITDATA;


//  For future implementation of unicode...
#define edit_memcpy		memcpy
#define memcpy_fromedit		memcpy
#define neTextOut		TextOutW


#define neIsWord(c)		( (c) > 32 && (((c) > 127) || isalpha(c) || isdigit(c)) )


// Drawing attributes
#define NEDRAW_ENTIRE			0x0001
#define NEDRAW_ROW				0x0002
#define NEDRAW_CALC_CURSOR		0x0004
#define NEDRAW_CALC_EDITPOS		0x0008

LRESULT CALLBACK SLNewEditCtrlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static int neGetTextHeight(HWND hWnd, HDC hdc);
static int neGetTextWith(HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData, const EDITCHAR * txt, int len);

static int neCharPressed(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void neRecalcRows(HWND hwnd, PSLEDITDATA * ppData);
static void neDrawAllText(HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData, int action);


//  Clipboard for cut and paste.
static EDITCHAR *neClipboard = NULL;
static int neClipboardSize = 0;


/*
 *  Create new edit control
 */
static int
neCreate(HWND hwnd)
{
	PSLEDITDATA pSLEditData;
	HWND pCtrl = hwnd;
	int len;
	//int charH, nl;
	RECT rc;


	GetClientRect(hwnd, &rc);
	//nl = (rc.bottom - rc.top + charH - 1) / charH;

	if (!(pSLEditData = malloc(sizeof(SLEDITDATA)))) {
		EPRINTF("EDIT: malloc error!\n");
		return -1;
	}

	len = 1 + strlen(pCtrl->szTitle);
	if (len < LEN_SLEDIT_BUFFER)
		len = LEN_SLEDIT_BUFFER;

	pCtrl->userdata2 = (ULONG_PTR) pSLEditData;
	pCtrl->userdata = 0;

#ifdef USE_BIG5
	pSLEditData->hFont = CreateFont(12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FF_DONTCARE | DEFAULT_PITCH, "HZXFONT");
#else
	pSLEditData->hFont = GetStockObject(DEFAULT_FONT);
#endif

	pSLEditData->bufferLen = len;
	pSLEditData->editPos = 0;
	pSLEditData->caretX = 0;
	pSLEditData->caretRow = 0;
	pSLEditData->scrollX = 0;
	pSLEditData->scrollRow = 0;
	pSLEditData->epFirstIdx = 0;
	pSLEditData->epLineCount = 0;
	pSLEditData->epLineOX = 0;
	pSLEditData->epLineAlign = 0;

	pSLEditData->selStart = 0;
	pSLEditData->selEnd = 0;
	pSLEditData->passwdChar = '*';
	pSLEditData->leftMargin = MARGIN_EDIT_LEFT;
	pSLEditData->topMargin = MARGIN_EDIT_TOP;
	pSLEditData->rightMargin = MARGIN_EDIT_RIGHT;
	pSLEditData->bottomMargin = MARGIN_EDIT_BOTTOM;

	pSLEditData->hardLimit = -1;

	pSLEditData->buffer =
		(EDITCHAR *) calloc(SZEDITCHAR, pSLEditData->bufferLen);

	/* undo information */
	pSLEditData->lastOp = EDIT_OP_NONE;
	pSLEditData->lastPos = 0;
	pSLEditData->affectedLen = 0;
	pSLEditData->undoBufferLen = LEN_SLEDIT_UNDOBUFFER;
	pSLEditData->undoBuffer[0] = '\0';

	//edit_memcpy ( pSLEditData->buffer, pCtrl->szTitle, len );
	pSLEditData->dataEnd = GdConvertEncoding(pCtrl->szTitle,
						 mwTextCoding,
						 strlen(pCtrl->szTitle),
						 pSLEditData->buffer,
						 MWTF_UC16);

	pSLEditData->charHeight = neGetTextHeight(hwnd, NULL);
	neRecalcRows(hwnd, &pSLEditData);
	return 0;
}


static void
neRecalcRows(HWND hwnd, PSLEDITDATA * ppData)
{
	PSLEDITDATA pSLEditData;
	PSLEDITDATA pCurData = *ppData;
	RECT rc;
	int nl;
	int charH;

	charH = neGetTextHeight(hwnd, NULL);
	GetClientRect(hwnd, &rc);
	nl = (rc.bottom - rc.top + charH - 1) / charH;

	pSLEditData = (PSLEDITDATA) malloc(sizeof(SLEDITDATA));
	if (pSLEditData == NULL)
		return;

	*pSLEditData = *pCurData;
	pSLEditData->charHeight = charH;
	pSLEditData->cLines = nl;
	*ppData = pSLEditData;
	hwnd->userdata2 = (ULONG_PTR) pSLEditData;
	free(pCurData);
}

/*
 *  Destroy an edit control
 */
static void
neDestroy(HWND hwnd)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hwnd->userdata2);

	free(pSLEditData->buffer);
	free(pSLEditData);
	hwnd->userdata2 = 0;
	DestroyCaret();
}


/*
 *  Reallocate the buffer with the new size
 */
static BOOL
neReallocateBuffer(PSLEDITDATA pSLEditData, int len)
{
	EDITCHAR *newbuff;

	if (len < pSLEditData->dataEnd)
		len = pSLEditData->dataEnd;

	newbuff = (EDITCHAR *) malloc(len * SZEDITCHAR);
	if (newbuff == NULL) {
		EPRINTF("Unable to allocate buffer for EDIT control.\n");
		return FALSE;
	}

	memcpy(newbuff, pSLEditData->buffer, len * SZEDITCHAR);
	free(pSLEditData->buffer);
	pSLEditData->bufferLen = len;
	pSLEditData->buffer = newbuff;
	return TRUE;
}


/*
 *  Check if it should reduce allocated buffer
 */
static void
neCheckBufferSize(HWND hWnd, PSLEDITDATA pSLEditData)
{
	if ((pSLEditData->bufferLen - pSLEditData->dataEnd) > LEN_SLEDIT_REMOVEBUFFER)
		neReallocateBuffer(pSLEditData, pSLEditData->dataEnd + LEN_SLEDIT_ADDBUFFER);
}

/*
 *  Increment dataEnd and check if buffer should reallocated
 */
static BOOL
neIncDataEnd(PSLEDITDATA pSLEditData, int delta)
{
	if ((pSLEditData->hardLimit >= 0) &&
	    (pSLEditData->dataEnd + delta >= pSLEditData->hardLimit))
		return FALSE;

	pSLEditData->dataEnd += delta;
	if (pSLEditData->dataEnd > pSLEditData->bufferLen) {
		if (!neReallocateBuffer(pSLEditData, pSLEditData->bufferLen + LEN_SLEDIT_ADDBUFFER)) {
			pSLEditData->dataEnd -= delta;
			return FALSE;
		}
	}

	return TRUE;
}

/*
 *  Copy selection to clipboard
 */
static BOOL
neCopyToCliboard(HWND hWnd, PSLEDITDATA pSLEditData)
{
	if (pSLEditData->selStart < pSLEditData->selEnd) {
		int count = pSLEditData->selEnd - pSLEditData->selStart;
		if (count + pSLEditData->selStart > pSLEditData->dataEnd)
			count = pSLEditData->dataEnd - pSLEditData->selStart;
		if (count > 0) {
			if (neClipboard != NULL)
				free(neClipboard);
			neClipboard = (EDITCHAR *) malloc(count * SZEDITCHAR);
			if (neClipboard == NULL)
				return FALSE;
			memcpy(neClipboard,
			       pSLEditData->buffer + pSLEditData->selStart,
			       count * SZEDITCHAR);
			neClipboardSize = count;
			return TRUE;
		}
	}

	return FALSE;
}


static void
neUpdateCaretPos(HWND hWnd)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	SetCaretPos(pSLEditData->caretX, pSLEditData->topMargin + pSLEditData->caretRow * pSLEditData->charHeight);
}

/*
 *  Parse WM_SETTEXT command
 */
static BOOL
neSetText(HWND hWnd, const char *text)
{
	//DWORD dwStyle = hWnd->style;
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	int len;

	//FIXME: consider UTF8
	len = strlen(text);

	if (pSLEditData->hardLimit >= 0)
		len = min(len, pSLEditData->hardLimit);

	if (len > pSLEditData->bufferLen) {
		if (!neReallocateBuffer(pSLEditData, len))
			return FALSE;
	}

	pSLEditData->dataEnd = GdConvertEncoding(text, mwTextCoding, len, pSLEditData->buffer, MWTF_UC16);

	//edit_memcpy ( pSLEditData->buffer, text, len );
	neCheckBufferSize(hWnd, pSLEditData);

	pSLEditData->editPos = 0;
	pSLEditData->scrollX = 0;
	pSLEditData->caretX = 0;
	pSLEditData->scrollRow = 0;
	pSLEditData->caretRow = 0;

	if ((hWnd->userdata & EST_FOCUSED))
		neUpdateCaretPos(hWnd);

	InvalidateRect(hWnd, NULL, FALSE);
	return TRUE;
}


/*
 *  Set FONT
 */
static void
neSetFont(HWND hWnd, HFONT hFont, BOOL bRedraw)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

	pSLEditData->hFont = (HFONT) hFont;
	neRecalcRows(hWnd, &pSLEditData);
	ShowWindow(hWnd, SW_HIDE);
	ShowWindow(hWnd, SW_SHOWNA);
	if (bRedraw)
		InvalidateRect(hWnd, NULL, TRUE);
}



/*
 *  Calculate the password chr with
 */
static int
neGetPasswdCharWith(HDC hdc, EDITCHAR pwdChar)
{
	int xw, xh, xb;

	GdGetTextSize(hdc->font->pfont, &pwdChar, 1, &xw, &xh, &xb, MWTF_UC16);
	return xw;
}


/*
 * Get height of character
 */
static int
neGetTextHeight(HWND hWnd, HDC hdc)
{
	int xw, xh, xb;
	BOOL bRelDC = FALSE;

	if (hdc == NULL)
		hdc = GetDC(hWnd), bRelDC = TRUE;
	SelectObject(hdc, ((PSLEDITDATA) (hWnd->userdata2))->hFont);
	GdGetTextSize(hdc->font->pfont, "X", 1, &xw, &xh, &xb, MWTF_ASCII);
	if (bRelDC)
		ReleaseDC(hWnd, hdc);
	return xh;
}

/*
 *  Calculate a string with
 */
static int
neGetTextWith(HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData,
	      const EDITCHAR * txt, int len)
{
	int xw, xh, xb;
	DWORD dwStyle = hWnd->style;
	BOOL bRelDC = FALSE;

	if (dwStyle & dwStyle & ES_PASSWORD)
		return len * neGetPasswdCharWith(hdc, pSLEditData->passwdChar);

	if (hdc == NULL)
		hdc = GetDC(hWnd), bRelDC = TRUE;
	SelectObject(hdc, pSLEditData->hFont);
	GdGetTextSize(hdc->font->pfont, txt, len, &xw, &xh, &xb, MWTF_UC16);
	if (bRelDC)
		ReleaseDC(hWnd, hdc);
	return xw;
}



/*
 *  Calculate client area width
 */
static int
edtGetOutWidth(const HWND pCtrl)
{
	return pCtrl->clirect.right - pCtrl->clirect.left
		- ((PSLEDITDATA) (pCtrl->userdata2))->leftMargin
		- ((PSLEDITDATA) (pCtrl->userdata2))->rightMargin;
}



/*
 *  Output a string in a PASSWORD EDIT
 */
static void
neTextOutPwd(HDC hdc, int x, int y, EDITCHAR pwdChar, int len)
{
	int i, xs;

	xs = neGetPasswdCharWith(hdc, pwdChar);

	for (i = 0; i < len; i++) {
		neTextOut(hdc, x, y, &pwdChar, 1);
		x += xs;
	}
}



/*
 *  Determine the index position based on coords
 *  When exit, the pPoint is set to the position
 *  that the caret should be moved on.
 */
static int
neIndexFromPos(HWND hWnd, POINT * pPoint)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	HDC hdc = GetDC(hWnd);
	int i, txts;

	SelectObject(hdc, pSLEditData->hFont);
	pPoint->x -= pSLEditData->leftMargin + 2;
	pPoint->y -= pSLEditData->topMargin;

	for (i = 0, txts = 0; i <= pSLEditData->dataEnd; i++) {
		txts = neGetTextWith(hWnd, hdc, pSLEditData, pSLEditData->buffer, i);
		if (txts >= (pPoint->x + pSLEditData->scrollX))
			break;
	}

	if (i > pSLEditData->dataEnd)
		i = pSLEditData->dataEnd;

	pPoint->x = txts - pSLEditData->scrollX + pSLEditData->leftMargin;
	ReleaseDC(hWnd, hdc);
	return i;
}


/*
 *  Check if scroll pos should be changed.
 *  Return nonzero if the entire window should be redrawn
 */
static BOOL
neRecalcScrollPos(HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData,
		  BOOL checkNewline)
{
	int xs;
	int lastscrollX;
	int lastscrollR;
	int pfirst, palign;

	lastscrollX = pSLEditData->scrollX;
	lastscrollR = pSLEditData->scrollRow;
	xs = edtGetOutWidth(hWnd);
	pfirst = pSLEditData->epFirstIdx;
	palign = pSLEditData->epLineAlign;
	neDrawAllText(hWnd, hdc, pSLEditData, NEDRAW_CALC_CURSOR);

	if (pSLEditData->epX < pSLEditData->scrollX)
		pSLEditData->scrollX = pSLEditData->epX;
	else if ((pSLEditData->epX - pSLEditData->scrollX -
		  (pSLEditData->epLineAlign ? 2 : 0)) > xs) {
		int scrollStep =
			(pSLEditData->epX - pSLEditData->scrollX) - xs;
		if ((pSLEditData->editPos < pSLEditData->dataEnd)
		    && (scrollStep < (xs / 2))
		    && !(hWnd->
			 userdata & (EST_SELSCROLLLEFT | EST_SELSCROLLRIGHT))
			)
			scrollStep = xs / 2;

		pSLEditData->scrollX += scrollStep;
	}

	pSLEditData->caretX = pSLEditData->epX - pSLEditData->scrollX +
		pSLEditData->leftMargin;
	pSLEditData->caretRow = pSLEditData->epY / pSLEditData->charHeight - pSLEditData->scrollRow;
	if (pSLEditData->cLines > 1 && pSLEditData->caretRow >= pSLEditData->cLines - 1) {
		int delta = pSLEditData->caretRow - pSLEditData->cLines + 2;
		pSLEditData->scrollRow += delta;
		pSLEditData->caretRow -= delta;
	} else if (pSLEditData->caretRow < 0) {
		pSLEditData->scrollRow += pSLEditData->caretRow;
		pSLEditData->caretRow = 0;
	}

	return ((lastscrollX != pSLEditData->scrollX) ||
		(lastscrollR != pSLEditData->scrollRow) ||
		((pSLEditData->epFirstIdx > pfirst || pSLEditData->epLineAlign != palign) && checkNewline));
}



/*
 *  Draw or calculate the text in the client area
 */
static void
neDrawAllText(HWND hWnd, HDC hdc, PSLEDITDATA pSLEditData, int action)
{
	DWORD dwStyle = hWnd->style;
	BOOL bRelDC = FALSE;
	RECT rc;
	int bkcol, fgcol;
	EDITCHAR *pTxt, *pEnd, *edittext;
	int szy;
	int tot = 0;
	int cy = 0;
	int cx;
	int ln;
	int xs;
	int done;
	unsigned long attrib = 0;

	if ((action & (NEDRAW_ENTIRE | NEDRAW_ROW)) && (GetFocus() == hWnd))
		HideCaret(hWnd);

	if (hdc == NULL) {
		hdc = GetDC(hWnd);
		bRelDC = TRUE;
	}

	if (dwStyle & WS_DISABLED)
		bkcol = GetSysColor(COLOR_INACTIVECAPTIONTEXT);
	else
		bkcol = GetSysColor(COLOR_INFOBK);

	fgcol = GetSysColor(COLOR_INFOTEXT);

	SelectObject(hdc, pSLEditData->hFont);

	GetClientRect(hWnd, &rc);
	rc.left = pSLEditData->leftMargin - pSLEditData->scrollX;
	rc.top = pSLEditData->topMargin -
		pSLEditData->scrollRow * pSLEditData->charHeight;
	xs = edtGetOutWidth(hWnd);

	ln = pSLEditData->dataEnd;

	edittext = doCharShape_UC16(pSLEditData->buffer, ln, &ln, &attrib);

	pTxt = edittext;
	pEnd = pTxt + pSLEditData->dataEnd;
	tot = 0;
	SetTextColor(hdc, fgcol);
	SetBkColor(hdc, bkcol);
	SetBkMode(hdc, OPAQUE);
	szy = neGetTextHeight(hWnd, hdc);
	cx = rc.left;

	done = 0;

	while ((pTxt < pEnd) && !done) {
		int count;
		int isEditRow = 0;

		count = pSLEditData->dataEnd - tot;

		if (count > 0) {
			int n = count;
			int nl = 0;
			EDITCHAR *vtxt = NULL;
			int *v2l = NULL;
			char *direction = NULL;
			int deltay = 0, deltachr = 0;

			attrib &= ~TEXTIP_RTOL;

			if (dwStyle & ES_MULTILINE) {
				for (n = 0; tot + n < ln; n++)
					if (pTxt[n] == '\n') {
						deltachr = 1;
						break;
					}

				if (!(dwStyle & ES_AUTOHSCROLL)) {
					int newn = n;
					int wx = neGetTextWith(hWnd, hdc, pSLEditData, pTxt, newn);
					while ((wx > xs) && (newn > 1)) {
						// note that the macro isspace cause segfault on che > 255
						while (newn > 1 && (pTxt[--newn] > ' '));
						wx = neGetTextWith(hWnd, hdc, pSLEditData, pTxt, newn);
					}
					if ((newn < n) && (pTxt[newn] <= ' '))
						n = newn, deltachr = 1;
				}

				if (n < count)
					deltay = szy, nl = 1;
				count = n;
			}

#if MW_FEATURE_INTL
			if (attrib & TEXTIP_EXTENDED) {
				v2l = (int *) malloc(sizeof(int) * (1 + n));
				direction = (char *) malloc(sizeof(char) * (1 + n));
				vtxt = doCharBidi_UC16(pTxt, n, v2l, direction, &attrib);
				if ((vtxt != NULL) && (cx == rc.left) && (attrib & TEXTIP_RTOL))
					cx = rc.left + xs - neGetTextWith(hWnd, hdc, pSLEditData, vtxt, n);
			}
#endif
			isEditRow = ((tot <= pSLEditData->editPos) && (pSLEditData->editPos <= tot + count));
			//  If this row is the one with editPos, set the index of newline
			if (isEditRow) {
				pSLEditData->epFirstIdx = tot;
				pSLEditData->epLineCount = count;
				pSLEditData->epLineOX = cx;
				pSLEditData->epLineAlign = (attrib & TEXTIP_RTOL) ? 1 : 0;
			}

			if ((action & NEDRAW_ENTIRE) || (isEditRow && (action & NEDRAW_ROW))) {
				if (dwStyle & ES_PASSWORD)
					neTextOutPwd(hdc, cx, rc.top + cy, pSLEditData->passwdChar, count);
				else {
					EDITCHAR *drawtxt = (vtxt != NULL) ? vtxt : pTxt;

					//  Verify if text should be displayed reversed or normal.
					if ((pSLEditData->selStart >= pSLEditData->selEnd)
					    || (tot >= pSLEditData->selEnd)
					    || (tot + count < pSLEditData->selStart)
					    || ((tot >= pSLEditData->selStart)
						&& (tot + count < pSLEditData->selEnd))) {
						if (((tot >= pSLEditData->selStart) && (tot + count < pSLEditData-> selEnd))) {
							SetTextColor(hdc, bkcol);
							SetBkColor(hdc, RGB(0, 0, 255));
						} else {
							SetTextColor(hdc, fgcol);
							SetBkColor(hdc, bkcol);
						}
						neTextOut(hdc, cx, rc.top + cy, drawtxt, count);
					} else {
						// Text that is mixed sel and nonsel is displayed char by char
						int idx;
						int ox = 0;

						for (idx = 0; idx < count;
						     idx++) {
							int ridx = (v2l != NULL) ?  v2l[idx] : idx;
							if ((tot + ridx >= pSLEditData-> selStart)
							    && (tot + ridx < pSLEditData-> selEnd)){
								SetTextColor(hdc, bkcol);
								SetBkColor (hdc, RGB (0, 0, 255));
							} else {
								SetTextColor(hdc, fgcol);
								SetBkColor (hdc, bkcol);
							}

							neTextOut(hdc, cx + ox, rc.top + cy, drawtxt + idx, 1);
							ox += neGetTextWith(hWnd, hdc, pSLEditData, drawtxt + idx, 1);
						}
					}
				}
			}

			if ((action & NEDRAW_CALC_CURSOR)) {
				if (isEditRow) {
					int x;
					int idx = pSLEditData->editPos - tot;
					int nc = idx;
					DPRINTF("***IDX=%d, vidx=%d, n=%d, dir=%d, chr=%04X\n", idx,
							(v2l != NULL) ? v2l[idx] : idx, n,
							(direction != NULL) ? direction[idx] : 0, pSLEditData->buffer[tot + idx]);
					if (vtxt) {
						// for RTOL characters cursor will be displayed at the right.
						if (idx < n)
							nc = v2l[idx] + ((direction [idx] & 1) ?  1 : 0);
						else
							nc = (((idx > 0) &&
								  (direction [idx - 1] & 1))? 
								  v2l[idx - 1] : idx);

						if (nc <= n)
							x = neGetTextWith(hWnd, hdc, pSLEditData, vtxt, nc);
					} else
						x = neGetTextWith(hWnd, hdc, pSLEditData, pTxt, idx);

					if (nc <= n) {
						pSLEditData->epX = cx + x + pSLEditData->scrollX;
						pSLEditData->epY = cy;

						// If we're called only for this, set done.
						if (action == NEDRAW_CALC_CURSOR)
							done = 1;

						action &= ~NEDRAW_CALC_CURSOR;
					}
				}
			} else if ((action & NEDRAW_CALC_EDITPOS)) {
				if (cy / pSLEditData->charHeight >=
				    pSLEditData->epY /
				    pSLEditData->charHeight) {
					int nc, idx, x, dx, bdx = 100000, bi = n, bx = -1;
					for (idx = 0; idx <= n; idx++) {
						if (vtxt) {
							if (idx < n)
								nc = v2l[idx] + ((direction[idx] & 1)?  1 : 0);
							else
								nc = (((idx > 0) &&
								      (direction [idx - 1] & 1))?
								      v2l[idx - 1] : idx);
							x = neGetTextWith(hWnd, hdc, pSLEditData, vtxt, nc);
						} else
							x = neGetTextWith(hWnd, hdc, pSLEditData, pTxt, idx);

						dx = cx + x - pSLEditData->epX + pSLEditData->scrollX;
						if ((dx >= -2) && ((dx <= bdx)))
							bdx = dx, bi = idx, bx = x;
					}
					if (bx < 0)
						bx = x;
					pSLEditData->editPos = tot + bi;
					pSLEditData->epX = cx + bx - pSLEditData->leftMargin + pSLEditData->scrollX;
					pSLEditData->epY = cy;
					if (action == NEDRAW_CALC_EDITPOS)
						done = 1;
					action &= ~NEDRAW_CALC_EDITPOS;
				}
			}

			if (vtxt) {
				free(vtxt);
				free(direction);
				free(v2l);
				vtxt = NULL;
			}
			// Maybe obsolete here: does nl could be zero here ??
			if (!nl)
				cx += neGetTextWith(hWnd, hdc, pSLEditData, pTxt, count);
			else
				cx = rc.left;

			pTxt += count + deltachr;
			tot += count + deltachr;
			cy += deltay;
			if ((rc.top + cy >= rc.bottom) &&
			    !(action & (NEDRAW_CALC_CURSOR | NEDRAW_CALC_EDITPOS)))
				break;
		}
	}

	//  If it's finisced without calculating, put to end
	if ((action & NEDRAW_CALC_CURSOR)) {
		pSLEditData->epX = cx + pSLEditData->scrollX;
		pSLEditData->epY = cy;
	}
	//  If called for editpos and not found a pos, set to zero      
	if ((action & NEDRAW_CALC_EDITPOS)) {
		pSLEditData->epX = cx + pSLEditData->scrollX;
		pSLEditData->epY = cy;
		pSLEditData->editPos = pSLEditData->dataEnd;
	}

	free(edittext);

	if (bRelDC)
		ReleaseDC(hWnd, hdc);

	if ((action & (NEDRAW_ENTIRE | NEDRAW_ROW)) && (GetFocus() == hWnd))
		ShowCaret(hWnd);
}


/*
 *  neNCPaint
 */
static void
neNCPaint(HWND hWnd, WPARAM wParam)
{
	HDC hdc;
	RECT rc;
	DWORD dwStyle = hWnd->style;

	hdc = wParam ? (HDC) wParam : GetWindowDC(hWnd);
	GetClientRect(hWnd, &rc);
	if (dwStyle & WS_DISABLED)
		FastFillRect(hdc, &rc, GetSysColor(COLOR_INACTIVECAPTIONTEXT));
	else
		FastFillRect(hdc, &rc, GetSysColor(COLOR_INFOBK));

	GetWindowRect(hWnd, &rc);

	if (dwStyle & WS_BORDER)
		Draw3dInset(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);

	if (!wParam)
		ReleaseDC(hWnd, hdc);
}



/*
 *  PAINT the EDIT CLIENT AREA
 */
static void
nePaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	RECT rect, rc;
	PAINTSTRUCT ps;
	DWORD dwStyle = hWnd->style;
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	HDC hdc;

	hdc = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rect);

	if (dwStyle & WS_DISABLED) {
		rc.left = 0;
		rc.top = 0;
		rc.bottom = rect.bottom;
		rc.right = rect.right;
		FillRect(hdc, &rc, GetStockObject(LTGRAY_BRUSH));
		SetBkColor(hdc, LTGRAY /*COLOR_lightgray */ );
	} else {
		rc.left = 0;
		rc.top = 0;
		rc.bottom = rect.bottom;
		rc.right = rect.right;
		FillRect(hdc, &rc, GetStockObject(WHITE_BRUSH));
		SetBkColor(hdc, WHITE /*COLOR_lightwhite */ );
	}

	SetTextColor(hdc, BLACK /*COLOR_black */ );
	SelectObject(hdc, pSLEditData->hFont);


	neDrawAllText(hWnd, hdc, pSLEditData, NEDRAW_ENTIRE);

	EndPaint(hWnd, &ps);
}




/*
 *  Invalidate client area
 */
static void
neInvalidateClient(HWND hWnd)
{
	RECT InvRect;
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

	InvRect.left = pSLEditData->leftMargin;
	InvRect.top = pSLEditData->topMargin;
	InvRect.right = hWnd->clirect.right - hWnd->clirect.left;
	InvRect.bottom = hWnd->clirect.bottom - hWnd->clirect.top;
	InvalidateRect(hWnd, &InvRect, FALSE);
}


/*
 *  Set the focus, create caret
 */
static void
neSetFocus(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

	if ((hWnd->userdata & EST_FOCUSED) != 0)
		return;

	hWnd->userdata |= EST_FOCUSED;

	pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

	if ((hWnd->userdata & EST_REPLACE))
		CreateCaret(hWnd, NULL, mwDefEditCaretSizeIns, pSLEditData->charHeight);
	else
		CreateCaret(hWnd, NULL, mwDefEditCaretSize, pSLEditData->charHeight);

	neUpdateCaretPos(hWnd);
	ShowCaret(hWnd);

	SendMessage(GetParent(hWnd), WM_COMMAND,
		    (WPARAM) MAKELONG(hWnd->id, EN_SETFOCUS), (LPARAM) hWnd);
}

/*
 * Kill focus
 */
static void
neKillFocus(HWND hWnd)
{
	hWnd->userdata &= ~EST_FOCUSED;
	HideCaret(hWnd);
	DestroyCaret();
	SendMessage(GetParent(hWnd), WM_COMMAND,
		    (WPARAM) MAKELONG(hWnd->id, EN_KILLFOCUS), (LPARAM) hWnd);
}

/*
 *  Move selection with new cursor position
 */
static BOOL
neMoveSelection(PSLEDITDATA pSLEditData)
{
	int s = pSLEditData->selStart;
	int e = pSLEditData->selEnd;

	if (pSLEditData->editPos < pSLEditData->selCenter) {
		pSLEditData->selStart = pSLEditData->editPos;
		pSLEditData->selEnd = pSLEditData->selCenter;
	} else {
		pSLEditData->selEnd = pSLEditData->editPos;
		pSLEditData->selStart = pSLEditData->selCenter;
	}

	return ((s != pSLEditData->selStart) || (e != pSLEditData->selEnd));
}


/*
 *  Handle mouse down
 */
static void
neMouseLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	DWORD dwStyle = hWnd->style;

	pSLEditData->epX =
		LOWORD(lParam) - pSLEditData->leftMargin +
		pSLEditData->scrollX;
	pSLEditData->epY =
		HIWORD(lParam) - pSLEditData->topMargin +
		pSLEditData->scrollRow * pSLEditData->charHeight;
	if (!(dwStyle & ES_MULTILINE))
		pSLEditData->epY = 0;
	//i = neIndexFromPos ( hWnd, &pt );
	neDrawAllText(hWnd, NULL, pSLEditData, NEDRAW_CALC_EDITPOS);

	//  If a selection was present, remove and redraw
	if (pSLEditData->selStart < pSLEditData->selEnd)
		neInvalidateClient(hWnd);

	pSLEditData->selStart = pSLEditData->editPos;
	pSLEditData->selCenter = pSLEditData->editPos;
	pSLEditData->selEnd = pSLEditData->editPos;
	pSLEditData->caretX = pSLEditData->epX - pSLEditData->scrollX + pSLEditData->leftMargin;
	if ((dwStyle & ES_MULTILINE))
		pSLEditData->caretRow = pSLEditData->epY / pSLEditData->charHeight - pSLEditData->scrollRow;
	neUpdateCaretPos(hWnd);
	SetCapture(hWnd);
}

/*
 *  Handle mouse move
 */
static void
neMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	POINT pt;
	int i;

	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);

	if (GetCapture() != hWnd)
		return;

	ScreenToClient(hWnd, &pt);

	do {
		if (pt.x < pSLEditData->leftMargin) {
			if (!(hWnd->userdata & EST_SELSCROLLLEFT)) {
				hWnd->userdata |= EST_SELSCROLLLEFT;
				SetTimer(hWnd, IDTM_SELSCROLLLEFT, TM_SELSCROLL, NULL);
				pt.x = pSLEditData->leftMargin;
				break;
			}
			return;
		}

		if (pt.x > pSLEditData->leftMargin + edtGetOutWidth(hWnd)) {
			if (!(hWnd->userdata & EST_SELSCROLLRIGHT)) {
				hWnd->userdata |= EST_SELSCROLLRIGHT;
				SetTimer(hWnd, IDTM_SELSCROLLRIGHT, TM_SELSCROLL, NULL);
				pt.x = pSLEditData->leftMargin + edtGetOutWidth(hWnd);
				break;
			}
			return;
		}

		if ((hWnd->userdata & EST_SELSCROLLLEFT))
			KillTimer(hWnd, IDTM_SELSCROLLLEFT);
		if ((hWnd->userdata & EST_SELSCROLLRIGHT))
			KillTimer(hWnd, IDTM_SELSCROLLRIGHT);
		hWnd->userdata &= ~(EST_SELSCROLLLEFT | EST_SELSCROLLRIGHT);
	}
	while (0);

	i = neIndexFromPos(hWnd, &pt);

	pSLEditData->editPos = i;
	neMoveSelection(pSLEditData);

	pSLEditData->caretX = pt.x;
	neUpdateCaretPos(hWnd);
	neDrawAllText(hWnd, NULL, pSLEditData, NEDRAW_ENTIRE);
}

/*
 *  Handle mouse LBUTTUP
 */
static void
neMouseLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (GetCapture() != hWnd)
		return;
	ReleaseCapture();
	if ((hWnd->userdata & EST_SELSCROLLLEFT))
		KillTimer(hWnd, IDTM_SELSCROLLLEFT);
	if ((hWnd->userdata & EST_SELSCROLLRIGHT))
		KillTimer(hWnd, IDTM_SELSCROLLRIGHT);
}


/*
 *  Handle mouse DOUBLE CLICK
 */
static void
neMouseDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);

	pSLEditData->selStart = 0;
	pSLEditData->selEnd = pSLEditData->dataEnd;
	neInvalidateClient(hWnd);
}


/*
 *  Hadle WM_TIMER message for selection scrolling
 */
static void
neTimerMessage(HWND hWnd, WPARAM wParam)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	int lastpos = pSLEditData->editPos;

	if (wParam == IDTM_SELSCROLLLEFT) {
		if (pSLEditData->editPos > 0)
			pSLEditData->editPos--;
	} else if (wParam == IDTM_SELSCROLLRIGHT) {
		if (pSLEditData->editPos < pSLEditData->dataEnd)
			pSLEditData->editPos++;
	}

	if (lastpos != pSLEditData->editPos) {
		neMoveSelection(pSLEditData);
		if (neRecalcScrollPos(hWnd, NULL, pSLEditData, FALSE))
			neInvalidateClient(hWnd);
	}
}

/*
 *  Delete the text in the selection
 */
static BOOL
neCutSelectedText(HWND hWnd, BOOL bCopyToClipb)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	int count;

	if (bCopyToClipb)
		neCopyToCliboard(hWnd, pSLEditData);

	if (pSLEditData->selStart < pSLEditData->selEnd) {
		if (pSLEditData->selStart < 0)
			pSLEditData->selStart = 0;
		if (pSLEditData->selEnd > pSLEditData->dataEnd)
			pSLEditData->selEnd = pSLEditData->dataEnd;
		count = pSLEditData->selEnd - pSLEditData->selStart;
		memmove(pSLEditData->buffer + pSLEditData->selStart,
			pSLEditData->buffer + pSLEditData->selEnd,
			pSLEditData->dataEnd - pSLEditData->selEnd);
		pSLEditData->dataEnd -= count;
		pSLEditData->editPos = pSLEditData->selStart;
		if (pSLEditData->editPos > pSLEditData->dataEnd)
			pSLEditData->editPos = pSLEditData->dataEnd;

		neRecalcScrollPos(hWnd, NULL, pSLEditData, FALSE);
		neInvalidateClient(hWnd);
		pSLEditData->selStart = pSLEditData->selEnd = 0;
		neCheckBufferSize(hWnd, pSLEditData);
		return TRUE;
	}

	return FALSE;
}


/*
 *  A Control character has been pressed
 */
static int
neKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	int lastPos = pSLEditData->editPos;
	RECT InvRect;
	BOOL bRedraw = FALSE;
	BOOL onWord = FALSE;
	DWORD dwStyle = hWnd->style;

	//DPRINTF( "KEYDOWN: %08X %08X\n", (int)wParam, (int)lParam );

	//  If key isn't a control key, exit
	if (!(lParam & (1 << 24)) && (wParam != VK_BACK))
		return 0;

	if (wParam == VK_TAB) {
		PostMessage(GetParent(hWnd), WM_KEYDOWN, wParam, lParam);
		return 0;
	}

	switch (wParam) {
	case VK_LEFT:
		if (pSLEditData->editPos > 0)
			pSLEditData->editPos--;
		if ((hWnd->userdata & EST_CTRL) && (pSLEditData->editPos < pSLEditData->dataEnd)) {
			onWord = neIsWord(pSLEditData->buffer[pSLEditData->editPos]);
			while ((pSLEditData->editPos > 0) && (neIsWord(pSLEditData->buffer[pSLEditData->editPos]) == onWord))
				pSLEditData->editPos--;
		}

		if ((hWnd->userdata & EST_SHIFT))
			bRedraw = neMoveSelection(pSLEditData);
		else {
			bRedraw = (pSLEditData->selStart < pSLEditData->selEnd);
			pSLEditData->selStart = pSLEditData->selEnd = pSLEditData->editPos;
		}
		break;

	case VK_RIGHT:
		if (pSLEditData->editPos < pSLEditData->dataEnd)
			pSLEditData->editPos++;
		if ((hWnd->userdata & EST_CTRL) && (pSLEditData->editPos < pSLEditData->dataEnd)) {
			onWord = neIsWord(pSLEditData->buffer[pSLEditData->editPos]);
			while ((pSLEditData->editPos < pSLEditData->dataEnd)
			       && (neIsWord (pSLEditData->buffer[pSLEditData->editPos]) == onWord))
				pSLEditData->editPos++;
		}

		if ((hWnd->userdata & EST_SHIFT))
			bRedraw = neMoveSelection(pSLEditData);
		else {
			bRedraw = (pSLEditData->selStart < pSLEditData->selEnd);
			pSLEditData->selStart = pSLEditData->selEnd =
				pSLEditData->editPos;
		}
		break;

	case VK_UP:
	case VK_DOWN:
		if (!(dwStyle & ES_MULTILINE))
			break;
		pSLEditData->epX = pSLEditData->caretX + pSLEditData->scrollX;
		pSLEditData->epY = (pSLEditData->caretRow +
			 pSLEditData->scrollRow) * pSLEditData->charHeight;
		if (wParam == VK_UP) {
			pSLEditData->epY -= pSLEditData->charHeight;
			if (pSLEditData->epY < 0) {
				pSLEditData->epY = 0;
				if (pSLEditData->scrollRow > 0)
					pSLEditData->scrollRow--;
			}
		} else {
			pSLEditData->epY += pSLEditData->charHeight;
		}
		neDrawAllText(hWnd, NULL, pSLEditData, NEDRAW_CALC_EDITPOS);
		break;

	case VK_HOME:
		pSLEditData->editPos = 0;
		if ((hWnd->userdata & EST_SHIFT))
			bRedraw = neMoveSelection(pSLEditData);
		else {
			bRedraw = (pSLEditData->selStart < pSLEditData->selEnd);
			pSLEditData->selStart = pSLEditData->selEnd =
				pSLEditData->editPos;
		}
		break;

	case VK_END:
		pSLEditData->editPos = pSLEditData->dataEnd;
		if ((hWnd->userdata & EST_SHIFT))
			bRedraw = neMoveSelection(pSLEditData);
		else {
			bRedraw = (pSLEditData->selStart < pSLEditData->selEnd);
			pSLEditData->selStart = pSLEditData->selEnd = pSLEditData->editPos;
		}
		break;

	case VK_BACK:
		if (neCutSelectedText(hWnd, FALSE));
		else if (pSLEditData->editPos > 0) {
			pSLEditData->editPos--;
			memmove(pSLEditData->buffer + pSLEditData->editPos,
				pSLEditData->buffer + pSLEditData->editPos + 1,
				(pSLEditData->dataEnd - pSLEditData->editPos) * SZEDITCHAR);
			pSLEditData->dataEnd--;
			neCheckBufferSize(hWnd, pSLEditData);
			bRedraw = TRUE;
		}
		SendMessage(GetParent(hWnd), WM_COMMAND,
			    (WPARAM) MAKELONG(hWnd->id, EN_CHANGE), (LPARAM) hWnd);
		break;

	case VK_DELETE:
		if (neCutSelectedText
		    (hWnd, ((hWnd->userdata & EST_SHIFT) != 0)));
		else if (pSLEditData->editPos < pSLEditData->dataEnd) {
			memmove(pSLEditData->buffer + pSLEditData->editPos,
				pSLEditData->buffer + pSLEditData->editPos + 1,
				(pSLEditData->dataEnd - pSLEditData->editPos) * SZEDITCHAR);
			pSLEditData->dataEnd--;
			neCheckBufferSize(hWnd, pSLEditData);
			bRedraw = TRUE;
		}
		SendMessage(GetParent(hWnd), WM_COMMAND,
			    (WPARAM) MAKELONG(hWnd->id, EN_CHANGE), (LPARAM) hWnd);
		break;

	case VK_INSERT:
		if ((hWnd->userdata & EST_SHIFT))
			neCharPressed(hWnd, -1, -1);
		else if ((hWnd->userdata & EST_CTRL))
			neCopyToCliboard(hWnd, pSLEditData);
		else {
			hWnd->userdata ^= EST_REPLACE;
			if (GetFocus() == hWnd) {
				DestroyCaret();
				if ((hWnd->userdata & EST_REPLACE))
					CreateCaret(hWnd, NULL, 3, pSLEditData->charHeight);
				else
					CreateCaret(hWnd, NULL, 1, pSLEditData->charHeight);
				neUpdateCaretPos(hWnd);
				ShowCaret(hWnd);
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
		if (GetCapture() != hWnd) {
			hWnd->userdata |= EST_SHIFT;
			pSLEditData->selCenter = pSLEditData->editPos;
		}
		break;
	}

	if ((lastPos != pSLEditData->editPos) || bRedraw) {
		if (neRecalcScrollPos(hWnd, NULL, pSLEditData, FALSE) || bRedraw) {
			InvRect.left = pSLEditData->leftMargin;
			InvRect.top = pSLEditData->topMargin;
			InvRect.right = hWnd->clirect.right - hWnd->clirect.left;
			InvRect.bottom = hWnd->clirect.bottom - hWnd->clirect.top;
			InvalidateRect(hWnd, &InvRect, FALSE);
		}
		neUpdateCaretPos(hWnd);
	}

	return 0;
}


/*
 *  Key released
 */
static int
neKeyUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	//  If key isn't a control key, exit
	if (!(lParam & (1 << 24)) && (wParam != VK_BACK))
		return 0;

	switch (wParam) {
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
static int
neCharPressed(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	EDITCHAR locBuffer[2];
	EDITCHAR *charBuffer = locBuffer;
	int i, chars, inserting;
	PSLEDITDATA pSLEditData = (PSLEDITDATA) (hWnd->userdata2);
	DWORD dwStyle = hWnd->style;
	BOOL isPasting = (((LPARAM) wParam == -1) && lParam == -1);


	if (dwStyle & ES_READONLY)
		return 0;

	//DPRINTF ( "char: %08X - %08X\n", (int)wParam, (int)lParam );


	if ((wParam == 0xD) && (hWnd->userdata & EST_CTRL))
		DPRINTF("CTRL+ENTER!!!!!\n");

	//   check if called for pasting clipboard
	if (isPasting) {
		if (neClipboard == NULL)
			return 0;

		charBuffer = neClipboard;
		chars = neClipboardSize;
	} else {
		chars = 1;
		charBuffer[0] = (EDITCHAR) LOWORD(wParam);
		switch (LOWORD(wParam)) {
		case 0x00:	/* NULL */
		case 0x07:	/* BEL */
		case 0x08:	/* BS */
		case 0x09:	/* HT */
		case 0x0A:	/* LF */
		case 0x0B:	/* VT */
		case 0x0C:	/* FF */
		case 0x1B:	/* Escape */
			return 0;

		case 0x0D:	/* CR */
			charBuffer[0] = '\n';
			break;
		}

		if ((dwStyle & ES_NUMBER) && !isdigit(charBuffer[0]))
			return 0;
	}

	//  If there is a selection, remove it.
	neCutSelectedText(hWnd, FALSE);

	if ((hWnd->userdata & EST_REPLACE) && !isPasting) {
		if (pSLEditData->dataEnd == pSLEditData->editPos)
			inserting = chars;
		else
			inserting = 0;
	} else
		inserting = chars;

	/* check space */
	if ((pSLEditData->hardLimit >= 0) &&
	    (pSLEditData->dataEnd + inserting > pSLEditData->hardLimit)) {
		SendMessage(GetParent(hWnd), WM_COMMAND,
			    (WPARAM) MAKELONG(hWnd->id, EN_MAXTEXT), (LPARAM) hWnd);
		return 0;
	}
	//   Increase dataEnd and check buffer size
	if (!neIncDataEnd(pSLEditData, inserting)) {
		SendMessage(GetParent(hWnd), WM_COMMAND,
			    (WPARAM) MAKELONG(hWnd->id, EN_MAXTEXT), (LPARAM) hWnd);
		return 0;
	}

	if (inserting == -1) {
		for (i = pSLEditData->editPos; i < pSLEditData->dataEnd - 1;
		     i++)
			pSLEditData->buffer[i] = pSLEditData->buffer[i + 1];
	} else if (inserting > 0) {
		for (i = pSLEditData->dataEnd - 1;
		     i > pSLEditData->editPos - 1 && i - inserting>=0; i--)
			pSLEditData->buffer[i] =
				pSLEditData->buffer[i - inserting];
	}

	for (i = 0; i < chars; i++) {
		if (dwStyle & ES_UPPERCASE)
			pSLEditData->buffer[pSLEditData->editPos + i] = toupper(charBuffer[i]);
		else if (dwStyle & ES_LOWERCASE)
			pSLEditData->buffer[pSLEditData->editPos + i] = tolower(charBuffer[i]);
		else
			pSLEditData->buffer[pSLEditData->editPos + i] = charBuffer[i];
	}

	pSLEditData->editPos += chars;
	pSLEditData->selCenter = pSLEditData->editPos;

	if (neRecalcScrollPos(hWnd, NULL, pSLEditData, TRUE))
		neInvalidateClient(hWnd);
	else
		neDrawAllText(hWnd, NULL, pSLEditData, NEDRAW_ROW);

	neUpdateCaretPos(hWnd);

	SendMessage(GetParent(hWnd), WM_COMMAND,
		    (WPARAM) MAKELONG(hWnd->id, EN_CHANGE), (LPARAM) hWnd);
	return 0;
}


/*
 * WINDOW PROCEDURE
 */
LRESULT CALLBACK
SLNewEditCtrlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND pCtrl;
	DWORD dwStyle;
	PSLEDITDATA pSLEditData;

	pCtrl = hWnd;
	dwStyle = pCtrl->style;

	switch (message) {
	case WM_CREATE:
		neCreate(hWnd);
		break;

	case WM_DESTROY:
		neDestroy(hWnd);
		break;

	case WM_SETFONT:
		neSetFont(hWnd, (HFONT) wParam, (BOOL) LOWORD(lParam));
		break;

	case WM_GETFONT:
		pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
		return (LRESULT) pSLEditData->hFont;

	case WM_KILLFOCUS:
		neKillFocus(hWnd);
		break;

	case WM_SETFOCUS:
		neSetFocus(hWnd, wParam, lParam);
		break;

	case WM_ENABLE:
		if ((!(dwStyle & WS_DISABLED) && !wParam) ||
		    ((dwStyle & WS_DISABLED) && wParam)) {
			if (wParam)
				pCtrl->style &= ~WS_DISABLED;
			else
				pCtrl->style |= WS_DISABLED;

			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;

	case WM_NCCALCSIZE:
		{
			LPNCCALCSIZE_PARAMS lpnc;

			/* calculate client rect from passed window rect in rgrc[0] */
			lpnc = (LPNCCALCSIZE_PARAMS) lParam;
			if (GetWindowLong(hWnd, GWL_STYLE) & WS_BORDER)
				InflateRect(&lpnc->rgrc[0], -2, -2);
		}
		break;

	case WM_NCPAINT:
		neNCPaint(hWnd, wParam);
		break;

	case WM_PAINT:
		nePaint(hWnd, wParam, lParam);
		break;

	case WM_KEYDOWN:
		return neKeyDown(hWnd, wParam, lParam);

	case WM_KEYUP:
		return neKeyUp(hWnd, wParam, lParam);

	case WM_CHAR:
		return neCharPressed(hWnd, wParam, lParam);

	case WM_GETTEXTLENGTH:
		pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
		return pSLEditData->dataEnd;

	case WM_GETTEXT:
		{
			char *buffer = (char *) lParam;
			int len;

			pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
			len = min((int) wParam, pSLEditData->dataEnd);
			//memcpy_fromedit (buffer, pSLEditData->buffer, len);
			return GdConvertEncoding(pSLEditData->buffer, MWTF_UC16, len, buffer, mwTextCoding)?
				strlen(buffer): 0;
		}

	case WM_SETTEXT:
		neSetText(hWnd, (const char *) lParam);
		break;

	case WM_LBUTTONDBLCLK:
		neMouseDblClk(hWnd, wParam, lParam);
		break;

	case WM_LBUTTONDOWN:
		neMouseLButtonDown(hWnd, wParam, lParam);
		break;

	case WM_LBUTTONUP:
	case WM_NCLBUTTONUP:
		neMouseLButtonUp(hWnd, wParam, lParam);
		break;

	case WM_NCMOUSEMOVE:
		neMouseMove(hWnd, wParam, lParam);
		break;

	case WM_TIMER:
		neTimerMessage(hWnd, wParam);
		break;

	case WM_GETDLGCODE:
		if (!(pCtrl->style & ES_WANTRETURN))
			return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;
		else
			return DLGC_WANTALLKEYS | DLGC_HASSETSEL;

	case EM_SETREADONLY:
		if (wParam)
			pCtrl->style /*dwStyle */  |= ES_READONLY;
		else
			pCtrl->style /*dwStyle */  &= ~ES_READONLY;
		return 0;

	case EM_SETPASSWORDCHAR:
		pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
		if (pSLEditData->passwdChar != (int) wParam) {
			if (dwStyle & ES_PASSWORD) {
				pSLEditData->passwdChar = (int) wParam;
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		return 0;

	case EM_GETPASSWORDCHAR:
		pSLEditData = (PSLEDITDATA) (pCtrl->userdata2);
		return pSLEditData->passwdChar;

	case EM_LIMITTEXT:
		{
			int newLimit = (int) wParam;

			if (newLimit >= 0) {
				pSLEditData =
					(PSLEDITDATA) (pCtrl->userdata2);
				if (pSLEditData->bufferLen < newLimit)
					pSLEditData->hardLimit = -1;
				else
					pSLEditData->hardLimit = newLimit;
			}
		}
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}




int
MwRegisterEditControl(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc = (WNDPROC) SLNewEditCtrlProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = 0;		/*LoadCursor(NULL, IDC_ARROW); */
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "EDIT";

	return RegisterClass(&wc);
}
