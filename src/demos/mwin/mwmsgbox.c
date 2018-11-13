/* Demo for a MessageBox in Microwindows. This program should pop up a simple message with an "OK" and
 * a "Cancel" button. Both exit the program.
 *
 * Georg Potthast 2018 
 */
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  MwInitializeDialogs ( hInstance );
  MessageBox(NULL, " Hello to this messagebox demo! ", "A minimal windows program", MB_OKCANCEL );

  return 0;
}
 
