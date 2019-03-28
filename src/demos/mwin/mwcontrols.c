/* Demo showing for various controls in Microwindows. 
 *
 * Georg Potthast 2018 
 */

#include <windows.h>

	const char g_class_name[] = "myWindowClass";
	const int ID_STATIC = 1;
	const int ID_EDITBOX = 10;
	const int ID_BUTTON = 20;
	const int ID_CHECKBOX = 30;
	const int ID_DOT = 40;
	const int ID_LIST = 50;

	// declare the WndProc function (see later)
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	// declare myCreateBox
	HWND myCreateBox(HWND hwnd, int id, char* type, char* s, int x, int y, int width, int height);

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
	   
	   if (!RegisterClass(&wc))
	   {
	      MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
	      return 0;
	   }

	   hwnd = CreateWindowEx(
	      WS_EX_CLIENTEDGE,                         // what the border looks like
	      g_class_name,
	      "Assorted Controls",                 // text appearing in top bar
	      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	      CW_USEDEFAULT, CW_USEDEFAULT, 520, 620,   // window xpos, ypos, width, height
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
	    int i;

	  switch(msg)
	  {
	  case WM_CREATE:
	      myCreateBox(hwnd, -1,"frame", "", 1,9,82,82);
	      myCreateBox(hwnd, ID_STATIC,"static", "the quick brown fox", 2, 10, 80, 30);
	      myCreateBox(hwnd, ID_STATIC+1,"static", "Static Text2", 2, 50, 60, 20);
	      myCreateBox(hwnd, ID_STATIC+2,"static", "Static Text3", 2, 70, 60, 20);
	      for(i=0; i<6; ++i) {
		  myCreateBox(hwnd, -1,"frame", "", 1,100-1+30*i,82,19);
		  myCreateBox(hwnd, ID_EDITBOX+i, "edit1","hello", 2, 100+30*i, 80, 17); 
	      }
	      myCreateBox(hwnd, ID_BUTTON,"button", "Clear radio", 350,10,60,20);
	      myCreateBox(hwnd, ID_CHECKBOX,"check", "Toggle", 150,10,60,20);
	      myCreateBox(hwnd, ID_DOT,"radio", "Radio", 250,10,60,20);
			break;
	  case WM_PAINT:
              hdc=BeginPaint(hwnd,&ps);
              EndPaint(hwnd,&ps);
	      break; 
	  case WM_LBUTTONDOWN:
	   {  HWND h = GetDlgItem(hwnd, ID_STATIC); 
	      if (h) SetWindowText(h, "jumped over");
	      break; 
	   }
	  case WM_COMMAND:
	    if (LOWORD(wParam)==ID_BUTTON) 
              {MessageBox(hwnd, "You pushed the clear button", " ", MB_OK);
               SendDlgItemMessage(hwnd,ID_DOT,BM_SETCHECK,BST_UNCHECKED,0);
               SetDlgItemText(hwnd,ID_STATIC+2,"not set");
               SetDlgItemText(hwnd,ID_STATIC+2,"not set");
               return 0;
               break;
              } 
              else if (LOWORD(wParam)==ID_CHECKBOX)
              {UINT status = IsDlgButtonChecked(hwnd,ID_CHECKBOX);
               if (status) SetDlgItemText(hwnd,ID_STATIC+1,"checked");
               else SetDlgItemText(hwnd,ID_STATIC+1,"cleared");
               break;
              } 
              else if (LOWORD(wParam)==ID_DOT)
              {  UINT status = IsDlgButtonChecked(hwnd,ID_DOT);
               if (status) SetDlgItemText(hwnd,ID_STATIC+2,"set");
               else SetDlgItemText(hwnd,ID_STATIC+2,"not set");
               break;
              }
              break; 
	   case WM_KEYDOWN:
	      SetDlgItemText(hwnd, ID_STATIC, "the lazy dog");
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

HWND myCreateBox(HWND hwnd, int id, char* type, char* s, 
                 int x, int y, int width, int height)
// Creates a box control: frame, static box, edit box or list box
// hwnd is the window within which the box appears,
// id is a user-provided ID number,
// type= 'frame' or 'static' or 'edit' or 'list' (only the 1st character is used)
//       for edit boxes, include a 1 (e.g. 'edit1') to specify a single-line box
// s = string to be displayed (ignored for a frame)
// x,y = position on screen,
// width, height = width and height of the box.
{ HWND h;

  char typechar = type[0];
  DWORD styleex = 0;
  DWORD style = 0;
  char* classname = {0};
  
  switch ( typechar )
  { // first consider button, checkbox, radio button
    case 'b': case 'B':
       styleex = 0L;
       style = WS_CHILD | WS_VISIBLE; // | WS_TABSTOP | BS_PUSHBUTTON;
       // style BS_PUSHBUTTON a normal button, BS_DEFPUSHBUTTON for the default button
       classname = "BUTTON";
    break;
    case 'c': case 'C':
       styleex = WS_EX_STATICEDGE;
       style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX;
       // for BS_CHECKBOX the parent must set the check state
       classname = "BUTTON";
    break;
    case 'r': case 'R':
       styleex = WS_EX_STATICEDGE;
       style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON;
       // for BS_RADIOBUTTON the parent must set the check state
       classname = "BUTTON";
    break;

    // next consider frame and static text
    case 'f': case 'F':
       styleex = WS_EX_WINDOWEDGE;
       style = WS_CHILD | WS_VISIBLE | SS_BLACKFRAME;
       classname = "STATIC";
    break;
    case 's': case 'S':
       styleex = WS_EX_STATICEDGE;
       style = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX;
       classname = "STATIC";
    break;
    
    // next consider edit box and list box
    case 'e': case 'E':  
    {  styleex = WS_EX_CLIENTEDGE;
       style = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
       // now search for a '1' in type. If none found, then specify a multi-line
       // edit box. We don't use strchr() so that we don't need a #include.
       char* c = type;
       while( (*c != '1') && (*c != 0) ) ++c;
       if (*c==0) style |= ES_MULTILINE | ES_AUTOVSCROLL;
       // for scroll bars specify | WS_VSCROLL | WS_HSCROLL
       classname = "EDIT";
    }
    break;
    case 'l': case 'L': 
       styleex = WS_EX_CLIENTEDGE;
       style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY;
       classname = "LISTBOX";
    break;
    default:
          MessageBox(hwnd, "unrecognised box type in myCreateBox.", "Error", MB_OK | MB_ICONERROR);
  }     
  
  h = CreateWindowEx(styleex, classname, "", style, x,y,width,height,
            hwnd, (HMENU) (intptr_t)id, NULL, NULL);

  if (h == NULL)
  {   MessageBox(hwnd, "myCreateBox could not create box.", "Error", MB_OK | MB_ICONERROR);
      return h;
  }

  // else
  // set the font
  HFONT hfont;
  hfont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
  SendMessage(h, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(FALSE, 0));

  // set the text
  switch ( typechar )
  { 
    case 'f': case 'F': break;
    
    case 'b': case 'B': case 'c': case 'C': case 'r': case 'R':
    case 's': case 'S': case 'e': case 'E':
       SendMessage(h, WM_SETTEXT, 0, (LPARAM) s); break;
       
    case 'l': case 'L': 
       SendMessage(h, LB_ADDSTRING, 0, (LPARAM) s); break;
  }
  
  return h;
}

