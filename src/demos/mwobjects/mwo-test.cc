/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Demo program for Micro-Windows
 */

#define MWINCLUDECOLORS
#include <mwobjects.h>
#include <iostream>

extern MWIMAGEHDR image_microwin;
extern MWIMAGEHDR image_zion208;

PMWIMAGEHDR image  = &image_zion208;

using namespace MicroWindowsObjects;

class TestWindowClass
  : public WindowClass
{
public:
  
  TestWindowClass (LPCSTR lpszClassName);
    
};

class TestChildWindow
  : public Window
{
public:

  TestChildWindow ();
  
  HWND create (LPCSTR lpszClassName,
               HWND parent, 
               int x, int y, 
               int nWidth, int nHeight);
  
  void set_trace (bool t) 
    { trace = t; }
  
protected:

  virtual LRESULT message_handler (UINT   msg,
                                   WPARAM wParam,
                                   LPARAM lParam);
private:
  
  bool trace;
  
};

class TestWindow
  : public Window
{
public:

  TestWindow (LPCSTR lpszClassName, bool trace = false);
  
private:
  
  bool            trace;
  Window          button;
  TestChildWindow image[3];
  
};

class Test3dWindow
  : public Window
{
public:

  Test3dWindow (LPCSTR lpszChild);

protected:

  virtual LRESULT message_handler (UINT   msg,
                                   WPARAM wParam,
                                   LPARAM lParam);
private:

  vec1 gx;
  vec1 gy;
  
  vec1 last_gx;
  vec1 last_gy;
  
};

class TestRoseWindow
  : public TestWindow
{
public:

  TestRoseWindow (LPCSTR lpszClassName, bool trace = false);
  
protected:

  virtual LRESULT message_handler (UINT   msg,
                                   WPARAM wParam,
                                   LPARAM lParam);
};

class TestCircleWindow
  : public TestWindow
{
public:

  TestCircleWindow (LPCSTR lpszClassName, bool trace = false);
  
protected:

  virtual LRESULT message_handler (UINT   msg,
                                   WPARAM wParam,
                                   LPARAM lParam);
};

class TestDaisyWindow
  : public TestWindow
{
public:

  TestDaisyWindow (LPCSTR lpszClass, bool trace = false);
  
protected:

  virtual LRESULT message_handler (UINT   msg,
                                   WPARAM wParam,
                                   LPARAM lParam);
};

class TestFileDescriptor
  : public FileDescriptor
{
public:
  
  void do_fd_test ();
  
protected:

  LRESULT read ();
  LRESULT write ();
  LRESULT except ();
};

class TestApplication
  : public Application
{
  enum { GROUPS = 2 };
  
public:

  TestApplication ();
  
protected:

  virtual int initialise ();
  virtual int shutdown ();
  
private:

  TestWindowClass    main_class;

  Test3dWindow       *t3d [GROUPS];
  TestRoseWindow     *tr [GROUPS];
  TestCircleWindow   *tc [GROUPS];
  TestDaisyWindow    *td [GROUPS];

  TestFileDescriptor *fd;
  
};

TestWindowClass::TestWindowClass (LPCSTR lpszClassName)
  : WindowClass (lpszClassName,
                 CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW)
{
  set_background ((HBRUSH) GetStockObject (LTGRAY_BRUSH));
}

TestChildWindow::TestChildWindow ()
{
}

HWND
TestChildWindow::create (LPCSTR lpszClassName,
                         HWND parent, 
                         int x, int y, 
                         int nWidth, int nHeight)
{
  return Window::create (0,
                         lpszClassName, 
                         "", 
                         WS_BORDER | WS_CHILD | WS_VISIBLE,
                         x, y, nWidth, nHeight,
                         parent, 0, 0, 0);
}

LRESULT
TestChildWindow::message_handler (UINT   msg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  Paint paint (*this);

  switch (msg) 
  {
    case WM_PAINT:
      paint.begin ();

      DrawDIB (paint, paint.left (), paint.top (), image);

      paint.end ();
      break;

    default:
      return Window::message_handler (msg, wParam, lParam);
  }
  return 0;
}

TestWindow::TestWindow (LPCSTR lpszClassName, bool trace)
  : trace (trace)
{
  Rect rc (GetDesktopWindow ());
  int  width;
  int  height;
 
  width = height = rc.right () / 2;
  
  if (trace)
    cout << "create: hwnd=" << get_handle ()
         << " " << rc << endl;

  create (0, lpszClassName, "Micro C++ Application",
          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
          CW_USEDEFAULT, CW_USEDEFAULT,
          width, height,
          0, 0, 0, 0);
  
  button.create (0, "BUTTON", "Ok",
                 WS_CHILD | WS_VISIBLE,
                 width * 5 / 8, 10, 50, 14,
                 *this, 0, 0, 0);

  image[0].create (lpszClassName, *this, 
                   4, 4, 
                   width / 3, height / 3);
  image[1].create (lpszClassName, *this, 
                   width / 3, height / 3, 
                   width / 3, height / 3);
  image[2].create (lpszClassName, *this, 
                   width * 3 / 5, height * 3 / 5, 
                   width * 2 / 3, height * 2 / 3);

  image[0].set_trace (trace);
  image[1].set_trace (trace);
  image[2].set_trace (trace);

}

Test3dWindow::Test3dWindow (LPCSTR lpszClassName)
  : gx (0),
    gy (0),
    last_gx (0),
    last_gy (0)
{
  Rect rect (GetDesktopWindow ());
  int  width;
  int  height;
 
  width = height = rect.right () / 2;
  
  create (0, lpszClassName, "Micro C++ Application",
          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
          CW_USEDEFAULT, CW_USEDEFAULT,
          width, height,
          0, 0, 0, 0);
}

LRESULT
Test3dWindow::message_handler (UINT   msg,
                               WPARAM wParam,
                               LPARAM lParam)
{
  Paint paint (*this, lParam);
  Rect  rc;
 
  switch (msg) 
  {
    case WM_PAINT:
      paint.begin (true);

      look3 (-2 * gx, -2 * gy, 1.2);
      drawgrid (-8.0, 8.0, 10, -8.0, 8.0, 10);

      last_gx = gx;
      last_gy = gy;
      
      paint.end ();
      break;

    case WM_MOUSEMOVE:

      rc.get_client (*this);
      gx = (vec1) paint.get_point_x () / rc.right ();
      gy = (vec1) paint.get_point_y () / rc.bottom ();

      if (gx > last_gx || gy > last_gy)
        invalidate_rect (0, FALSE);
      break;

    default:
      return Window::message_handler (msg, wParam, lParam);
  }
  return 0;
}

TestRoseWindow::TestRoseWindow (LPCSTR lpszClassName, bool trace)
  : TestWindow (lpszClassName, trace)
{
}

LRESULT
TestRoseWindow::message_handler (UINT   msg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
  Paint paint (*this);
  
  switch (msg) 
  {
    case WM_PAINT:
      paint.begin (true);
      
      rose (1.0, 7, 13);

      paint.end ();
      break;

    default:
      return Window::message_handler (msg, wParam, lParam);
  }
  return 0;
}

TestCircleWindow::TestCircleWindow (LPCSTR lpszClassName, bool trace)
  : TestWindow (lpszClassName, trace)
{
}

LRESULT
TestCircleWindow::message_handler (UINT   msg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
  Paint paint (*this);
  
  switch (msg) 
  {
    case WM_PAINT:
      paint.begin (true);
      
      setcolor3 (BLACK);
      circle3 (1.0);

      paint.end ();
      break;

    default:
      return Window::message_handler (msg, wParam, lParam);
  }
  return 0;
}

TestDaisyWindow::TestDaisyWindow (LPCSTR lpszClassName, bool trace)
  : TestWindow (lpszClassName, trace)
{
}

LRESULT
TestDaisyWindow::message_handler (UINT   msg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  Paint paint (*this);
  
  switch (msg) 
  {
    case WM_PAINT:
      paint.begin (true);
      
      setcolor3 (BLUE);
      daisy (1.0, 20);

      paint.paint_3d ();
      
      paint.text_out (10, 250, "Date built : %s", __DATE__);
      
      paint.end ();
      break;
      
    case WM_LBUTTONDOWN:
      cout << "left down : " << *this << endl;
      SendMessage (*this, WM_FDINPUT, 200, 0);
      return Window::message_handler (msg, wParam, lParam);
      break;
      
    case WM_RBUTTONDOWN:
      cout << "right down : " << *this << endl;
      SendMessage (*this, WM_FDOUTPUT, 200, 0);
      return Window::message_handler (msg, wParam, lParam);
      break;
      
    case WM_LBUTTONDBLCLK:
      cout << "double left : " << *this << endl;
      SendMessage (*this, WM_FDEXCEPT, 200, 0);
      return Window::message_handler (msg, wParam, lParam);
      break;
      
    default:
      return Window::message_handler (msg, wParam, lParam);
  }
  return 0;
}

void 
TestFileDescriptor::do_fd_test ()
{
}

LRESULT
TestFileDescriptor::read ()
{
  cout << "test read fd for `" << *get_window () << "' and fd " << *this << endl;
  return 0;  
}

LRESULT
TestFileDescriptor::write ()
{
  cout << "test write fd for `" << *get_window () << "' and fd " << *this << endl;
  return 0;  
}

LRESULT
TestFileDescriptor::except ()
{
  cout << "test except fd for `" << *get_window () << "' and fd " << *this << endl;
  return 0;  
}

TestApplication::TestApplication ()
  : Application (image_microwin),
    main_class ("test")
{
  for (int i = 0; i < GROUPS; i++)
  {
    t3d [i] = 0;
    tr [i] = 0;
    tc [i] = 0;
    td [i] = 0;
  }
}

int
TestApplication::initialise ()
{
  main_class.register_class ();

  for (int i = 0; i < GROUPS; i++)
  {
    t3d [i] = new Test3dWindow ("test");
    tr [i] = new TestRoseWindow ("test");
    tc [i] = new TestCircleWindow ("test");
    td [i] = new TestDaisyWindow ("test");
  }

  fd = new TestFileDescriptor ();
  
  cout << "attach to " << *td[GROUPS - 1] << endl;
  
  td[GROUPS - 1]->attach (200, *fd);
  
  return 0;
}

int
TestApplication::shutdown ()
{
  for (int i = 0; i < GROUPS; i++)
  {
    if (t3d [i])
      delete t3d [i];
    t3d [i] = 0;
    
    if (tr [i])
      delete tr [i];
    tr [i] = 0;
    
    if (tc [i])
      delete tc [i];
    tc [i] = 0;
    
    if (td [i])
      delete td [i];
    td [i] = 0;
  }
  
  return 0;
}

TestApplication test_application;
