#include <windows.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawRectangles(HWND);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PWSTR lpCmdLine, int nCmdShow) {
    
    MSG  msg;
    WNDCLASSW wc = {0};

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = "Brush";
    wc.hInstance     = hInstance;
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.lpfnWndProc   = WndProc;
    wc.hCursor       = LoadCursor(0, IDC_ARROW);

    RegisterClassW(&wc);
    CreateWindowW(wc.lpszClassName, "Several Brushes",
          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
          100, 100, 220, 240, NULL, NULL, hInstance, NULL);

    while (GetMessage(&msg, NULL, 0, 0)) {
    
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

  return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
    WPARAM wParam, LPARAM lParam) {
    
    switch(msg) {
  
        case WM_PAINT:

            DrawRectangles(hwnd);	    
            break;

        case WM_DESTROY:

            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void DrawRectangles(HWND hwnd) {

    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hwnd, &ps);
    HPEN hPen = CreatePen(PS_NULL, 1, RGB(0, 0, 0));
    HPEN holdPen = SelectObject(hdc, hPen);

    HBRUSH hBrush1 = CreateSolidBrush(RGB(121, 90, 0));
    HBRUSH hBrush2 = CreateSolidBrush(RGB(240, 63, 19));
    HBRUSH hBrush3 = CreateSolidBrush(RGB(240, 210, 18));
    HBRUSH hBrush4 = CreateSolidBrush(RGB(9, 189, 21));

    HBRUSH holdBrush = SelectObject(hdc, hBrush1);

    Rectangle(hdc, 30, 30, 100, 100);
    SelectObject(hdc, hBrush2);
    Rectangle(hdc, 110, 30, 180, 100);
    SelectObject(hdc, hBrush3);
    Rectangle(hdc, 30, 110, 100, 180);
    SelectObject(hdc, hBrush4);
    Rectangle(hdc, 110, 110, 180, 180);

    SelectObject(hdc, holdPen);
    SelectObject(hdc, holdBrush);

    DeleteObject(hPen);
    DeleteObject(hBrush1);
    DeleteObject(hBrush2);
    DeleteObject(hBrush3);
    DeleteObject(hBrush4);

    EndPaint(hwnd, &ps);
}
