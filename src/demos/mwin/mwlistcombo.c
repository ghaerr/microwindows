/* Example how to use a listbox and a combobox
 * 
 * Georg Potthast 2018
 */
#define MWINCLUDECOLORS
#include <windows.h>
#include <string.h>

#define ID_LIST 7
#define ID_COMBO 10

LRESULT CALLBACK wproc(HWND,UINT,WPARAM,LPARAM);
void disp(HWND hwnd, char* s, int x, int y);

char * array[] = {
    "First entry     ",
    "Second entry    ",
    "Third entry     ",
    "Fourth entry    ",
    "Fifth entry     ",
    "Sixth entry     ",
};

#define n_array (sizeof (array) / sizeof (const char *))


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
        static char szAppName[]="ListCombo";
        HWND hwnd;
        MSG msg;
        WNDCLASS wndclass;

	int width, height;
	int i;
	RECT r;
        HWND hlist, hcombo;


	GetWindowRect(GetDesktopWindow(), &r);
	width = height = r.right / 4;
	height /= 2;

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
        hwnd=CreateWindowEx(0L, szAppName, "List and Combobox Example",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                          NULL, NULL, NULL, NULL);

	hlist = CreateWindowEx(0L, "LISTBOX", "OK",
		WS_VSCROLL|
		//WS_HSCROLL|
		WS_BORDER|
		WS_CHILD | 
		WS_VISIBLE | 
		LBS_NOTIFY,
		150, 55, 100, 54,
		hwnd, (HMENU)ID_LIST, NULL, NULL);

	hcombo = CreateWindowEx(0L, "COMBOBOX", "Combobox",
		 //CBS_SIMPLE | 	/* edit+list */
		 //CBS_DROPDOWNLIST | 	/* static+pop */
		 CBS_DROPDOWN | 	/* edit+pop */
		 WS_VSCROLL | WS_CHILD | WS_VISIBLE,
		 20, 55, 100, (18*3),
		 //10,10,100,100,
		 hwnd, (HMENU)ID_COMBO, NULL, NULL);

for (i = 0; i < n_array; i++) {	
#if 1  
	SendDlgItemMessage ( hwnd, ID_LIST,  LB_ADDSTRING, 0, (LPARAM)(LPSTR)array[i]);
	SendDlgItemMessage ( hwnd, ID_COMBO, CB_ADDSTRING, 0, (LPARAM)(LPSTR)array[i]);
#else
	SendMessage(hlist,  LB_ADDSTRING, 0, (LPARAM)(LPSTR)array[i]);
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)array[i]);
#endif	
}

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
        
        switch (iMsg) {
        case WM_CREATE:
                break;
        case WM_PAINT:
                hdc=BeginPaint(hwnd,&ps);
                EndPaint(hwnd,&ps);
                break;
	case WM_COMMAND:
	    //printf("ID (7=list,10=combo)::%d\n",LOWORD(wParam));
            if (LOWORD(wParam) == ID_LIST) {     
	        //printf("Listbox-command:%d\n",HIWORD(wParam)); //4+5 focus window
                if (HIWORD(wParam) == LBN_SELCHANGE) {                   
                    sel = SendDlgItemMessage ( hwnd, ID_LIST, LB_GETCURSEL, 1, 0 );
		    //printf ("Listbox-selection:%d: %s %d\n", sel, array[sel],strlen(array[sel])); //starts with 0
		    disp(hwnd, array[sel],150,30);
               }
	    }
            if (LOWORD(wParam) == ID_COMBO) {     
	        //printf("COMBOBOX-command:%d\n",HIWORD(wParam)); //4+5 focus window
                if (HIWORD(wParam) == CBN_SELCHANGE) {                   
                    sel = SendDlgItemMessage ( hwnd, ID_COMBO, CB_GETCURSEL, 0, 0 );
		    //printf ("Combobox-item selection:%d: %s\n", sel, array[sel]);
		} else if (HIWORD(wParam) == CBN_SELENDOK) { //drop down list closed now
		    sel = SendDlgItemMessage ( hwnd, ID_COMBO, CB_GETCURSEL, 0, 0 );
		    //printf ("Combobox-editselection:%d: %s\n", sel, array[sel]);
		    disp(hwnd, array[sel],20,30);
               }
	    }
	    break;
        case WM_DESTROY:
                PostQuitMessage(0);
                break;
        default:
                return DefWindowProc(hwnd,iMsg,wParam,lParam);
        }      
        return 0;
}

void disp(HWND hwnd, char* s, int x, int y)
	// display a string
	{  HFONT hfont, hOldfont;
	   HDC hdc;

	   hfont = (HFONT) GetStockObject(ANSI_VAR_FONT);    // obtain a standard font
	   hdc = GetDC(hwnd);                       // point to the "device context" for this window
	   hOldfont = (HFONT) SelectObject(hdc, hfont);      // select font into the device context
	   TextOut(hdc, x, y, s, strlen(s));
	   SelectObject(hdc, hOldfont);         // put the previous font back into dc
	   ReleaseDC(hwnd, hdc);                   // tidy up
	}   
