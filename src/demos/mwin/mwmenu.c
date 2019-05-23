/* Example how to implement a menu system on the application level.
 * 
 * A row of buttons at the top form the menu bar. The last button is just to
 * extend the bar to the right window border. If you click on one of the buttons
 * a submenu will appear implemented using the listbox widget.
 * 
 * A single click on an item in the listbox will move the bar to the item
 * a double-click will select it and close the submenu.
 * 
  * If you click the right mouse button anywhere in the window, the listbox 
 * implementing a context menu will move and be displayed at the mouse pointer
 * position. If you click the left button, the listbox will hide. 
 * 
 * Georg Potthast 2018
 */
#define MWINCLUDECOLORS
#include <windows.h>
#include <string.h>

#define ID_BUTTON1 1
#define ID_BUTTON2 2
#define ID_BUTTON3 3
#define ID_LIST1 7
#define ID_LIST2 8
#define ID_LIST3 9
#define ID_LIST4 10

/* 1,1+menuheight is the upper left corner for the application area */
#define menuheight 24
HWND hbutton;
char printselection[25];

LRESULT CALLBACK wproc(HWND,UINT,WPARAM,LPARAM);

char * farray[] = {
    "New       ",
    "Open      ",
    "Save      ",
    "Save as   ",
    "Settings  ",
    "Exit      ",
};
#define n_farray (sizeof (farray) / sizeof (const char *))

char * earray[] = {
    "Copy      ",
    "Cut       ",
    "Paste     ",
};
#define n_earray (sizeof (earray) / sizeof (const char *))

char * harray[] = {
    "Help     ",
    "About    ",
};
#define n_harray (sizeof (harray) / sizeof (const char *))

char * carray[] = {
    "New Window     ",
    "New Subwindow  ",
};
#define n_carray (sizeof (carray) / sizeof (const char *))

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
        static char szAppName[]="Menu";
        HWND hwnd;
        MSG msg;
        WNDCLASS wndclass;

	int width, height;
	int i;
	RECT r;

	GetWindowRect(GetDesktopWindow(), &r);
	width = height = r.right / 2;

		MwInitializeDialogs(hInstance); // register stand control and dialog classes

        wndclass.style          = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc    = (WNDPROC)wproc;
        wndclass.cbClsExtra     =0;
        wndclass.cbWndExtra     =0;
        wndclass.hInstance      =0;
        wndclass.hIcon          =0;
        wndclass.hCursor        =0;
        wndclass.hbrBackground  =(HBRUSH)GetStockObject(LTGRAY_BRUSH);
        wndclass.lpszMenuName   =NULL;
        wndclass.lpszClassName  = szAppName;

        RegisterClass(&wndclass);
        hwnd=CreateWindowEx(0L, szAppName, "Menu Example",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                          NULL, NULL, NULL, NULL);
	
	CreateWindowEx(0L, "BUTTON", "File",
		WS_CHILD | WS_VISIBLE,
		1, 1, 51, menuheight,
		hwnd, (HMENU)ID_BUTTON1, NULL, NULL);

	CreateWindowEx(0L, "BUTTON", "Edit",
		WS_CHILD | WS_VISIBLE,
		50, 1, 52, menuheight,
		hwnd, (HMENU)ID_BUTTON2, NULL, NULL);

	CreateWindowEx(0L, "BUTTON", "Help",
		WS_CHILD | WS_VISIBLE,
		100, 1, 53, menuheight,
		hwnd, (HMENU)ID_BUTTON3, NULL, NULL);

	hbutton = CreateWindowEx(0L, "BUTTON", "",
		WS_CHILD | WS_VISIBLE,
		150, 1, width-150, menuheight,
		hwnd, NULL, NULL, NULL);
	
	/* create unvisible listboxes */
	CreateWindowEx(0L, "LISTBOX", "OK",	
		WS_CHILD | LBS_NOTIFY, 1, 1+menuheight, 100, 84, //need 14 for one entry
		hwnd, (HMENU)ID_LIST1, NULL, NULL);
	CreateWindowEx(0L, "LISTBOX", "OK",	
		WS_CHILD | LBS_NOTIFY, 50, 1+menuheight, 100, 42,
		hwnd, (HMENU)ID_LIST2, NULL, NULL);
	CreateWindowEx(0L, "LISTBOX", "OK",	
		WS_CHILD | LBS_NOTIFY, 100, 1+menuheight, 100, 28,
		hwnd, (HMENU)ID_LIST3, NULL, NULL);
	CreateWindowEx(0L, "LISTBOX", "OK",	
		WS_CHILD | LBS_NOTIFY, 300, 250, 100, 28,
		hwnd, (HMENU)ID_LIST4, NULL, NULL);

for (i = 0; i < n_farray; i++) {SendDlgItemMessage ( hwnd, ID_LIST1,  LB_ADDSTRING, 0, (LPARAM)(LPSTR)farray[i]); }
for (i = 0; i < n_earray; i++) {SendDlgItemMessage ( hwnd, ID_LIST2,  LB_ADDSTRING, 0, (LPARAM)(LPSTR)earray[i]); }
for (i = 0; i < n_harray; i++) {SendDlgItemMessage ( hwnd, ID_LIST3,  LB_ADDSTRING, 0, (LPARAM)(LPSTR)harray[i]); }
for (i = 0; i < n_carray; i++) {SendDlgItemMessage ( hwnd, ID_LIST4,  LB_ADDSTRING, 0, (LPARAM)(LPSTR)carray[i]); }

        ShowWindow(hwnd,iCmdShow);
        UpdateWindow(hwnd);

#if !MULTIAPP
        while (GetMessage(&msg,NULL,0,0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
        }      
#endif
        return 0;
}       
LRESULT CALLBACK wproc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{       
        HDC hdc;
	int sel;
        PAINTSTRUCT ps;
	static POINT	mousept;
	POINT pt;
	HWND hctrl;
        
        switch (iMsg) {
        case WM_CREATE:
                break;

	case WM_PAINT:
	     hdc=BeginPaint(hwnd,&ps);
             EndPaint(hwnd,&ps);
             break;

	case WM_COMMAND:
            if (LOWORD(wParam) == ID_LIST1) {     
                if (HIWORD(wParam) == LBN_DBLCLK) {                   
                    sel = SendDlgItemMessage ( hwnd, ID_LIST1, LB_GETCURSEL, 1, 0 );
		    ShowWindow(GetDlgItem(hwnd, ID_LIST1), SW_HIDE);
		    sprintf(printselection,"You selected: %s",farray[sel]);
		    SetWindowText(hbutton, printselection);
		    if (sel==5) PostQuitMessage(0);
               }
	    }
            if (LOWORD(wParam) == ID_LIST2) {     
                if (HIWORD(wParam) == LBN_DBLCLK) {                   
                    sel = SendDlgItemMessage ( hwnd, ID_LIST2, LB_GETCURSEL, 1, 0 );
		    ShowWindow(GetDlgItem(hwnd, ID_LIST2), SW_HIDE);
		    sprintf(printselection,"You selected: %s",earray[sel]);
		    SetWindowText(hbutton, printselection);
               }
	    }
            if (LOWORD(wParam) == ID_LIST3) {     
                if (HIWORD(wParam) == LBN_DBLCLK) {                   
                    sel = SendDlgItemMessage ( hwnd, ID_LIST3, LB_GETCURSEL, 1, 0 );
		    ShowWindow(GetDlgItem(hwnd, ID_LIST3), SW_HIDE);
		    sprintf(printselection,"You selected: %s",harray[sel]);
		    SetWindowText(hbutton, printselection);
               }
	    }
            if (LOWORD(wParam) == ID_LIST4) {     
                if (HIWORD(wParam) == LBN_DBLCLK) {                   
                    sel = SendDlgItemMessage ( hwnd, ID_LIST4, LB_GETCURSEL, 1, 0 );
		    ShowWindow(GetDlgItem(hwnd, ID_LIST4), SW_HIDE);
		    sprintf(printselection,"You selected: %s",carray[sel]);
		    SetWindowText(hbutton, printselection);
               }
	    }

	    if (LOWORD(wParam)==ID_BUTTON1){
		ShowWindow(GetDlgItem(hwnd, ID_LIST2), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST3), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST1), SW_NORMAL);
		SendDlgItemMessage (hwnd, ID_LIST1, LB_SETCURSEL, 0, 0 );
		//SendDlgItemMessage (hwnd, ID_LIST1, LB_SETSEL, 0, 0 );
	    } 
	    if (LOWORD(wParam)==ID_BUTTON2){
		ShowWindow(GetDlgItem(hwnd, ID_LIST1), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST3), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST2), SW_NORMAL);
		SendDlgItemMessage (hwnd, ID_LIST2, LB_SETCURSEL, 0, 0 );
	    } 
	    if (LOWORD(wParam)==ID_BUTTON3){
		ShowWindow(GetDlgItem(hwnd, ID_LIST1), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST2), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST3), SW_NORMAL);
		SendDlgItemMessage (hwnd, ID_LIST3, LB_SETCURSEL, 0, 0 );
	    } 
	    break;
	case WM_LBUTTONDOWN:
		ShowWindow(GetDlgItem(hwnd, ID_LIST1), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST2), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST3), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST4), SW_HIDE);
		break;
	case WM_RBUTTONDOWN:  			
		ShowWindow(GetDlgItem(hwnd, ID_LIST1), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST2), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, ID_LIST3), SW_HIDE);
		POINTSTOPOINT(pt, lParam);
		mousept.x = pt.x;
		mousept.y = pt.y;
		//printf("Right button down at:%d,%d\n",mousept.x,mousept.y);
		hctrl = GetDlgItem(hwnd,ID_LIST4);
		MoveWindow(hctrl, mousept.x,mousept.y, 100, 28, 1);
		ShowWindow(GetDlgItem(hwnd, ID_LIST4), SW_NORMAL);
		SendDlgItemMessage (hwnd, ID_LIST4, LB_SETCURSEL, 0, 0 );
		break;		
        case WM_DESTROY:
                PostQuitMessage(0);
                break;
        default:
                return DefWindowProc(hwnd,iMsg,wParam,lParam);
        }      
        return 0;
}
