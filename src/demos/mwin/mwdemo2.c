#define MWINCLUDECOLORS
#include <windows.h>

LRESULT CALLBACK windowproc(HWND,UINT,WPARAM,LPARAM);

#if EMSCRIPTEN && MULTIAPP
#define WinMain	mwdemo2_WinMain
#endif

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;
	int width, height;
	RECT r;
	HWND hlist,hcombo;
	static char szMainWindowClass[] = "main";

	/* register builtin controls and dialog classes*/
	MwInitializeDialogs(hInstance);

	/* calc main window size as half total area*/
	GetWindowRect(GetDesktopWindow(), &r);
	width = height = r.right / 2;

	/* register our main window class*/
	wndclass.style          = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc    = (WNDPROC)windowproc;
	wndclass.cbClsExtra     =0;
	wndclass.cbWndExtra     =0;
	wndclass.hInstance      =0;
	wndclass.hIcon          =0;
	wndclass.hCursor        =0;
	wndclass.hbrBackground  =(HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wndclass.lpszMenuName   =NULL;
	wndclass.lpszClassName  = szMainWindowClass;
	RegisterClass(&wndclass);

	/* create main window*/
	hwnd = CreateWindowEx(0L, szMainWindowClass,
		"Hello",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		NULL,
		NULL);

	CreateWindowEx(0L, "EDIT",
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
		//WS_HSCROLL|
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
		"Static text",
		WS_CHILD | WS_VISIBLE,
		width * 5 / 8, 106+34+6, 100, 18,
		hwnd, (HMENU)9, NULL, NULL);

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


	hcombo = CreateWindowEx(0L, "COMBOBOX",
 		"Combobox",
		 //CBS_SIMPLE | 				/* edit+list */
		 //CBS_DROPDOWNLIST | 			/* static+pop */
		 CBS_DROPDOWN | 				/* edit+pop */
		 WS_VSCROLL | WS_CHILD | WS_VISIBLE,
		 width * 5 / 8, 106+14+4+18+4+68, 100, (18*5),
 		hwnd, (HMENU)10, NULL, NULL);

	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Cherry");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Apple");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Orange");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Banana");

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

/* main window callback procedure*/
LRESULT CALLBACK windowproc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{       
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rc;
	int i;
	//HGDIOBJ oldfont;

	switch (iMsg) {
	case WM_CREATE:
		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd,&ps);
		GetClientRect(hwnd,&rc);

		/* draw on window background*/
		Arc(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);
		Pie(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);
		for (i=0;i<100;i++)
			SetPixel(hdc,i,i,BLUE);

		/* select chinese HZXFONT and draw text*/
		//oldfont = SelectObject(hdc,CreateFont(12, 0,0,0,0,0,0,0,0,0,0,0, FF_DONTCARE|DEFAULT_PITCH, "HZXFONT"));
		DrawText(hdc, " Hello World ", -1, &rc, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
		//DeleteObject(SelectObject(hdc,oldfont));

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
