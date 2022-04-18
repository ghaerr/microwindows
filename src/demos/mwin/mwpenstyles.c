#include <windows.h>
#include <string.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawLines(HWND);

static void disp(HWND hwnd, char* s, int x, int y)
	// display a string
	{  HFONT hfont, hOldfont;
	   HDC hdc;

	   hfont = (HFONT) GetStockObject(ANSI_VAR_FONT);    // obtain a standard font
	   hdc = GetDC(hwnd);                       // point to the "device context" for this window
	   hOldfont = (HFONT) SelectObject(hdc, hfont);      // select font into the device context
	   if (hOldfont)  // if succesful
	   {
	      TextOut(hdc, x, y, s, strlen(s));
	      SelectObject(hdc, hOldfont);         // put the previous font back into dc
	   }
	   else MessageBox(hwnd, "disp could not select the font", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);

	   ReleaseDC(hwnd, hdc);                   // tidy up
	}   


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    MSG  msg;
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = "Pen styles";
    wc.hInstance     = hInstance;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpfnWndProc   = WndProc;
    wc.hCursor       = LoadCursor(0, IDC_ARROW);

    RegisterClass(&wc);
    CreateWindow(wc.lpszClassName, "Pen styles",
          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
          100, 100, 430, 190, NULL, NULL, hInstance, NULL);

#if !MULTIAPP
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
  return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    
    switch(msg) {
  
        case WM_PAINT:

            DrawLines(hwnd);
            break;

        case WM_DESTROY:

            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DrawLines(HWND hwnd) {

    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hwnd, &ps);
    HPEN hPen1 = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN hPen2 = CreatePen(PS_DASH, 1, RGB(0, 0, 0));
    HPEN hPen3 = CreatePen(PS_DOT, 1, RGB(0, 0, 0));
    HPEN hPen4 = CreatePen(PS_DASHDOT, 1, RGB(0, 0, 0));
    HPEN hPen5 = CreatePen(PS_DASHDOTDOT, 1, RGB(0, 0, 0));

    HPEN holdPen = SelectObject(hdc, hPen1);
    MoveToEx(hdc, 50, 30, NULL);
    LineTo(hdc, 300, 30);
    disp(hwnd, "solid", 310, 23);

    SelectObject(hdc, hPen2);
    MoveToEx(hdc, 50, 50, NULL);
    LineTo(hdc, 300, 50);
    disp(hwnd, "dash", 310, 43);

    SelectObject(hdc, hPen3);
    MoveToEx(hdc, 50, 70, NULL);
    LineTo(hdc, 300, 70);
    disp(hwnd, "dot", 310, 63);

    SelectObject(hdc, hPen4);
    MoveToEx(hdc, 50, 90, NULL);
    LineTo(hdc, 300, 90);
    disp(hwnd, "dashdot", 310, 83);

    SelectObject(hdc, hPen5);
    MoveToEx(hdc, 50, 110, NULL);
    LineTo(hdc, 300, 110);
    disp(hwnd, "dashdotdot", 310, 103);

    SelectObject(hdc, hPen1);
    MoveToEx(hdc, 50, 130, NULL);
    LineTo(hdc, 300, 130);
    disp(hwnd, "solid", 310, 123);
    
    SelectObject(hdc, holdPen);
    DeleteObject(hPen1);
    DeleteObject(hPen2);
    DeleteObject(hPen3);
    DeleteObject(hPen4);
    DeleteObject(hPen5);

    EndPaint(hwnd, &ps);  
}
