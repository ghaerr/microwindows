/*Example of the progress bar widget
 * If you click on the button, you start
 * a timer and the progressbar shows how
 * many timer ticks were received
 * 
 * Georg Potthast 2018
 */
#define MWINCLUDECOLORS
#include <windows.h>
#include <unistd.h>
#include <string.h>

#define ID_BUTTON 1
#define ID_TIMER 2
#define ID_PROGBAR 3
#define ID_STATIC 4

HWND hprogbar;
HWND hbutton;

LRESULT CALLBACK wproc(HWND,UINT,WPARAM,LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
        static char szAppName[]="Helloprogbar";
        HWND hwnd;
        MSG msg;
        WNDCLASS wndclass;

        HWND hlist;
	int width, height;

	width=270;
	height=120;

	MwRegisterButtonControl(NULL);
	MwRegisterProgressBarControl(NULL);
	MwRegisterStaticControl(NULL);

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
        hwnd=CreateWindowEx(0L, szAppName, "Progressbar example",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                          NULL, NULL, NULL, NULL);

	hprogbar = CreateWindowEx(0L, "PROGBAR",
		"OK",
		WS_BORDER|WS_CHILD | WS_VISIBLE,
		30, 20, 190, 25,
		hwnd, (HMENU)ID_PROGBAR, NULL, NULL);

	hbutton = CreateWindowEx(0L, "BUTTON",
		"Start",
		WS_CHILD | WS_VISIBLE,
		85, 60, 85, 25,
		hwnd, (HMENU)ID_BUTTON, NULL, NULL);

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
	HFONT hfont, hOldfont;
	static int i;
	char percent[2];
        
        switch (iMsg) {
        case WM_CREATE:
            break;
        case WM_PAINT:
	    SendMessage(hprogbar, PBM_SETRANGE, 0, 300); //lower numbers will display blocked bar
	    SendMessage(hprogbar, PBM_SETSTEP, 1, 0);
	    SendMessage(hprogbar, PBM_SETPOS, 0, 0);
	    hdc=BeginPaint(hwnd,&ps);
            EndPaint(hwnd,&ps);
	    hfont = (HFONT) GetStockObject(ANSI_VAR_FONT);    // obtain a standard font
	    hdc = GetDC(hwnd);                       
	    hOldfont = (HFONT) SelectObject(hdc, hfont);      // select font into the device context
	    break;
        case WM_TIMER:
            SendMessage(hprogbar, PBM_STEPIT, 0, 0);
            i++;
	    //printf("%d,",i); fflush(stdout);
	    sprintf(percent,"%d,",i/3);

	    hdc=BeginPaint(hwnd,&ps);
	    rect.left=380; rect.top=50; rect.right=100; rect.bottom=20;
            DrawText(hdc, percent, -1, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
            EndPaint(hwnd,&ps);
	    
            if (i > 300) {
                KillTimer(hwnd, ID_TIMER);
                SendMessage(hbutton, WM_SETTEXT, (WPARAM) NULL, (LPARAM) "Start");
                i = 0;
            }

            break;
        case WM_COMMAND:
          
            if (i == 0) {  
                i = 1;
                SendMessage(hprogbar, PBM_SETPOS, 0, 0);
                SetTimer(hwnd, ID_TIMER, 5, NULL);
                //SendMessage(hbutton, WM_SETTEXT, (WPARAM) NULL, (LPARAM) "In progress");
		SetWindowTextW(hbutton, "In progress");
            }

          break;
        case WM_DESTROY:
	    KillTimer(hwnd, ID_TIMER);
	    DeleteObject(SelectObject(hdc,hOldfont));
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd,iMsg,wParam,lParam);
        }      
        return 0;
}
