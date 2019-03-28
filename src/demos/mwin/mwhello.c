/* Demo for displaying text in a window in Microwindows. 
 * First, the program displays "Hello world". Second, if you click the window, it pops up a 
 * message box saying "Hello again". 
 *
 * Georg Potthast 2018 
 */
#include <windows.h>
#include <string.h>

	const char g_class_name[] = "myWindowClass";

	// declare the WndProc function (see later)
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static void disp(HWND hwnd, HDC hdc, char* s)
	// display a string
	{  HFONT hfont, hOldfont;

	   hfont = (HFONT) GetStockObject(ANSI_VAR_FONT);    // obtain a standard font
	   hOldfont = (HFONT) SelectObject(hdc, hfont);      // select font into the device context
	   if (hOldfont)  // if succesful
	   {
	      TextOut(hdc, 10, 50, s, strlen(s));
	      SelectObject(hdc, hOldfont);         // put the previous font back into dc
	   }
	   else MessageBox(hwnd, "disp could not select the font", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
	}   

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{   
	   HDC hdc;
	   PAINTSTRUCT ps;
	  
	   switch(msg)
	   {
	   case WM_SIZE:  
	   case WM_PAINT:
	     hdc=BeginPaint(hwnd,&ps);
	     disp(hwnd, hdc, "Hello world");
	     EndPaint(hwnd,&ps);
	     break; 
	   case WM_LBUTTONDOWN:
	     MessageBox(hwnd, "Hello again", "Box title", MB_OK);
	     break; 
	   case WM_CLOSE:
	      DestroyWindow(hwnd);
	     break; 
	   case WM_DESTROY:
	      PostQuitMessage(0);
	      break;
	   default:
	      break;
	   }
	   return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	// here is the main function:
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
	      "Using TextOut",                 // text appearing in top bar
	      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	      CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,   // window xpos, ypos, width, height
	      NULL, NULL, hInstance, NULL);

	   if (hwnd == NULL)
	   {
	      MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
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

