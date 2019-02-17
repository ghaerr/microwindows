/*
 * Messagebox module for Microwindows
 *
 * Ported from ReactOS to Microwindows by
 * DI (FH) Ludwig Ertl / CSP GmbH 2010
 *
 * Code is based on:
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/messagebox.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Thomas Weidenmueller (w3seek@users.sourceforge.net)
 */
#include <stdlib.h>
#include <string.h>
#include "windows.h"

#define USEICON		0			/* =1 for icons in messagebox*/
/*
 * Here are the identifiers for the stringtable, you may have to
 * move them to another module (i.e. resource.h for user32).
 */
#define IDS_ERROR	(2)
#define IDS_OK	(800)
#define IDS_CANCEL	(801)
#define IDS_ABORT	(802)
#define IDS_RETRY	(803)
#define IDS_IGNORE	(804)
#define IDS_YES	(805)
#define IDS_NO	(806)
#define IDS_HELP	(808)
#define IDS_TRYAGAIN	(809)
#define IDS_CONTINUE	(810)

#define MSGBOX_IDICON   (1088)
#define MSGBOX_IDTEXT   (100)

#define IDI_HAND          MAKEINTRESOURCE(32513)
#define IDI_QUESTION      MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION   MAKEINTRESOURCE(32515)
#define IDI_ASTERISK      MAKEINTRESOURCE(32516)
#define IDI_WINLOGO       MAKEINTRESOURCE(32517)


/*
 * Internal stuff
 */
#define LANG_NEUTRAL                     0x00

#define MSGBOXEX_SPACING    (16)
#define MSGBOXEX_BUTTONSPACING  (6)
#define MSGBOXEX_MARGIN (12)
#define MSGBOXEX_MAXBTNSTR  (32)
#define MSGBOXEX_MAXBTNS    (4)

typedef struct _MSGBOXINFO {
  HICON Icon;
  HFONT Font;
  DWORD ContextHelpId;
  MSGBOXCALLBACK Callback;
  DWORD Style;
  int DefBtn;
  int nButtons;
  LONG *Btns;
  UINT Timeout;
} MSGBOXINFO, *PMSGBOXINFO;

#define BTN_CX (75)
#define BTN_CY (23)


/* This contains all the static strings used by the MessageBox.
 * They are used in case you don't have a resource with the real string.
 */
static char *
GetMsgboxString(UINT uid)
{
	switch (uid) {
	case IDS_OK:	return "Ok";
	case IDS_ERROR:	return "&Error";
	case IDS_CANCEL:return "&Cancel";
	case IDS_YES:	return "&Yes";
	case IDS_NO:	return "&No";
	case IDS_TRYAGAIN: return "Try &again";
	case IDS_CONTINUE: return "C&ontinue";
	case IDS_ABORT: return "A&bort";
	case IDS_RETRY: return "&Retry";
	case IDS_IGNORE: return "&Ignore";
	case IDS_HELP: return "&Help";
	}
	return "";
}

/*
 * Internal LoadString with fallback to hardcoded strings
 */
static int
loadstring(HINSTANCE hInstance, UINT uid, LPTSTR lpBuffer, int nMaxBuff)
{
	char *pszRes;

	if (hInstance) return LoadString (hInstance, uid, lpBuffer, nMaxBuff);
	pszRes = GetMsgboxString (uid);
	strncpy (lpBuffer, pszRes, nMaxBuff);
	return strlen(pszRes);
}

/*
 * Window procedure for the dialog box
 */
static LRESULT CALLBACK
MessageBoxProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  int i;
  PMSGBOXINFO mbi;

  switch(message) {
    case WM_INITDIALOG:
      mbi = (PMSGBOXINFO)lParam;
      if(!GetProp(hwnd, "ROS_MSGBOX"))
      {
        SetProp(hwnd, "ROS_MSGBOX", (HANDLE)lParam);
        if(mbi->Icon)
          SendDlgItemMessage(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM)mbi->Icon, 0);

        /* set control fonts */
        SendDlgItemMessage(hwnd, MSGBOX_IDTEXT, WM_SETFONT, (WPARAM)mbi->Font, 0);
        for(i = 0; i < mbi->nButtons; i++)
          SendDlgItemMessage(hwnd, mbi->Btns[i], WM_SETFONT, (WPARAM)mbi->Font, 0);
#if 0
        switch(mbi->Style & MB_TYPEMASK)
        {
          case MB_ABORTRETRYIGNORE:
          case MB_YESNO:
            RemoveMenu(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_BYCOMMAND);
            break;
        }
#endif
        SetFocus(GetDlgItem(hwnd, mbi->DefBtn));
        if(mbi->Timeout && (mbi->Timeout != (UINT)-1))
          SetTimer(hwnd, 0, mbi->Timeout, NULL);
      }
      return 0;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDOK:
        case IDCANCEL:
        case IDABORT:
        case IDRETRY:
        case IDIGNORE:
        case IDYES:
        case IDNO:
        case IDTRYAGAIN:
        case IDCONTINUE:
          EndDialog(hwnd, wParam);
          return TRUE;
      }
      return 0;
    case WM_CLOSE:
      mbi = (PMSGBOXINFO)GetProp(hwnd, "ROS_MSGBOX");
      if(!mbi)
        return 0;
      switch(mbi->Style & MB_TYPEMASK)
      {
        case MB_ABORTRETRYIGNORE:
        case MB_YESNO:
          return 1;
      }
      EndDialog(hwnd, IDCANCEL);
      return 1;

    case WM_TIMER:
      if(wParam == 0)
      {
        EndDialog(hwnd, 32000);
      }
      return 0;
  }
  return 0;
}

static int
MessageBoxTimeoutIndirect(const MSGBOXPARAMS *lpMsgBoxParams, UINT Timeout)
{
    DLGTEMPLATE *tpl;
    DLGITEMTEMPLATE *itxt;
    //DLGITEMTEMPLATE *iico;
    char capbuf[32];
    HMODULE hUser32 = 0;
    LPVOID buf;
    BYTE *dest;
    LPCSTR caption, text;
    HFONT hFont;
    HICON Icon = (HICON)0;
    HDC hDC;
    int ret, i, btnleft, btntop, lmargin, nButtons = 0;
    LONG Buttons[MSGBOXEX_MAXBTNS];
    char ButtonText[MSGBOXEX_MAXBTNS][MSGBOXEX_MAXBTNSTR];
    DLGITEMTEMPLATE *ibtn[MSGBOXEX_MAXBTNS];
    RECT btnrect, txtrect, rc;
    SIZE btnsize;
    MSGBOXINFO mbi;
    BOOL defbtn = FALSE;
    DWORD units = GetDialogBaseUnits();
	DWORD style, dwExtendedStyle;
	int cdit;

    if(!lpMsgBoxParams->lpszCaption || !*lpMsgBoxParams->lpszCaption)
    {
      loadstring(hUser32, IDS_ERROR, &capbuf[0], 32);
      caption = &capbuf[0];
    }
    else
      caption = (LPCSTR)lpMsgBoxParams->lpszCaption;

    if(!lpMsgBoxParams->lpszText || !*lpMsgBoxParams->lpszText)
      text = "";
    else
      text = (LPCSTR)lpMsgBoxParams->lpszText;

    /* Create selected buttons */
    switch(lpMsgBoxParams->dwStyle & MB_TYPEMASK)
    {
        case MB_OKCANCEL:
            Buttons[0] = IDOK;
            Buttons[1] = IDCANCEL;
            nButtons = 2;
            break;
        case MB_CANCELTRYCONTINUE:
            Buttons[0] = IDCANCEL;
            Buttons[1] = IDTRYAGAIN;
            Buttons[2] = IDCONTINUE;
            nButtons = 3;
            break;
        case MB_ABORTRETRYIGNORE:
            Buttons[0] = IDABORT;
            Buttons[1] = IDRETRY;
            Buttons[2] = IDIGNORE;
            nButtons = 3;
            break;
        case MB_YESNO:
            Buttons[0] = IDYES;
            Buttons[1] = IDNO;
            nButtons = 2;
            break;
        case MB_YESNOCANCEL:
            Buttons[0] = IDYES;
            Buttons[1] = IDNO;
            Buttons[2] = IDCANCEL;
            nButtons = 3;
            break;
        case MB_RETRYCANCEL:
            Buttons[0] = IDRETRY;
            Buttons[1] = IDCANCEL;
            nButtons = 2;
            break;
        case MB_OK:
            /* fall through */
        default:
            Buttons[0] = IDOK;
            nButtons = 1;
            break;
    }
    /* Create Help button */
    if(lpMsgBoxParams->dwStyle & MB_HELP)
      Buttons[nButtons++] = IDHELP;

#if USEICON
    switch(lpMsgBoxParams->dwStyle & MB_ICONMASK)
    {
      case MB_ICONEXCLAMATION:
        Icon = _LoadIcon(0, IDI_EXCLAMATION);
        MessageBeep(MB_ICONEXCLAMATION);
        break;
      case MB_ICONQUESTION:
        Icon = _LoadIcon(0, IDI_QUESTION);
        MessageBeep(MB_ICONQUESTION);
        break;
      case MB_ICONASTERISK:
        Icon = _LoadIcon(0, IDI_ASTERISK);
        MessageBeep(MB_ICONASTERISK);
        break;
      case MB_ICONHAND:
        Icon = _LoadIcon(0, IDI_HAND);
        MessageBeep(MB_ICONHAND);
        break;
      case MB_USERICON:
        Icon = _LoadIcon(lpMsgBoxParams->hInstance, (LPCWSTR)lpMsgBoxParams->lpszIcon);
        MessageBeep(MB_OK);
        break;
      default:
        /* By default, Windows 95/98/NT does not associate an icon to message boxes.
         * So ReactOS should do the same.
         */
        Icon = (HICON)0;
        MessageBeep(MB_OK);
        break;
    }
#endif

    for(i = 0; i < nButtons; i++) {
      switch(Buttons[i])
      {
        case IDOK:
          loadstring(hUser32, IDS_OK, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDCANCEL:
          loadstring(hUser32, IDS_CANCEL, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDYES:
          loadstring(hUser32, IDS_YES, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDNO:
          loadstring(hUser32, IDS_NO, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDTRYAGAIN:
          loadstring(hUser32, IDS_TRYAGAIN, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDCONTINUE:
          loadstring(hUser32, IDS_CONTINUE, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDABORT:
          loadstring(hUser32, IDS_ABORT, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDRETRY:
          loadstring(hUser32, IDS_RETRY, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDIGNORE:
          loadstring(hUser32, IDS_IGNORE, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDHELP:
          loadstring(hUser32, IDS_HELP, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        default:
          ButtonText[i][0] = 0;
          break;
      }
    }

	/* allocate dialog template*/
    if (!(buf = malloc(512)))
      return 0;

    hDC = CreateCompatibleDC(0);
    hFont = GetStockObject(DEFAULT_GUI_FONT);

    tpl = (DLGTEMPLATE *)buf;
    style = WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU | DS_CENTER | DS_MODALFRAME|DS_NOIDLEMSG;
    dwExtendedStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT;
    cdit = nButtons + (Icon != NULL) + 1;
    if(lpMsgBoxParams->dwStyle & MB_TOPMOST)
      dwExtendedStyle |= WS_EX_TOPMOST;
    if(lpMsgBoxParams->dwStyle & MB_RIGHT)
      dwExtendedStyle |= WS_EX_RIGHT;
	dest = resDialogTemplate((BYTE *)tpl, caption, style, dwExtendedStyle, 100, 100, 0, 0, NULL, NULL, cdit);

#if USEICON
    /* Create icon */
    if(Icon)
    {
      iico = (DLGITEMTEMPLATE *)dest;
	  dest = resDialogItemTemplate(dest, WS_CHILD|WS_VISIBLE|SS_ICON, 0, MSGBOX_IDICON,
	  		0, 0, 0, 0, DLGIGTEM_CLASS_STATIC, NULL);
    }
#endif

    /* create static for text */
    itxt = (DLGITEMTEMPLATE *)dest;
    style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX;
    itxt->style |= (lpMsgBoxParams->dwStyle & MB_RIGHT)? SS_RIGHT: SS_LEFT;
	dest = resDialogItemTemplate(dest, style, 0, MSGBOX_IDTEXT, 0, 0, 0, 0, DLGITEM_CLASS_STATIC, text);

    /* create buttons */
    btnsize.cx = BTN_CX;
    btnsize.cy = BTN_CY;
    btnrect.left = btnrect.top = 0;
	btnrect.right = btnrect.bottom = 0;
    for(i = 0; i < nButtons; i++)
    {
      ibtn[i] = (DLGITEMTEMPLATE *)dest;
      ibtn[i]->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
      if(!defbtn && (i == ((lpMsgBoxParams->dwStyle & MB_DEFMASK) >> 8)))
      {
        ibtn[i]->style |= BS_DEFPUSHBUTTON;
        mbi.DefBtn = Buttons[i];
        defbtn = TRUE;
      }
      else
        ibtn[i]->style |= BS_PUSHBUTTON;
	  dest = resDialogItemTemplate(dest, ibtn[i]->style, 0, Buttons[i], 0, 0, 0, 0, DLGITEM_CLASS_BUTTON, ButtonText[i]);
      SelectObject(hDC, hFont);
      DrawText(hDC, ButtonText[i], -1, &btnrect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);
      btnsize.cx = max(btnsize.cx, btnrect.right);
      btnsize.cy = max(btnsize.cy, btnrect.bottom);
    }

    /* make first button the default button if no other is */
    if(!defbtn)
    {
      ibtn[0]->style &= ~BS_PUSHBUTTON;
      ibtn[0]->style |= BS_DEFPUSHBUTTON;
      mbi.DefBtn = Buttons[0];
    }

    /* calculate position and size of controls */
    txtrect.right = GetSystemMetrics(SM_CXSCREEN) / 5 * 4;
#if USEICON
    if(Icon)
      txtrect.right -= GetSystemMetrics(SM_CXICON) + MSGBOXEX_SPACING;
#endif
    txtrect.top = txtrect.left = txtrect.bottom = 0;
    SelectObject(hDC, hFont);
    DrawText(hDC, text, -1, &txtrect, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
    txtrect.right++;

    /* calculate position and size of the icon */
    rc.left = rc.bottom = rc.right = 0;
    btntop = 0;
#if USEICON
    if(iico)
    {
      rc.right = GetSystemMetrics(SM_CXICON);
      rc.bottom = GetSystemMetrics(SM_CYICON);
#ifdef MSGBOX_ICONVCENTER
      rc.top = MSGBOXEX_MARGIN + (max(txtrect.bottom, rc.bottom) / 2) - (GetSystemMetrics(SM_CYICON) / 2);
      rc.top = max(MSGBOXEX_SPACING, rc.top);
#else
      rc.top = MSGBOXEX_MARGIN;
#endif
      btnleft = (nButtons * (btnsize.cx + MSGBOXEX_BUTTONSPACING)) - MSGBOXEX_BUTTONSPACING;
      if(btnleft > txtrect.right + rc.right + MSGBOXEX_SPACING)
      {
#ifdef MSGBOX_TEXTHCENTER
        lmargin = MSGBOXEX_MARGIN + ((btnleft - txtrect.right - rc.right - MSGBOXEX_SPACING) / 2);
#else
        lmargin = MSGBOXEX_MARGIN;
#endif
        btnleft = MSGBOXEX_MARGIN;
      }
      else
      {
        lmargin = MSGBOXEX_MARGIN;
        btnleft = MSGBOXEX_MARGIN + ((txtrect.right + rc.right + MSGBOXEX_SPACING) / 2) - (btnleft / 2);
      }
      rc.left = lmargin;
      iico->x = (rc.left * 4) / LOWORD(units);
      iico->y = (rc.top * 8) / HIWORD(units);
      iico->cx = (rc.right * 4) / LOWORD(units);
      iico->cy = (rc.bottom * 8) / HIWORD(units);
      btntop = rc.top + rc.bottom + MSGBOXEX_SPACING;
      rc.left += rc.right + MSGBOXEX_SPACING;
    }
    else
#endif /* USEICON*/
    {
      btnleft = (nButtons * (btnsize.cx + MSGBOXEX_BUTTONSPACING)) - MSGBOXEX_BUTTONSPACING;
      if(btnleft > txtrect.right)
      {
#ifdef MSGBOX_TEXTHCENTER
        lmargin = MSGBOXEX_MARGIN + ((btnleft - txtrect.right) / 2);
#else
        lmargin = MSGBOXEX_MARGIN;
#endif
        btnleft = MSGBOXEX_MARGIN;
      }
      else
      {
        lmargin = MSGBOXEX_MARGIN;
        btnleft = MSGBOXEX_MARGIN + (txtrect.right / 2) - (btnleft / 2);
      }
      rc.left = lmargin;
    }
    /* calculate position of the text */
    rc.top = MSGBOXEX_MARGIN + (rc.bottom / 2) - (txtrect.bottom / 2);
    rc.top = max(rc.top, MSGBOXEX_MARGIN);

    /* calculate position of the buttons */
    btntop = max(rc.top + txtrect.bottom + MSGBOXEX_SPACING, btntop);
    for(i = 0; i < nButtons; i++)
    {
      ibtn[i]->x = (btnleft * 4) / LOWORD(units);
      ibtn[i]->y = (btntop * 8) / HIWORD(units);
      ibtn[i]->cx = (btnsize.cx * 4) / LOWORD(units);
      ibtn[i]->cy = (btnsize.cy * 8) / HIWORD(units);
      btnleft += btnsize.cx + MSGBOXEX_BUTTONSPACING;
    }
    /* calculate size and position of the messagebox window */
    btnleft = max(btnleft - MSGBOXEX_BUTTONSPACING, rc.left + txtrect.right);
    btnleft += MSGBOXEX_MARGIN;
    btntop +=  btnsize.cy + MSGBOXEX_MARGIN;
    /* set size and position of the message static */
    itxt->x = (rc.left * 4) / LOWORD(units);
    itxt->y = (rc.top * 8) / HIWORD(units);
    itxt->cx = (((btnleft - rc.left - MSGBOXEX_MARGIN) * 4) / LOWORD(units));
    itxt->cy = ((txtrect.bottom * 8) / HIWORD(units));
    /* set size of the window */
    tpl->cx = (btnleft * 4) / LOWORD(units);
    tpl->cy = (btntop * 8) / HIWORD(units);

    /* finally show the messagebox */
    mbi.Icon = Icon;
    mbi.Font = hFont;
    mbi.ContextHelpId = lpMsgBoxParams->dwContextHelpId;
    mbi.Callback = lpMsgBoxParams->lpfnMsgBoxCallback;
    mbi.Style = lpMsgBoxParams->dwStyle;
    mbi.nButtons = nButtons;
    mbi.Btns = &Buttons[0];
    mbi.Timeout = Timeout;

    if(hDC)
      DeleteDC(hDC);
    if(hFont)
      DeleteObject(hFont);

    ret =  DialogBoxIndirectParam(lpMsgBoxParams->hInstance, tpl, lpMsgBoxParams->hwndOwner, MessageBoxProc, (LPARAM)&mbi);

    free (buf);
    return ret;
}

int WINAPI
MessageBoxTimeout(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType,
	WORD wLanguageId, DWORD dwTime)
{
    MSGBOXPARAMS msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = lpText;
    msgbox.lpszCaption = lpCaption;
    msgbox.dwStyle = uType;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = wLanguageId;

    return MessageBoxTimeoutIndirect(&msgbox, (UINT)dwTime);		// FIXME
}

int WINAPI
MessageBoxEx(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId)
{
    MSGBOXPARAMS msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = lpText;
    msgbox.lpszCaption = lpCaption;
    msgbox.dwStyle = uType;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = wLanguageId;

    return MessageBoxIndirect(&msgbox);
}

int WINAPI
MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
    return MessageBoxEx(hWnd, lpText, lpCaption, uType, LANG_NEUTRAL);
}

int WINAPI
MessageBoxIndirect( const MSGBOXPARAMS *lpMsgBoxParams)
{
    return MessageBoxTimeoutIndirect(lpMsgBoxParams, (UINT)-1);
}
