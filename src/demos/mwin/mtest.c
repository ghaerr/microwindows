#include <windows.h>

LRESULT CALLBACK wproc(HWND,UINT,WPARAM,LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
        static char szAppName[]="HolaWin";
        HWND hwnd;
        MSG msg;
        WNDCLASS wndclass;

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
                          80,
                          80,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
               
               
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
        
        switch (iMsg) {
        case WM_CREATE:
                break;
        case WM_PAINT:
        /*case WM_MOUSEFIRST:*/
                hdc=BeginPaint(hwnd,&ps);
                GetClientRect(hwnd,&rect);
                DrawText(hdc, "Hola, NOS", -1, &rect,
                         DT_SINGLELINE|DT_CENTER|DT_VCENTER);
                EndPaint(hwnd,&ps);
                break;
        case WM_DESTROY:
                PostQuitMessage(0);
                break;
        default:
                return DefWindowProc(hwnd,iMsg,wParam,lParam);
        }      
        return (0);
}
