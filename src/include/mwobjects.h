/*
  Copyright (C) Chris Johns (ccj@acm.org)

  MicroWindows C++ Wrappers.

 */

#if !defined (_MWOBJECTS_H_)
#define _MWOBJECTS_H_

extern "C" 
{  
#include "windows.h"
#include "wintern.h"
#include "graph3d.h"
};

#include <set>
#include <iostream>

namespace MicroWindowsObjects
{
  class FileDescriptor;

  //
  // Manage Window Classes. Notice you do not need to specify
  // a Window Procedure function pointer. This is handled for 
  // you.
  //

  class WindowClass
  {
  public:
    
    WindowClass ();
    WindowClass (LPCSTR    lpszClassName,
                 UINT      style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW,
                 int       cbClsExtra = 0,
                 int       cbWndExtra = 0,
                 HINSTANCE hInstance = 0,
                 HICON     hIcon = 0,
                 HCURSOR   hCursor = 0,
                 HBRUSH    hbrBackground = 0,
                 LPCSTR    lpszMenuName = 0);
    
    void set_style (UINT style)
      { wclass.style = style; }

    void set_class_extra (int cbClsExtra)
      { wclass.cbClsExtra = cbClsExtra; }

    void set_window_extra (int cbWndExtra)
      { wclass.cbWndExtra = cbWndExtra; }

    void set_instance (HINSTANCE hInstance)
      { wclass.hInstance = hInstance; }

    void set_icon (HICON hIcon)
      { wclass.hIcon = hIcon; }

    void set_cursor (HCURSOR hCursor)
      { wclass.hCursor = hCursor; }

    void set_background (HBRUSH hbrBackground)
      { wclass.hbrBackground = hbrBackground; }

    void set_menu_name (LPCSTR lpszMenuName)
      { wclass.lpszMenuName = lpszMenuName; }
    
    ATOM register_class ();
    
  private:

    //
    // This variable is a local copy which is 
    // registered. After that is class does little.
    //

    WNDCLASS wclass;
  };
  
  class Window
  {
    friend WindowClass;
    
  public:

    Window ();
    Window (DWORD     dwExStyle,
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
            LPVOID    lpParam);

    virtual ~Window ();
    
    HWND create (DWORD     dwExStyle,
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
                 LPVOID    lpParam);

    BOOL destory ();

    HWND get_handle () const
      { return hwnd; }
    operator HWND () const
      { return get_handle (); }
        
    BOOL is_visible ()
      { return IsWindowVisible (hwnd); }
    BOOL show (int nCmdShow)
      { return ::ShowWindow (hwnd, nCmdShow); }
    BOOL update ()
      { return ::UpdateWindow (hwnd); }
    BOOL invalidate_rect (CONST RECT *lpRect, BOOL bErase)
      { return ::InvalidateRect (hwnd, lpRect, bErase); }

    HWND get_focus ()
      { return ::GetFocus (); }
    HWND set_focus ()
      { return ::SetFocus (hwnd); }

    BOOL move (int x, int y, int nWidth, int nHeight, BOOL bRepaint)
      { return MoveWindow (hwnd, x, y, nWidth, nHeight, bRepaint); }

    // FIXME: Should these be here ?
    BOOL client_to_screen (LPPOINT lpPoint)
      { return ::ClientToScreen (hwnd, lpPoint); }
    BOOL screen_to_client (LPPOINT lpPoint)
      { return ::ClientToScreen (hwnd, lpPoint); }

    LONG get_long (int nIndex)
      { return ::GetWindowLong (hwnd, nIndex); }
    LONG set_long (int nIndex, LONG wNewLong)
      { return ::SetWindowWord (hwnd, nIndex, wNewLong); }
    WORD get_word (int nIndex)
      { return ::GetWindowWord (hwnd, nIndex); }
    WORD set_word (int nIndex, WORD wNewWord)
      { return ::SetWindowWord (hwnd, nIndex, wNewWord); }

    DWORD get_class_long (int nIndex)
      { return ::GetClassLong (hwnd, nIndex); }

    int get_text (LPSTR lpString, int nMaxCount)
      { return GetWindowText (hwnd, lpString, nMaxCount); }
    BOOL set_text (LPCSTR lpString)
      { return SetWindowText (hwnd, lpString); }

    //
    // File Descriptor handlers.
    //

    virtual bool attach (const int fd, FileDescriptor& file_descriptor);
    virtual bool detach (FileDescriptor& file_descriptor);

  protected:

    //
    // The message handler. Insure you call this class's default
    // handler for default message handling so any special
    // filtering can occur.
    //

    virtual LRESULT message_handler (UINT   msg,
                                     WPARAM wParam,
                                     LPARAM lParam);

  private:

    //
    // We only need one WndProc.
    //

    static LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND   hwnd;
    LPVOID lpCreateParams;
    
    //
    // Set of FileDescriptor objects
    //

    struct ltint 
    {
      bool operator() (const FileDescriptor* f1, const FileDescriptor* f2) const;
    };

    set<FileDescriptor*, ltint> file_descriptors;
    
  };
  
  class Rect
  {
  public:

    Rect () 
      { empty (); }
    Rect (int xLeft, int yTop, int xRight, int yBottom) 
      { set (xLeft, yTop, xRight, yBottom); }
    Rect (HWND hwnd)
      { get_window (hwnd); }
        
    BOOL get_window (HWND hwnd)
       { return GetWindowRect (hwnd, &rect); }
    BOOL get_client (HWND hwnd)
       { return GetClientRect (hwnd, &rect); }

    BOOL set (int xLeft, int yTop, int xRight, int yBottom)
      { return ::SetRect (&rect, xLeft, yTop, xRight, yBottom); }

    BOOL empty () 
      { return ::SetRectEmpty (&rect); }

    Rect& operator= (const Rect& other)
      { CopyRect (&rect, &other.rect); return *this; }

    operator bool () const
      { return ::IsRectEmpty (&rect); }

    operator RECT* ()
      { return &rect; }
        
    BOOL copy (LPRECT lpRectDst)
      { return ::CopyRect (lpRectDst, &rect); }

    BOOL inflate (int dx, int dy) 
      { return ::InflateRect (&rect, dx, dy); }

    BOOL offset (int dx, int dy)
      { return ::OffsetRect (&rect, dx, dy); }

    BOOL point_inside (POINT Point)
      { return PtInRect (&rect, Point); }

    MWCOORD left () const
      { return rect.left; }
    MWCOORD top () const
      { return rect.top; }
    MWCOORD right () const
      { return rect.right; }
    MWCOORD bottom () const
      { return rect.bottom; }
        
  private:

    RECT rect;
  };

  inline ostream& operator<< (ostream& s, const Rect& r) {
    s << "rect"
      << "(l=" << r.left ()  << ",t=" << r.top ()
      << ",r=" << r.right () << ",b=" << r.bottom ()
      << ")";
    return s;
  }

  //
  // Generic Paint class. Try to help the paint message.
  //

  class Paint
  {
  public:

    enum { TEXT_BUF_SIZE = 512 };
    
    Paint (HWND hwnd);
    Paint (HWND hwnd, LPARAM lpParam);
    virtual ~Paint ();
    
    //
    // These begin and end a paint session.
    //

    void begin (bool init_3d = false, bool draw_3d_in_memory = false);
    void end ();
    
    //
    // Aspects of the client paint area under our control.
    //

    operator HWND () 
      { return hwnd; }
    operator RECT* ()
      { return r; }
    operator HDC () 
      { return hdc; }
    operator PAINTSTRUCT* () 
      { return &ps; }
    operator POINT* () 
      { return &pt; }

    GDICOORD get_point_x () const
      { return pt.x; }
    GDICOORD get_point_y () const
      { return pt.y; }
    
    MWCOORD left () const
      { return r.left (); }
    MWCOORD top () const
      { return r.top (); }
    MWCOORD right () const
      { return r.right (); }
    MWCOORD bottom () const
      { return r.bottom (); }
    
    //
    // Colour Control.
    //

    DWORD get_system_colour (int nIndex)
      { return ::GetSysColor (nIndex); }
    
    //
    // Pixel, line and rectange draw support.
    //

    COLORREF set_pixel (int x, int y, COLORREF crColour)
      { return ::SetPixel (hdc, x, y, crColour); }

    BOOL move_to (int x, int y)
      { return ::MoveToEx (hdc, x, y, 0); }

    BOOL line_to (int x, int y)
      { return ::LineTo (hdc, x, y); }

    BOOL line_to (int x1, int y1, int x2, int y2)
      { if (!move_to (x1, y1)) return FALSE;
        return line_to (x2, y2); }

    BOOL rectangle (int x1, int y1, int x2, int y2)
      { return ::Rectangle (hdc, x1, y1, x2, y2); }

    BOOL fill_rectangle (int x1, int y1, int x2, int y2, HBRUSH hbr)
      { Rect r (x1, y1, x2, y2);
        return ::FillRect (hdc, r, hbr); }
    
    //
    // Paint any 3d objects.
    //

    void initialise_3d (bool draw_3d_in_mem) 
      { if (!draw_3d) { 
        draw_3d = true; ::init3 (hdc, draw_3d_in_mem ? hwnd : 0); } }
    void paint_3d ()
      { if (draw_3d) { ::paint3 (hdc); draw_3d = false; } }
        
    //
    // Text Output.
    //

    void set_text_fromat (UINT uFormat) 
       { text_format = uFormat; }
     
    int text_out (int x, int y, const char *format, ...);

  private:

    HWND        hwnd;
    bool        draw_3d;
    bool        drawing;
    Rect        r;
    HDC         hdc;
    PAINTSTRUCT ps;
    POINT       pt;
    UINT        text_format;
    
    char        format_buf[TEXT_BUF_SIZE];
    
  };

  //
  // FileDescriptor handles fd event from the  User Registered File 
  // Descriptor support.
  //

  class FileDescriptor
  {
    friend class Window;
    
  public:

    FileDescriptor ();
    virtual ~FileDescriptor ();

    //
    // Enable/disable controls.
    //

    bool enable_read ();
    bool disable_read ();
    bool read_enabled () const
      { return read_is_enabled; }
    
    bool enable_write ();
    bool disable_write ();
    bool write_enabled () const
      { return write_is_enabled; }
    
    bool enable_except ();
    bool disable_except ();
    bool except_enabled () const
      { return except_is_enabled; }
    
    int fd () const
      { return file_desc; }
    
    operator int () const
      { return fd (); }
    
    const Window *get_window () const
      { return window; }
    
  protected:

    //
    // These are called in responce to user fd messages to the window.
    //

    virtual LRESULT read ();
    virtual LRESULT write ();
    virtual LRESULT except ();
    
  private:

    int    file_desc;
    bool   read_is_enabled;
    bool   write_is_enabled;
    bool   except_is_enabled;
    Window *window;
    
  };
  
  class Application
  {
  public:
  
    Application ();
    Application (MWIMAGEHDR& background);
    virtual ~Application ();
    
    HINSTANCE instance () const
      { return app_instance; }
    
    HINSTANCE prev_instance () const
      { return app_prev_instance; }

    LPSTR cmd_line () const
      { return app_cmd_line; }

    int show_cmd ()
      { return app_show_cmd; }
        
    //
    // This is called the public C linkage WinMain. Do not call.
    //

    static int WINAPI WinMain (HINSTANCE hInstance, 
                               HINSTANCE hPrevInstance, 
                               LPSTR     lpCmdLine,
                               int       nShowCmd);
  protected:
    
    virtual int initialise ();
    virtual int shutdown ();

  private:
    
    MWIMAGEHDR  *background;
    HINSTANCE app_instance;
    HINSTANCE app_prev_instance;
    LPSTR     app_cmd_line;
    int       app_show_cmd;

  };
}

#endif

