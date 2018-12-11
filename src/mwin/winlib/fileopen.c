/*
 * "Open and Save As Dialog Box" for Microwindows win32 api.
 *
 * 2018, Georg Potthast
 *
 * The save dialog allows to initialize the filename edit control
 * See the mwopenfile.c example for the usage of this control
 * 
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "windows.h"	

#define OFN_MAXBTNSTR  (32)
#define OFN_MAXBTNS    (4)
#define OFN_IDTEXT   (100)
#define DLGTEMPLATE_SIZE 18 /* sizeof(DLGTEMPLATE) */
#define DLGITEMTEMPLATE_SIZE 18  /* sizeof(DLGITEMTEMPLATE) */
#define SAFETY_MARGIN 32 /* Extra number of bytes to allocate just in case */

#define ID_EDIT 5
#define ID_LIST 7
#define ID_OK 8
#define ID_STATICN 9
#define ID_STATICF 10
#define ID_COMBO 11
#define ID_CANCEL 12

struct filters {
  char name[32];
  char suffix[32];
};
struct filters filterpairs[10];

char currentfilter[1024];

char curpath[PATH_MAX] = {"."};

/* Converts ASCIIZ String to WCHAR-String for Resource template */
static int CopyToWchar (PWCHAR lpDest, LPCSTR lpText, int nChars)
{	int i;
	for (i=0; i<nChars; i++)
		lpDest[i] = lpText[i];
	lpDest[i]=0;
	return (i+1)*sizeof(WCHAR);
}

void fill_listbox(HWND hwnd,const char *path)
{
       struct dirent **namelist;
       int n;
       int i=0;
       char pathtmp[PATH_MAX+2] = {' ',' '};

       n = scandir(path, &namelist, NULL, alphasort);
       if (n < 0)
           return;
       else {
           while (i<n) {
	     if (strcmp(namelist[i]->d_name, ".") != 0) {
	       if (namelist[i]->d_type == DT_DIR) {
	        sprintf(pathtmp,"> %s",namelist[i]->d_name);
		SendDlgItemMessage ( hwnd, ID_LIST,  LB_ADDSTRING, 0, (LPARAM)(LPSTR) pathtmp); //namelist[i]->d_name);
	       } else {
		sprintf(pathtmp,"  %s",namelist[i]->d_name); 
		if ((strcmp(pathtmp+strlen(pathtmp)-strlen(currentfilter),currentfilter)==0) || (strcmp(currentfilter,".*")==0))
		  SendDlgItemMessage ( hwnd, ID_LIST,  LB_ADDSTRING, 0, (LPARAM)(LPSTR) pathtmp); //namelist[i]->d_name);
	       }
	     }
               free(namelist[i]);
	       ++i;
           }
           free(namelist);
       }
}

int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

/* Window procedure for the dialog box */
static LRESULT
FileOpenCtrlProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	static LPOPENFILENAME mdData;
	int sel,isdir;
	char dirorfilename[PATH_MAX];
	char dirpath[PATH_MAX];
	char* slashptr;
	int dirflag;
	HWND hEdit,htty;
	const char blank[2]=" ";

  dirflag=0;
  switch(message) {
    case WM_INITDIALOG:
      mdData=lParam; //valid if WM_INITDIALOG message only
      if (mdData->lpstrInitialDir != NULL) {
	sprintf(curpath,"%s",mdData->lpstrInitialDir);
      }
      if (strcmp(curpath,".")==0) getcwd(curpath, sizeof(curpath));
      fill_listbox(hwnd,curpath);
      if( htty == NULL ) htty = CreateFont ( 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, "arial" );
      SendDlgItemMessage(hwnd, ID_LIST, WM_SETFONT, (WPARAM)htty, 0 );
      SendDlgItemMessage(hwnd, ID_LIST, LB_SETITEMHEIGHT, 0, 15 );
      
      SendDlgItemMessage(hwnd, ID_EDIT, WM_SETFONT, (WPARAM)htty, 0 );
      //now the only difference of save file to open file - initialize edit control if specified
      if ((mdData->lpstrFileTitle != NULL) && (mdData->dwSaveDialog==1)) { 
	SendDlgItemMessage (hwnd, ID_EDIT, WM_SETTEXT, 0, (LPARAM)(char*)mdData->lpstrFileTitle);
      }
      
      int i=0;
      int fptr=0;
      char filtertmp[1024];
      char edittmp[1024];
     if (mdData->lpstrFilter!=0) {
      SendDlgItemMessage(hwnd, ID_COMBO, WM_SETFONT, (WPARAM)htty, 0 );
      while (strlen(mdData->lpstrFilter+fptr) !=0) {
	sprintf(filterpairs[i].name,"%s",mdData->lpstrFilter+fptr);
	fptr=fptr+strlen(filterpairs[i].name)+1;
	sprintf(filterpairs[i].suffix,"%s",mdData->lpstrFilter+fptr);
	fptr=fptr+strlen(filterpairs[i].suffix)+1;
	i++;
	sprintf(filterpairs[i].name,""); //='\0'; //clear first
	if (i>10) break; //safety
      }
      i=0;
     } // mdData->lpstrFilter!=0
      while (strlen(filterpairs[i].name)>0) { //filters present - fill combobox
	sprintf(filtertmp,"%s [%s]",filterpairs[i].name,filterpairs[i].suffix);
	SendDlgItemMessage ( hwnd, ID_COMBO,  CB_ADDSTRING, 0, (LPARAM)(LPSTR) filtertmp); 
	i++;
      }

      return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
	case ID_LIST:     
		if (HIWORD(wParam) == LBN_SELCHANGE | LBN_DBLCLK) { 
                    sel = SendDlgItemMessage ( hwnd, ID_LIST, LB_GETCURSEL, 1, 0 );
		    SendDlgItemMessage (hwnd, ID_LIST, LB_GETTEXT, sel, (LPARAM)dirorfilename);
		    strcpy(dirorfilename,dirorfilename+2); //clear first two columns "> "
		    if (HIWORD(wParam) == LBN_DBLCLK) {
		    if (strcmp(dirorfilename,"..")==0){ //move up in the directory tree
		      slashptr=strrchr((char*)curpath, '/');
		      *slashptr='\0';  
		    } else {
		      sprintf(dirpath,"%s/%s",curpath,dirorfilename);
		      isdir = isDirectory(dirpath);
		      dirflag=1;
		      if (isdir==1) {
			strcpy(curpath,dirpath);
			} else {
			//double click - return filename in ofn structure just as OK button
			SendMessageW(hwnd, WM_COMMAND, (WPARAM)ID_OK, 0);
			PostQuitMessage(0);
			}
		    }
		    SendDlgItemMessage (hwnd, ID_LIST, LB_RESETCONTENT, 0, 0);
		    fill_listbox(hwnd,curpath);
		    } //dblclk
		    sprintf(dirpath,"%s/%s",curpath,dirorfilename);
		    if ((strcmp(dirorfilename,"..")==0) || (isDirectory(dirpath)==1) || dirflag==1) {
		      SendDlgItemMessage (hwnd, ID_EDIT, WM_SETTEXT, 0, (LPARAM)(char*)blank); //clear edit control
		    } else {
		      SendDlgItemMessage (hwnd, ID_EDIT, WM_SETTEXT, 0, (LPARAM)(char*)dirorfilename );
		      //alternative:
		      //hEdit = GetDlgItem(hwnd, ID_EDIT);
                      //SetWindowText(hEdit, dirorfilename);
		    }
		} //LBN_SELCHANGE | LBN_DBLCLK
	    break;
	case ID_OK:
	      //set filename in ofn structure to dirorfilename
	      //did the user type a file name into the edit box?
	      hEdit = GetDlgItem(hwnd, ID_EDIT);
	      GetWindowText(hEdit, edittmp, 1024);
	      //SendDlgItemMessage (hwnd, ID_EDIT, WM_GETTEXT, 0, (LPARAM)(char*)edittmp);
	      if (strlen(edittmp)>0) {
			//set fileTitle in ofn structure to dirorfilename
			if (mdData->nMaxFileTitle>strlen(edittmp)) { //buffer allocated by app?
			  sprintf(mdData->lpstrFileTitle,"%s",edittmp);
			}
			//set File in ofn structure to curpath+dirorfilename
			sprintf(dirpath,"%s/%s",curpath,edittmp);
			if (mdData->nMaxFile>strlen(dirpath)) { //buffer allocated by app?
			  sprintf(mdData->lpstrFile,"%s/%s",curpath,edittmp);
			}		
	      } else {
              sel = SendDlgItemMessage ( hwnd, ID_LIST, LB_GETCURSEL, 1, 0 );
	      SendDlgItemMessage (hwnd, ID_LIST, LB_GETTEXT, sel, (LPARAM)dirorfilename);
	      strcpy(dirorfilename,dirorfilename+2); //clear first two columns "> "
	      if (strcmp(dirorfilename,"..")==0){ //move up in the directory tree
		//set filename in ofn structure to NULL
		MessageBox(NULL, "Please select a file name, not a directory!", "File selection dialog", MB_OKCANCEL );
		break; 
	      } else {
	        sprintf(dirpath,"%s/%s",curpath,dirorfilename);
	        isdir = isDirectory(dirpath);
	        if (isdir==1) {
			//set filename in ofn structure to NULL
			MessageBox(NULL, "Please select a file name, not a directory!", "File selection dialog", MB_OKCANCEL );
			break;
		} else {
			//set fileTitle in ofn structure to dirorfilename
			if (mdData->nMaxFileTitle>strlen(dirorfilename)) { //buffer allocated by app?
			  sprintf(mdData->lpstrFileTitle,"%s",dirorfilename);
			}
			//set File in ofn structure to curpath+dirorfilename
			if (mdData->nMaxFile>strlen(dirpath)) { //buffer allocated by app?
			  sprintf(mdData->lpstrFile,"%s/%s",curpath,dirorfilename);
			}
		}
	      }
	      } //strlen(edittmp)
	      PostQuitMessage(0);
	      break;
        case ID_CANCEL: //Button
	  mdData->lpstrFileTitle='\0';
	  mdData->lpstrFile='\0';
	  EndDialog(hwnd, wParam);
          return TRUE;
	  break;
	case ID_COMBO:
	  if (HIWORD(wParam) == CBN_SELENDOK) { //drop down list closed now
	  sel = SendDlgItemMessage ( hwnd, ID_COMBO, CB_GETCURSEL, 0, 0 );
	  sprintf(currentfilter,"%s",filterpairs[sel].suffix);
	  SendDlgItemMessage (hwnd, ID_LIST, LB_RESETCONTENT, 0, 0);
	  fill_listbox(hwnd,curpath);
	  }
	  break;
      } //switch (LOWORD(wParam))
      return 0;
    case WM_CLOSE:
      EndDialog(hwnd, IDCANCEL);
      return 1;
  }
  return 0;
}

BOOL WINAPI GetOpenFileNameIndirect (LPOPENFILENAME *Arg1){
    LPOPENFILENAME mdData;
    mdData=Arg1;
    DLGTEMPLATE *tpl;
    DLGITEMTEMPLATE *itxt,*ilist,*iedit,*ifilter,*icombo;
    
    int width, height;
    RECT r;

    char capbuf[32];
    HMODULE hUser32 = 0;
    LPVOID buf;
    BYTE *dest;
    LPCSTR caption, text;
    HFONT hFont;
    HICON Icon = (HICON)0;
    HDC hDC;
    int bufsize, ret, caplen, textlen, btnlen, i, btnleft, btntop, lmargin, nButtons = 0;
    LONG Buttons[OFN_MAXBTNS];
    char ButtonText[OFN_MAXBTNS][OFN_MAXBTNSTR];
    DLGITEMTEMPLATE *ibtn[OFN_MAXBTNS];
    RECT btnrect, txtrect, rc;
    SIZE btnsize;
    BOOL defbtn = FALSE;
    DWORD units = GetDialogBaseUnits();

    caption = (LPCSTR)mdData->lpstrTitle;
    caplen = strlen(caption);    

    bufsize=296; //select 4096 first while testing and then use value at end of resource definition
    if (!(buf = calloc( 1, bufsize + SAFETY_MARGIN))) return 0;

    hDC = CreateCompatibleDC(0);
    hFont = GetStockObject(DEFAULT_GUI_FONT);
    
    GetWindowRect(GetDesktopWindow(), &r);
    width = height = r.right / 2.5;
    height /= 1.3;

    tpl = (DLGTEMPLATE *)buf;
    tpl->style = WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU;
    tpl->style |= DS_CENTER | DS_MODALFRAME | DS_NOIDLEMSG;
    tpl->dwExtendedStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT;
    tpl->dwExtendedStyle |= WS_EX_TOPMOST;
    /* set size of the window */
    tpl->x = 100; tpl->y = 100; tpl->cx = width; tpl->cy = height; 
    /**********************************************************************/
    tpl->cdit = 7; //number of items to follow
    //set to 7 to display filter controls - filtering is not supported yet
    /**********************************************************************/

    dest = ((BYTE *)tpl + DLGTEMPLATE_SIZE);
    *(WORD*)dest = 0; /* no menu */
    *(((WORD*)dest) + 1) = 0; /* use default window class */
    dest += 2 * sizeof(WORD); /*advance by menu+class words */
    dest += CopyToWchar ((PWCHAR)dest, caption, caplen);

    /* create listbox */
    dest = (BYTE*)(((LONG)dest + 3) & ~3); //align to word
    ilist = (DLGITEMTEMPLATE *)dest;
    ilist->style = WS_VSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE | LBS_NOTIFY;
    ilist->dwExtendedStyle = 0;
    ilist->id = ID_LIST;
    /* set size and position of the message static */
    ilist->x = 100;
    ilist->y = 30;
    ilist->cx = 300;
    ilist->cy = 254;
    
    dest += DLGITEMTEMPLATE_SIZE;
    *(WORD*)dest = 0xFFFF;
    dest += sizeof(WORD);
    *(WORD*)dest = 0x0083; /* listbox control */
    dest += sizeof(WORD);
    dest += CopyToWchar ((PWCHAR)dest, "",0); //no creation data array
    *(WORD*)dest = 0;
    dest += sizeof(WORD);

    /* create editbox */
    dest = (BYTE*)(((LONG)dest + 3) & ~3); //align to word
    iedit = (DLGITEMTEMPLATE *)dest;
    iedit->style = WS_BORDER | WS_CHILD | WS_VISIBLE;
    iedit->dwExtendedStyle = 0;
    iedit->id = ID_EDIT;
    /* set size and position of the message static */
    iedit->x = 100;
    iedit->y = 300;
    iedit->cx = 300;
    iedit->cy = 18;
    
    dest += DLGITEMTEMPLATE_SIZE;
    *(WORD*)dest = 0xFFFF;
    dest += sizeof(WORD);
    *(WORD*)dest = 0x0081; /* edit control */
    dest += sizeof(WORD);
    dest += CopyToWchar ((PWCHAR)dest, "", 0);
    *(WORD*)dest = 0;
    dest += sizeof(WORD);

    /* create static for text "Name:" */
    dest = (BYTE*)(((LONG)dest + 3) & ~3); //align to word
    itxt = (DLGITEMTEMPLATE *)dest;
    itxt->style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX;
    itxt->style |= SS_LEFT;
    itxt->dwExtendedStyle = 0;
    itxt->id = ID_STATICN;
    /* set size and position of the message static */
    itxt->x = 65;
    itxt->y = 303;
    itxt->cx = 30;
    itxt->cy = 18;
    
    dest += DLGITEMTEMPLATE_SIZE;
    *(WORD*)dest = 0xFFFF;
    dest += sizeof(WORD);
    *(WORD*)dest = 0x0082; /* static control */
    dest += sizeof(WORD);
    dest += CopyToWchar ((PWCHAR)dest, "Name:", 5);
    *(WORD*)dest = 0;
    dest += sizeof(WORD);

    /* create OK button */
   i=0;
   dest = (BYTE*)(((LONG)dest + 3) & ~3); //align to word
   ibtn[i] = (DLGITEMTEMPLATE *)dest;
   ibtn[i]->style = WS_CHILD | WS_VISIBLE; 
   ibtn[i]->dwExtendedStyle = 0;
   ibtn[i]->id = ID_OK;
   dest += DLGITEMTEMPLATE_SIZE;
   *(WORD*)dest = 0xFFFF;
   dest += sizeof(WORD);
   *(WORD*)dest = 0x0080; /* button control */
   dest += sizeof(WORD);
   sprintf(ButtonText[i],"OK");
   btnlen = strlen(ButtonText[i]);
   dest += CopyToWchar ((PWCHAR)dest, ButtonText[i], btnlen);
   *(WORD*)dest = 0;
   dest += sizeof(WORD); 
   ibtn[i]->x = 420;
   ibtn[i]->y = 300;
   ibtn[i]->cx = 50;
   ibtn[i]->cy = 18; 
   btnrect.left = btnrect.top = 0;
   btnrect.right=ibtn[i]->cx;
   btnrect.bottom=ibtn[i]->cy;
   SelectObject(hDC, hFont);
   DrawText(hDC, ButtonText[i], btnlen, &btnrect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);

   /* create Cancel button */
   i=1;
   dest = (BYTE*)(((LONG)dest + 3) & ~3); //align to word
   ibtn[i] = (DLGITEMTEMPLATE *)dest;
   ibtn[i]->style = WS_CHILD | WS_VISIBLE; 
   ibtn[i]->dwExtendedStyle = 0;
   ibtn[i]->id = ID_CANCEL;
   dest += DLGITEMTEMPLATE_SIZE;
   *(WORD*)dest = 0xFFFF;
   dest += sizeof(WORD);
   *(WORD*)dest = 0x0080; /* button control */
   dest += sizeof(WORD);
   sprintf(ButtonText[i],"Cancel");
   btnlen = strlen(ButtonText[i]);
   dest += CopyToWchar ((PWCHAR)dest, ButtonText[i], btnlen);
   *(WORD*)dest = 0;
   dest += sizeof(WORD);
   ibtn[i]->x = 420;
   ibtn[i]->y = 330;
   ibtn[i]->cx = 50;
   ibtn[i]->cy = 18;   
   btnrect.left = btnrect.top = 0;
   btnrect.right=ibtn[i]->cx;
   btnrect.bottom=ibtn[i]->cy;
   SelectObject(hDC, hFont);
   DrawText(hDC, ButtonText[i], btnlen, &btnrect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);

   /* create combobox for filters*/
    dest = (BYTE*)(((LONG)dest + 3) & ~3); //align to word
    ilist = (DLGITEMTEMPLATE *)dest;
    ilist->style = CBS_DROPDOWN | WS_VSCROLL | WS_CHILD | WS_VISIBLE;
    ilist->dwExtendedStyle = 0;
    ilist->id = ID_COMBO;
    /* set size and position of the message static */
    ilist->x = 100;
    ilist->y = 330;
    ilist->cx = 300;
    ilist->cy = (18*5);
    
    dest += DLGITEMTEMPLATE_SIZE;
    *(WORD*)dest = 0xFFFF;
    dest += sizeof(WORD);
    *(WORD*)dest = 0x0085; /* combobox control */
    dest += sizeof(WORD);
    dest += CopyToWchar ((PWCHAR)dest, "",0); //no creation data array
    *(WORD*)dest = 0;
    dest += sizeof(WORD);

    /* create static for text "Filter:" */
    dest = (BYTE*)(((LONG)dest + 3) & ~3); //align to word
    itxt = (DLGITEMTEMPLATE *)dest;
    itxt->style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX;
    itxt->style |= SS_LEFT;
    itxt->dwExtendedStyle = 0;
    itxt->id = ID_STATICF;
    /* set size and position of the message static */
    itxt->x = 65;
    itxt->y = 333;
    itxt->cx = 30;
    itxt->cy = 18;
    
    dest += DLGITEMTEMPLATE_SIZE;
    *(WORD*)dest = 0xFFFF;
    dest += sizeof(WORD);
    *(WORD*)dest = 0x0082; /* static control */
    dest += sizeof(WORD);
    dest += CopyToWchar ((PWCHAR)dest, "Filter:", 7);
    *(WORD*)dest = 0;
    dest += sizeof(WORD);

   //printf("dest:%d\n",dest+4-(BYTE *)tpl); //calc buffer size 
   if(hDC) DeleteDC(hDC);

#if 0
    {
    	int i;
    	unsigned char *pChar = (unsigned char*)tpl;
    	char szBuf[64], *pDest=szBuf;

    	//for (i=0; i<bufsize; i++)
	for (i=0; i<296+32; i++)
    	{
    		if (i && i%8 == 0)
    		{
    			int j;

    			for (j=i-8; j<i; j++)
    				sprintf(pDest++, "%c", isprint(((unsigned char*)tpl)[j])?((unsigned char*)tpl)[j]:'.');
    			*pDest=0;
    			printf ("%s\n", szBuf);
    			pDest = szBuf;
    		}
    		sprintf (pDest, "%02X ", *pChar);
    		pDest+=3;
    		pChar++;
    	}
    }
#endif

    ret =  DialogBoxIndirectParam(0, tpl, GetDesktopWindow(), FileOpenCtrlProc, (LPARAM)mdData); //Arg1);

    if(hFont) DeleteObject(hFont);
    free (buf);
    return ret;
}


BOOL WINAPI GetOpenFileName (LPOPENFILENAME Arg1){
  LPOPENFILENAME ofnData;
  ofnData=Arg1;
  ofnData->dwSaveDialog=0;
  return GetOpenFileNameIndirect (ofnData);
}

BOOL WINAPI GetSaveFileName (LPOPENFILENAME Arg1){
  LPOPENFILENAME ofnData;
  ofnData=Arg1;
  ofnData->dwSaveDialog=1;
  return GetOpenFileNameIndirect (ofnData);
}

/*
typedef struct tagOFNA {
  DWORD         lStructSize;
  HWND          hwndOwner;
  HINSTANCE     hInstance;
  LPCSTR        lpstrFilter;
  LPSTR         lpstrCustomFilter;
  DWORD         nMaxCustFilter;
  DWORD         nFilterIndex;
  LPSTR         lpstrFile;
  DWORD         nMaxFile;
  LPSTR         lpstrFileTitle;
  DWORD         nMaxFileTitle;
  LPCSTR        lpstrInitialDir;
  LPCSTR        lpstrTitle;
  DWORD         Flags;
  WORD          nFileOffset;
  WORD          nFileExtension;
  LPCSTR        lpstrDefExt;
  LPARAM        lCustData;
  LPOFNHOOKPROC lpfnHook;
  LPCSTR        lpTemplateName;
  LPEDITMENU    lpEditInfo;
  LPCSTR        lpstrPrompt;
  void          *pvReserved;
  DWORD         dwSaveDialog;
  DWORD         FlagsEx;
} OPENFILENAMEA, *LPOPENFILENAMEA;
*/
/* in winres.h
enum MW_DLGITEMS_CLASSID
{
    DLGITEM_CLASS_BUTTON = 0x80,
    DLGITEM_CLASS_EDIT, //81
    DLGITEM_CLASS_STATIC, //82
    DLGITEM_CLASS_LISTBOX, //83
    DLGITEM_CLASS_SCROLLBAR, // 84
    DLGITEM_CLASS_COMBOBOX, //85
}; 
*/
