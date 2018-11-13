/* Very simple example how to use a resource file with the MWin API.
* This example opens a dialog window with a button. If you click the
* button the window will move and the title will be modified.
* See the mwdvetest example for a full demonstration of the available
* controls.
* Georg Potthast 2018 */

#include <windows.h>
#include "mwdialog.h" //defines e.g. IDD_DLGMAIN

#define ID_BUTTON 10   //specified in the resource file

/* function declaration */
BOOL CALLBACK mainDlgProc ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    MwInitializeDialogs ( hInstance );
    DialogBox ( hInstance, MAKEINTRESOURCE(IDD_DLGMAIN), NULL, (DLGPROC)mainDlgProc );

    return 0;
}
 
BOOL CALLBACK mainDlgProc ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  static int toggle=0;
  switch(message)
    {
      case WM_COMMAND:
	    //printf("ID_BUTTON:%d\n",LOWORD(wParam));
	    if (LOWORD(wParam)==ID_BUTTON) 
                {
		if (toggle==0) {
		SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM) (char*)" You clicked the button! ");
		toggle=1;		  
		MoveWindow(hWnd, 50, 50, 127*1.55, 59*1.8, 1);
		} else {
		SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM) (char*)"Simple Dialog Window");
		toggle=0; 
		MoveWindow(hWnd, 0, 0, 197, 106, 1);
		}
                break;
	      }
      default:
      return FALSE; 
    }
    return 0;
}
