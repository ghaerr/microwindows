/*
  Copyright (C) Chris Johns (ccj@acm.org)

  Microwindows C++ Wrappers.

 */

#include <algorithm>
#include <stdarg.h>
#include <stdio.h>
#include <mwobjects.h>

namespace MicroWindowsObjects
{

  static Application *the_application = 0;

  WindowClass::WindowClass ()
  {
    wclass.style         = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
    wclass.lpfnWndProc   = 0;
    wclass.lpfnWndProc   = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    wclass.hInstance     = 0;
    wclass.hIcon         = 0;
    wclass.hCursor       = 0;
    wclass.hbrBackground = 0;
    wclass.lpszMenuName  = 0;
    wclass.lpszClassName = 0;
  }

  WindowClass::WindowClass (LPCSTR    lpszClassName,
                            UINT      style,
                            int       cbClsExtra,
                            int       cbWndExtra,
                            HINSTANCE hInstance,
                            HICON     hIcon,
                            HCURSOR   hCursor,
                            HBRUSH    hbrBackground,
                            LPCSTR    lpszMenuName)
  {
    wclass.style         = style;
    wclass.lpfnWndProc   = (WNDPROC) Window::WndProc;
    wclass.cbClsExtra    = cbClsExtra;
    wclass.cbWndExtra    = cbWndExtra + sizeof (Window *);
    wclass.hInstance     = hInstance;
    wclass.hIcon         = hIcon;
    wclass.hCursor       = hCursor;
    wclass.hbrBackground = hbrBackground;
    wclass.lpszMenuName  = lpszMenuName;
    wclass.lpszClassName = lpszClassName; 
  }

  ATOM WindowClass::register_class ()
  {
    return RegisterClass (&wclass);
  }

  Window::Window ()
    : hwnd (0)
  {
  }

  Window::Window (DWORD     dwExStyle,
                  LPCSTR    lpClassName,
                  LPCSTR    lpWindowName,
                  DWORD     dwStyle,
                  int       x, 
                  int       y,
                  int       nWidth,
                  int       nHeight,
                  HWND      hwndParent,
                  HMENU     hMenu,
                  HINSTANCE hInstance,
                  LPVOID    lpParam)
    : hwnd (0)
  {
    create (dwExStyle,
            lpClassName,
            lpWindowName,
            dwStyle,
            x, y, nWidth, nHeight,
            hwndParent,
            hMenu,
            hInstance,
            lpParam);
  }

  Window::~Window ()
  {
    destory ();
  }

  HWND
  Window::create (DWORD     dwExStyle,
                  LPCSTR    lpClassName,
                  LPCSTR    lpWindowName,
                  DWORD     dwStyle,
                  int       x, 
                  int       y,
                  int       nWidth,
                  int       nHeight,
                  HWND      hwndParent,
                  HMENU     hMenu,
                  HINSTANCE hInstance,
                  LPVOID    lpParam)
  {
    lpCreateParams = lpParam;
  
    lpParam = (LPVOID) this;
  
    hwnd = ::CreateWindowEx (dwExStyle, 
                             lpClassName,
                             lpWindowName,
                             dwStyle,
                             x, y, nWidth, nHeight,
                             hwndParent,
                             hMenu,
                             hInstance,
                             lpParam);
    return hwnd;
  }

  BOOL
  Window::destory ()
  {
    if (hwnd)
    {
      HWND old_hwnd = hwnd;
      hwnd = 0;
      return ::DestroyWindow (old_hwnd);
    }
    return 0;
  }

  bool
  Window::attach (const int fd, FileDescriptor& file_descriptor)
  {
    //
    // The user must set the fd before being added to the set.
    //

    if ((fd < 0) || (fd >= FD_SETSIZE) || (file_descriptor.file_desc != -1))
      return false;
  
    //
    // If this fd is already taken do not add another.
    //

    file_descriptor.file_desc = fd;
    file_descriptor.window    = this;

    if (file_descriptors.find (&file_descriptor) != file_descriptors.end ())
    {
      file_descriptor.file_desc = -1;
      file_descriptor.window    = 0;
      return false;
    }
  
    file_descriptors.insert (&file_descriptor);
  
    return true;
  }

  bool
  Window::detach (FileDescriptor& file_descriptor)
  {
    //
    // The user must set the fd before being added to the set.
    //

    if ((file_descriptor.fd () < 0) || (file_descriptor.fd () >= FD_SETSIZE))
      return false;
  
    //
    // If this fd is already taken do not add another.
    //

    if (file_descriptors.find (&file_descriptor) == file_descriptors.end ())
      return false;
  
    file_descriptor.disable_read ();
    file_descriptor.disable_write ();
    file_descriptor.disable_except ();
  
    file_descriptors.erase (&file_descriptor);

    file_descriptor.file_desc = -1;
    file_descriptor.window    = 0;

    return true;
  }

  struct eq_fd
  {
    const int fd;
    eq_fd (const int fd) : fd (fd) {}
    bool operator() (const FileDescriptor* f1) const 
      { return (f1->fd () == fd); }
  };
  
  LRESULT
  Window::message_handler (UINT   msg,
                           WPARAM wp,
                           LPARAM lp)
  {
    switch (msg)
    {
      case WM_FDINPUT:
      case WM_FDOUTPUT:
      case WM_FDEXCEPT:
      
      {
        //
        // The iterator provides a reference to the object pointer
        // as the set contains pointers.
        //

        set<FileDescriptor*>::iterator file_descriptor;
 
        file_descriptor = find_if (file_descriptors.begin (), 
                                   file_descriptors.end (), 
                                   eq_fd ((int) wp));

        if (file_descriptor != file_descriptors.end ())
        {
          switch (msg)
          {
            case WM_FDINPUT:
              return (*file_descriptor)->read ();
            
            case WM_FDOUTPUT:
              return (*file_descriptor)->write ();
            
            case WM_FDEXCEPT:
              return (*file_descriptor)->except ();
          }
        }
      }
      break;
      
      default:
        break;
    }
  
    return ::DefWindowProc (hwnd, msg, wp, lp);
  }

  bool
  Window::ltint::operator () (const FileDescriptor* f1, 
                              const FileDescriptor* f2) const
  {
    return f1->fd () < f2->fd ();
  }

  LRESULT CALLBACK 
  Window::WndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
  {
    int    offset  = GetClassLong (hwnd, GCL_CBWNDEXTRA) - sizeof (Window *);
    Window *window = 0;
  
    if (msg == WM_CREATE)
    {
      LPCREATESTRUCT cs = (LPCREATESTRUCT) lp;
    
      window = dynamic_cast<Window*>((Window*) cs->lpCreateParams);

      if (window)
      {
        window->hwnd = hwnd;
        SetWindowLong (hwnd, offset, (DWORD) window);
        cs->lpCreateParams = window->lpCreateParams;
      }
    }
    else
    {
      window = dynamic_cast<Window*>((Window*) GetWindowLong (hwnd, offset));
    }
    
    if (window)
      return window->message_handler (msg, wp, lp);
  
    return ::DefWindowProc (hwnd, msg, wp, lp);
  }

  Paint::Paint (HWND hwnd)
    : hwnd (hwnd),
      draw_3d (false),
      drawing (false),
      r (0, 0, 0, 0),
      text_format (0)
  {
  }

  Paint::Paint (HWND hwnd, LPARAM lpParam)
    : hwnd (hwnd),
      draw_3d (false),
      drawing (false),
      r (0, 0, 0, 0),
      text_format (0)
  {
    POINTSTOPOINT (pt, lpParam);  
  }

  Paint::~Paint ()
  {
    end ();
  }

  void
  Paint::begin (bool init_3d, bool draw_3d_in_mem)
  {
    if (!drawing && hwnd)
    {
      hdc = ::BeginPaint (hwnd, &ps);
    
      r.get_client (hwnd);
    
      if (init_3d)
        initialise_3d (draw_3d_in_mem);
    
      drawing = true;
    }
  }

  void
  Paint::end ()
  {
    if (drawing)
    {
      paint_3d ();

      ::EndPaint (hwnd, &ps);

      drawing = false;
    }
  }

  int
  Paint::text_out (int x, int y, const char *format, ...)
  {
    Rect    rect (x, y, x + 100, y + 100);
    va_list arg;
  
    va_start (arg, format);
    vsnprintf (format_buf, TEXT_BUF_SIZE, format, arg);
    format_buf[TEXT_BUF_SIZE - 1] = '\0';

    return ::DrawText (hdc, format_buf, -1, rect, text_format);
  }

  FileDescriptor::FileDescriptor ()
    : file_desc (-1),
      window (0)
  {
  }

  FileDescriptor::~FileDescriptor ()
  {
    if (window)
      window->detach (*this);
  }

  bool
  FileDescriptor::enable_read ()
  {
    if (!read_is_enabled && window && (file_desc != -1))
    {
      ::MwRegisterFdInput (*window, file_desc);
      read_is_enabled = true;
      return true;
    }
    return false;
  }

  bool
  FileDescriptor::disable_read ()
  {
    if (read_is_enabled && window && (file_desc != -1))
    {
      ::MwUnregisterFdInput (*window, file_desc);
      read_is_enabled = false;
      return true;
    }
    return false;
  }


  bool
  FileDescriptor::enable_write ()
  {
    if (!write_is_enabled && window && (file_desc != -1))
    {
      ::MwRegisterFdOutput (*window, file_desc);
      write_is_enabled = true;
      return true;
    }
    return false;
  }

  bool
  FileDescriptor::disable_write ()
  {
    if (write_is_enabled && window && (file_desc != -1))
    {
      ::MwUnregisterFdOutput (*window, file_desc);
      write_is_enabled = false;
      return true;
    }
    return false;
  }

  bool
  FileDescriptor::enable_except ()
  {
    if (!except_is_enabled && window && (file_desc != -1))
    {
      ::MwRegisterFdExcept (*window, file_desc);
      except_is_enabled = true;
      return true;
    }
    return false;
  }

  bool
  FileDescriptor::disable_except ()
  {
    if (except_is_enabled && window && (file_desc != -1))
    {
      ::MwUnregisterFdExcept (*window, file_desc);
      except_is_enabled = false;
      return true;
    }
    return false;
  }

  LRESULT
  FileDescriptor::read ()
  {
    return 0;
  }

  LRESULT
  FileDescriptor::write ()
  {
    return 0;
  }

  LRESULT
  FileDescriptor::except ()
  {
    return 0;
  }

  Application::Application ()
    : background (0)
  {
    if (!the_application)
      the_application = this;
  }

  Application::Application (MWIMAGEHDR& background)
    : background (&background)
  {
    if (!the_application)
      the_application = this;
  }

  Application::~Application ()
  {
  }

  int
  Application::initialise ()
  {
    return 0;
  }

  int
  Application::shutdown ()
  {
    return 0;
  }

  int WINAPI 
  Application::WinMain (HINSTANCE hInstance, 
                        HINSTANCE hPrevInstance, 
                        LPSTR     lpCmdLine,
                        int       nShowCmd)
  {
    if (the_application)
    {
      int result;
    
      MwRegisterButtonControl (0);
    
      result = the_application->initialise ();
    
      if (result)
        return result;
    
      //
      // Set background wallpaper
      //
    
      if (the_application->background)
        MwSetDesktopWallpaper (the_application->background);

      MSG msg;
    
      //
      // type ESC to quit...
      //
    
      while (GetMessage (&msg, 0, 0, 0)) 
      {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
      }

      result = the_application->shutdown ();
    
      if (result)
        return result;
    
      return 0;
    }
  
    return 1;
  }

};

//
//  Global Microwindows WinMain () routine with "C" linkage
//

extern "C"
{
  int WinMain (HINSTANCE hInstance, 
               HINSTANCE hPrevInstance, 
               LPSTR     lpCmdLine, 
               int       nShowCmd)
  {
    return MicroWindowsObjects::Application::WinMain (hInstance, 
                                                      hPrevInstance, 
                                                      lpCmdLine,  
                                                      nShowCmd);
  }
};
