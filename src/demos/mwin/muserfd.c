#include <windows.h>
#include <wintern.h>
#include <stdio.h>
#include <stdlib.h>


#if DOS_DJGPP | defined(__FreeBSD__)
#include <sys/types.h>
#endif

#define MAX_TEST_FD (500)

LRESULT CALLBACK wproc(HWND,UINT,WPARAM,LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
        static char szAppName[]="HolaWin";
        HWND hwnd;
        WNDCLASS wndclass;
        int random_fd[MAX_TEST_FD][2], fd, unreg_fd;

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

        /*
         * Create a random list of fd's to test the list
         * code.
         */
        
        printf ("Select a random list of fd's to test with.\n");

        for (fd = 0; fd < MAX_TEST_FD; fd++)
        {
          random_fd[fd][0] = random () % FD_SETSIZE;
          random_fd[fd][1] = random () % 3;
        }

        for (fd = 0; fd < MAX_TEST_FD; fd++)
        {
          switch (random_fd[fd][1])
          {
            case 0:
              MwRegisterFdInput (hwnd, random_fd[fd][0]);
              break;
              
            case 1:
              MwRegisterFdOutput (hwnd, random_fd[fd][0]);
              break;
              
            case 2:
              MwRegisterFdExcept (hwnd, random_fd[fd][0]);
              break;

            default:

              printf ("Bad fd type, fd index = %i, type = %i !\n",
                      fd, random_fd[fd][1]);
              return 1;
          }
        }

        unreg_fd = random () % MAX_TEST_FD;

        for (fd = 0; fd < MAX_TEST_FD; fd++)
        {
          switch (random_fd[unreg_fd][1])
          {
            case 0:
              MwUnregisterFdInput (hwnd, random_fd[unreg_fd][0]);
              break;
              
            case 1:
              MwUnregisterFdOutput (hwnd, random_fd[unreg_fd][0]);
              break;
              
            case 2:
              MwUnregisterFdExcept (hwnd, random_fd[unreg_fd][0]);
              break;

            default:

              printf ("Bad fd type !\n");
              return 1;
          }

          unreg_fd++;
          if (unreg_fd >= MAX_TEST_FD)
            unreg_fd = 0;
        }

        
        return 0;
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
#if 0
        case WM_MOUSEFIRST:
#endif
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
