/* Demo for static text. It features a simple control, called a static text box. This is simply a box on the screen 
 * which displays a line a text.
 * When you click the window, the message changes to "jumped over", and when you press a key it changes to "the lazy dog". 
 * Georg Potthast 2018 
 */
#include <windows.h>

	const char g_class_name[] = "myWindowClass";
	const int ID_STATIC = 7;

	// declare the WndProc function (see later)
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	// declare myCreateStatic
	HWND myCreateStatic(HWND hwnd, int id, char* s, int x, int y, int width, int height);

	// here is the main function:
	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	   WNDCLASS wc;
	   HWND hwnd;
	   MSG Msg;
	
		MwInitializeDialogs(hInstance); // register stand control and dialog classes
	   
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
	   
	   // Now we call RegisterClassEx. This is called "registering the window class".
	   // It tells the operating system we are here and want a window.
	   if (!RegisterClass(&wc))
	   {
	      MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
	      return 0;
	   }

	   // Ok, the operating system says we can have a window. Now we create it using CreateWindowEx.
	   // The parameters here mainly define the appearance of the window: what the border looks like,
	   // the title if any, etc.
	   hwnd = CreateWindowEx(
	      WS_EX_CLIENTEDGE,                         // what the border looks like
	      g_class_name,
	      "Example of a static conrol",                 // text appearing in top bar
	      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	      CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,   // window xpos, ypos, width, height
	      NULL, NULL, hInstance, NULL);

	   if (hwnd == NULL)
	   {
	      MessageBox(NULL, "Window Creation Failed!", "Error!", MB_OK);
	      return 0;
	   }
	   
	   ShowWindow(hwnd, nCmdShow);
	   UpdateWindow(hwnd);
	   
	   /* And finally: the Message Loop. This while loop is perpetually going round and round
	      at the "base" of your program, picking up messages from the keyboard, mouse and other
	      devices, and calling the WndProc function (i.e. the function specified above when we set
	      wc.lpfnWndProc).
	   */  

#if !MULTIAPP
	   while (GetMessage(&Msg, NULL, 0, 0)) {
	      TranslateMessage(&Msg);
	      DispatchMessage(&Msg);
	   }
#endif
	   return 0;
	}

	
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	// This is the function repeatedly called by WinMain. It receives and reacts to messages.
	// hwnd is the window,
	// msg is the message, with further parameters wParam, lParam
	{
	    HDC hdc;
	    PAINTSTRUCT ps;

	  switch(msg)
	  {
	  case WM_CREATE:
	  case WM_PAINT:
	      myCreateStatic(hwnd, ID_STATIC, "the quick brown fox", 2, 20, 80, 30);
	      //get white background
              hdc=BeginPaint(hwnd,&ps);
              EndPaint(hwnd,&ps);
	      break; //return 0;
	  case WM_LBUTTONDOWN:
	   {  HWND h = GetDlgItem(hwnd, ID_STATIC); 
	      if (h) SetWindowText(h, "jumped over");
	      break; //return 0;
	   }
	   case WM_KEYDOWN:
	      SetDlgItemText(hwnd, ID_STATIC, "the lazy dog");
	      break; //return 0;
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

HWND myCreateStatic(HWND hwnd, int id, char* s, int x, int y, int width, int height)
// Creates a static text box.
// hwnd is the window within which the box appears,
// id is a user-provided ID number,
// x,y = position on screen,
// width, height = width and height of the box.
{  HWND h;

   h = CreateWindowEx(WS_EX_STATICEDGE, "STATIC", 
       s,
       WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
       x,y,width,height,
       hwnd, 
       (HMENU) (intptr_t) ID_STATIC, 
	NULL, NULL);

   if (h) // successful creation
   {  HFONT hfDefault;
      hfDefault = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
      SendMessage(h, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
   }
   else
      MessageBox(hwnd, "MyCreateStatic could not create static text.", 
                 "Error", MB_OK | MB_ICONERROR);

   return h;
}
