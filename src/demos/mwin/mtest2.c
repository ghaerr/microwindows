#define MWINCLUDECOLORS
#include <windows.h>

LRESULT CALLBACK wproc(HWND,UINT,WPARAM,LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
        static char szAppName[]="HolaWin";
        HWND hwnd;
        MSG msg;
        WNDCLASS wndclass;

	int width, height;
	RECT r;
        HWND hlist,hedit;

	GetWindowRect(GetDesktopWindow(), &r);
	width = height = r.right / 2;

	MwRegisterButtonControl(NULL);
	MwRegisterEditControl(NULL);
	MwRegisterListboxControl(NULL);
	MwRegisterProgressBarControl(NULL);
	MwRegisterStaticControl(NULL);
	MwRegisterComboboxControl(NULL);
	MwRegisterScrollbarControl(NULL);

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
        hwnd=CreateWindowEx(0L,
                          szAppName,
                          "Hola",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          width,	/* 80, */
                          height,	/* 80, */
                          NULL,
                          NULL,
                          NULL,
                          NULL);
#if !ELKS

	hedit=CreateWindowEx(0L, "EDIT",
		"OK",
		WS_BORDER|WS_CHILD | WS_VISIBLE,
		width * 5 / 8, 10, 100, 18,
		hwnd, (HMENU)5, NULL, NULL);

	CreateWindowEx(0L, "PROGBAR",
		"OK",
		WS_BORDER|WS_CHILD | WS_VISIBLE,
		width * 5 / 8, 32, 100, 18,
		hwnd, (HMENU)6, NULL, NULL);

	hlist = CreateWindowEx(0L, "LISTBOX",
		"OK",
		WS_VSCROLL|
		/*WS_HSCROLL|*/
		WS_BORDER|WS_CHILD | WS_VISIBLE,
		width * 5 / 8, 54, 100, 54,
		hwnd, (HMENU)7, NULL, NULL);

	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"Cherry");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"Apple");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"Orange");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"Banana");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"Smooth");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"00000");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"11111");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"22222");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"33333");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"44444");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"55555");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"66666");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"77777");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"88888");
	SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)(LPSTR)"99999");

	CreateWindowEx(0L, "BUTTON",
		"Cancel",
		WS_CHILD | WS_VISIBLE,
		width * 5 / 8 + 50, 106+16+6, 50, 14,
		hwnd, (HMENU)8, NULL, NULL);

	CreateWindowEx(0L, "STATIC",
		"Static",
		WS_CHILD | WS_VISIBLE,
		width * 5 / 8, 106+34+6, 100, 18,
		hwnd, (HMENU)9, NULL, NULL);
#if 0
	{
	HWND hcombo;

	hcombo = CreateWindowEx(0L, "COMBOBOX",
		 "Combobox",
#if 0
		 CBS_SIMPLE | 		/* edit+list */
#endif
#if 0
		 CBS_DROPDOWNLIST | 	/* static+pop */
#endif
#if 1
		 CBS_DROPDOWN | 	/* edit+pop */
#endif

		 WS_VSCROLL | WS_CHILD | WS_VISIBLE,
		 width * 5 / 8, 106+14+4+18+4, 100, (18*5),
		 hwnd, (HMENU)10, NULL, NULL);

	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Cherry");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Apple");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Orange");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Banana");
	}
#endif

#if 1
	CreateWindowEx(0L, "SCROLLBAR",
		"OK",
		SBS_VERT | 
		WS_CHILD | WS_VISIBLE ,
		width * 5 / 8 -(18*2), 106+68, 18, 128,
		hwnd, (HMENU)11, NULL, NULL);

	CreateWindowEx(0L, "SCROLLBAR",
		"OK",
		SBS_HORZ | 
		WS_CHILD | WS_VISIBLE ,
		width * 5 / 8 -18 , 106+68, 128, 18,
		hwnd, (HMENU)12, NULL, NULL);
#endif

#endif
               
        ShowWindow(hwnd,iCmdShow);
        UpdateWindow(hwnd);

        while (GetMessage(&msg,NULL,0,0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
        }      

        return msg.wParam;
}       
LRESULT CALLBACK wproc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{       
        HDC hdc;
        PAINTSTRUCT ps;
        RECT rect;
	HGDIOBJ oldfont;
        
        switch (iMsg) {
        case WM_CREATE:
                break;
        case WM_PAINT:
        /*case WM_MOUSEFIRST:*/
                hdc=BeginPaint(hwnd,&ps);

                /*GetClientRect(hwnd,&rc);*/
		/*Arc(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);*/
		/*Pie(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);*/

                GetClientRect(hwnd,&rect);
	        oldfont=SelectObject(hdc,CreateFont(12,
			0,0,0,0,0,0,0,0,0,0,0,
			FF_DONTCARE|DEFAULT_PITCH,
			"HZXFONT"));
                DrawText(hdc, "Hola, NOS, ¤¤¤å´ú¸Õ", -1, &rect,
                         DT_SINGLELINE|DT_CENTER|DT_VCENTER);
		DeleteObject(SelectObject(hdc,oldfont));

		/*for (i=0;i<100;i++)
			SetPixel(hdc,i,i,BLUE);*/

                EndPaint(hwnd,&ps);
                break;
        case WM_DESTROY:
                PostQuitMessage(0);
                break;
        default:
                return DefWindowProc(hwnd,iMsg,wParam,lParam);
        }      
        return 0;
}
