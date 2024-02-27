/*
 * "Open and Save As Dialog Box" for Microwindows win32 api.
 *
 * 2018, Georg Potthast
 *
 * The save dialog allows to initialize the filename edit control
 * See the mwopenfile.c example for the usage of this control
 * 
 */
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "uni_std.h"
#include "windows.h"	

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
char currentfilter[128];
char curpath[256] = {"."};

LRESULT CALLBACK FileOpenCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void fill_listbox(HWND hwnd,const char *path)
{
#ifndef __fiwix__
       struct dirent **namelist;
       int n;
       int i=0;
       char pathtmp[256+2] = {' ',' '};

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
#endif
}

static int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)				// FIXME non-portable 
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

/* Window procedure for the dialog box */
LRESULT CALLBACK FileOpenCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static LPOPENFILENAME mdData;
	int sel,isdir;
	char dirorfilename[256];
	char dirpath[256];
	char* slashptr;
	int dirflag;
	HWND hEdit;
	HFONT hFont;
	const char blank[2]=" ";

  dirflag=0;
  switch(message) {
    case WM_INITDIALOG:
      mdData = (LPOPENFILENAME)lParam; //valid if WM_INITDIALOG message only
      if (mdData->lpstrInitialDir != NULL) {
	sprintf(curpath,"%s",mdData->lpstrInitialDir);
      }
      if (strcmp(curpath,".")==0) getcwd(curpath, sizeof(curpath));
      fill_listbox(hwnd,curpath);
      hFont = CreateFont ( 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, "arial" );
      SendDlgItemMessage(hwnd, ID_LIST, WM_SETFONT, (WPARAM)hFont, 0 );
      SendDlgItemMessage(hwnd, ID_LIST, LB_SETITEMHEIGHT, 0, 15 );
      
      SendDlgItemMessage(hwnd, ID_EDIT, WM_SETFONT, (WPARAM)hFont, 0 );
      //now the only difference of save file to open file - initialize edit control if specified
      if ((mdData->lpstrFileTitle != NULL) && (mdData->dwSaveDialog==1)) { 
	SendDlgItemMessage (hwnd, ID_EDIT, WM_SETTEXT, 0, (LPARAM)(char*)mdData->lpstrFileTitle);
      }
      
      int i=0;
      int fptr=0;
      char filtertmp[1024];
      char edittmp[1024];
     if (mdData->lpstrFilter!=0) {
      SendDlgItemMessage(hwnd, ID_COMBO, WM_SETFONT, (WPARAM)hFont, 0 );
      while (strlen(mdData->lpstrFilter+fptr) !=0) {
	sprintf(filterpairs[i].name,"%s",mdData->lpstrFilter+fptr);
	fptr=fptr+strlen(filterpairs[i].name)+1;
	sprintf(filterpairs[i].suffix,"%s",mdData->lpstrFilter+fptr);
	fptr=fptr+strlen(filterpairs[i].suffix)+1;
	i++;
	filterpairs[i].name[0]='\0'; //clear first
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
		if (HIWORD(wParam) == (LBN_SELCHANGE | LBN_DBLCLK)) { 
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
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_OK, 0);
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
		  // FIXME strcpy below crashes system
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
	  mdData->lpstrFileTitle = NULL;
	  mdData->lpstrFile = NULL;
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

BOOL WINAPI
GetOpenFileNameIndirect(LPOPENFILENAME Arg1){
    LPOPENFILENAME mdData = Arg1;
    DLGTEMPLATE *tpl;
    int width, height;
    RECT r;
    LPVOID buf;
    BYTE *dest;
    LPCSTR caption;
    int ret;

    caption = (LPCSTR)mdData->lpstrTitle;

    if (!(buf = malloc(1024)))
		return 0;

    GetWindowRect(GetDesktopWindow(), &r);
	width = 320;
	height = 290;
	tpl = buf;

	/* dialog template*/
    DWORD style = WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU | DS_CENTER | DS_MODALFRAME|DS_NOIDLEMSG;
    DWORD exstyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT | WS_EX_TOPMOST;
	dest = resDialogTemplate((BYTE *)tpl, caption, style, exstyle, 100, 100, width, height, NULL, NULL, 7);

	/* dialog items*/
    style = WS_VSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE | LBS_NOTIFY;
	dest = resDialogItemTemplate(dest, style, 0, ID_LIST, 40, 30, 200, 154, DLGITEM_CLASS_LISTBOX, NULL);

	dest = resDialogItemTemplate(dest, style, 0, ID_EDIT, 40, 200, 200, 18, DLGITEM_CLASS_EDIT, NULL);

    style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX;
	dest = resDialogItemTemplate(dest, style, 0, ID_STATICN, 10, 203, 30, 13, DLGITEM_CLASS_STATIC, "Name:");

	dest = resDialogItemTemplate(dest, WS_CHILD|WS_VISIBLE, 0, ID_OK, 250, 200, 50, 18, DLGITEM_CLASS_BUTTON, "OK");

	dest = resDialogItemTemplate(dest, WS_CHILD|WS_VISIBLE, 0, ID_CANCEL, 250, 230, 50, 18, DLGITEM_CLASS_BUTTON, "Cancel");

    style = CBS_DROPDOWN | WS_VSCROLL | WS_CHILD | WS_VISIBLE;
	dest = resDialogItemTemplate(dest, style, 0, ID_COMBO, 40, 230, 200, 18*5, DLGITEM_CLASS_COMBOBOX, NULL);

    style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT;
	dest = resDialogItemTemplate(dest, style, 0, ID_STATICF, 10, 233, 30, 18, DLGITEM_CLASS_STATIC, "Filter:");

    ret =  DialogBoxIndirectParam(0, tpl, GetDesktopWindow(), FileOpenCtrlProc, (LPARAM)mdData);

    free (buf);
    return ret;
}

BOOL WINAPI
GetOpenFileName(LPOPENFILENAME Arg1)
{
	Arg1->dwSaveDialog = 0;
	return GetOpenFileNameIndirect(Arg1);
}

BOOL WINAPI
GetSaveFileName(LPOPENFILENAME Arg1)
{
	Arg1->dwSaveDialog=1;
	return GetOpenFileNameIndirect(Arg1);
}
