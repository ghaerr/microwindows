/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Microwindows main() function
 *	ALLEGRO and EMSCRIPTEN require special handling
 */
#include "windows.h"

extern HWND		rootwp;			/* root window pointer */

#if !NOMAIN
#if ALLEGRO
int real_main(int ac, char **av);	/* ALLEGRO renamed main entry point*/
int main(int ac, char **av)
{
#define MAIN(ac,av)	real_main(ac,av)
	/* required to force allegro to run OSX API calls on the main thread*/
	extern int al_run_main(int ac, char **av, int (*main_ptr)());
	return al_run_main(ac, av, real_main);
}
#else
#define MAIN(ac,av)	main(ac,av)
#endif /* ALLEGRO*/

int
MAIN(int ac, char **av)			/* ALLEGRO=real_main(), main() otherwise*/
{
	invoke_WinMain_Start(ac, av);

	/* call windows main program entry point*/
	WinMain(rootwp->hInstance, NULL,
		(LPSTR)((PMWAPPINSTANCE)rootwp->hInstance)->szCmdLine, SW_SHOW);

	invoke_WinMain_End();
	return 0;
}
#endif /* !NOMAIN*/
