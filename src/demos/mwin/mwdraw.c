/* Demo for simple graphics features. Draws a line, rectangles and ellipses. Modifies the brush to 
 * change the background color.
 *
 * Georg Potthast 2018 
 */
#define MWINCLUDECOLORS
#include <windows.h>

	const char g_class_name[] = "myWindowClass";

	// declare the WndProc function (see later)
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	int EZplot(HWND hwnd, int x, int y);

	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	   WNDCLASS wc;
	   HWND hwnd;
	   MSG Msg;
	   
	   MwInitializeDialogs ( hInstance ); //enable MessageBox dialog

	   //First we create a structure describing the window
	   wc.style         = 0;
	   wc.lpfnWndProc   = WndProc;       // N.B. here we specify the name of our function
	   wc.cbClsExtra    = 0;
	   wc.cbWndExtra    = 0;
	   wc.hInstance     = hInstance;
	   wc.hIcon         = 0; 
	   wc.hCursor       = 0; 
	   wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	   wc.lpszMenuName  = NULL;
	   wc.lpszClassName = g_class_name;
	   
	   if (!RegisterClass(&wc))
	   {
	      MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
	      return 0;
	   }

	   hwnd = CreateWindowEx(
	      WS_EX_CLIENTEDGE,                         // what the border looks like
	      g_class_name,
	      "Simple Graphics Routines",                 // text appearing in top bar
	      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	      CW_USEDEFAULT, CW_USEDEFAULT, 540, 620,   // window xpos, ypos, width, height
	      NULL, NULL, hInstance, NULL);

	   if (hwnd == NULL)
	   {
	      MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
	      return 0;
	   }

	   ShowWindow(hwnd, nCmdShow);
	   UpdateWindow(hwnd);

#if !MULTIAPP
	   while (GetMessage(&Msg, NULL, 0, 0)) {
	      TranslateMessage(&Msg);
	      DispatchMessage(&Msg);
	   }
#endif
	   return 0;
	}

	
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
	  HDC hdc;
	  PAINTSTRUCT ps;
	  RECT rc;
	  int i;

	  switch(msg)
	  {
	  case WM_PAINT:
	     hdc=BeginPaint(hwnd,&ps);
	     GetClientRect(hwnd,&rc);
		EZplot(hwnd, 1, 1);
		Arc(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, 0,0, 0,0);
		Pie(hdc, 150, 150, rc.right-rc.left-150, rc.bottom-rc.top-150, 0,0, 0,0);	
		for (i=0;i<400;i++) SetPixel(hdc,i,i,BLUE);
	     EndPaint(hwnd,&ps);
	     break;   
	  case WM_CLOSE:
	     DestroyWindow(hwnd);
	     return 0;
	  case WM_DESTROY:
	     PostQuitMessage(0);
	     return 0;
	  default:
	     break;
	  }
	  return DefWindowProc(hwnd, msg, wParam, lParam);
}

int EZplot(HWND hwnd, int x, int y)
{
   RECT rect;
   GetClientRect(hwnd, &rect);
   HDC hdc = GetDC(hwnd);
   if (hdc)
   {
      HPEN pen = CreatePen( PS_SOLID, 0, RGB(0,0,0) );
      HPEN prevpen = (HPEN) SelectObject(hdc, pen);

      HBRUSH brush = CreateSolidBrush(LTGRAY);
      HPEN prevbrush = (HPEN) SelectObject(hdc, brush);

      // draw rectangle with current pen, and fill it with current brush
      Rectangle(hdc, rect.left,rect.top,rect.right,rect.bottom);

      // Now reduce rect a bit
      LONG tmp = rect.left;
      rect.left = rect.left*19/20 + rect.right/20;
      rect.right = tmp/20 + rect.right*19/20;
      tmp = rect.bottom;
      rect.bottom = rect.bottom*19/20 + rect.top/20;
      rect.top = tmp/20 + rect.top*19/20;

      // Now create a white brush
      HBRUSH wbrush = CreateSolidBrush( RGB(255,255,255) );

      // paint the plotting region using the brush
      Rectangle(hdc, rect.left-1,rect.top-1,rect.right+1,rect.bottom+1);

      // Tidy up
      SelectObject(hdc, prevpen);
      SelectObject(hdc, prevbrush);
      ReleaseDC(hwnd, hdc);
      DeleteObject(pen);
      DeleteObject(brush);
      DeleteObject(wbrush);
   }
   return 0;
}
