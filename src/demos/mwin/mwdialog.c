/* Very simple example how to use a resource file with the MWin API.
* This example opens a dialog window with a button. If you click the
* button the window will move and the title will be modified.
* See the mwdvetest example for a full demonstration of the available
* controls.
* Georg Potthast 2018 */

#include <windows.h>
#include "../../images/demos/mwin/mwdialog/mwdialog.h" //defines e.g. IDD_DLGMAIN

#define ID_BUTTON 10   //specified in the resource file

/* function declaration */
DLGBOOL CALLBACK mainDlgProc ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    MwInitializeDialogs ( hInstance );
    DialogBox ( hInstance, MAKEINTRESOURCE(IDD_DLGMAIN), NULL, (DLGPROC)mainDlgProc );

    return 0;
}
 
DLGBOOL CALLBACK mainDlgProc ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  static int toggle=0;
  switch(message)
    {
      case WM_COMMAND:
	    if (LOWORD(wParam)==ID_BUTTON) 
                {
		if (toggle==0) {
		  SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM) (char*)" This is the new title! ");
		  toggle=1;
		  InvalidateRect(hWnd, NULL, TRUE);
		} else {
		  SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM) (char*)"Simple Dialog Window");
		  toggle=0; 
		  InvalidateRect(hWnd, NULL, TRUE);
		}
                break;
	      }
      default:
      return FALSE; 
    }
    return 0;
}
