/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Multiple Win32 application loader/main loop for Microwindows
 */
#include "windows.h"
#include "wintern.h"

#if EMSCRIPTEN
#include <emscripten.h>
#else
#include <dlfcn.h>
#endif

extern HWND		rootwp;			/* root window pointer */

/* run a single round of main loop (non-blocking)*/
static void
main_loop(void)
{
	MSG msg;

	/* process any messages in input queue*/
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	/* poll mouse and keyboard for input*/
	MwSelect(FALSE);

	/* handle expired timers*/
	MwHandleTimers();
}

#if !EMSCRIPTEN
/* default dynamically loaded apps*/
char *applist[] = {
"mwdemo", "mwdemo2", "mwmine", "mwterm", "mwpenstyles"
};
#define NUMAPPS		(sizeof(applist)/sizeof(char *))

/* load app shared lib and call WinMain*/
static void
load_app(char *name)
{
	void *dp;
	void (*fp)(HINSTANCE, HINSTANCE, LPSTR, int);
	char path[64];

	sprintf(path, "bin/%s.so", name);
	printf("Loading %s\n", path);
	dp = dlopen(path, RTLD_NOW|RTLD_LOCAL);
	if (!dp)
	{
		printf("App load failed: %s\n", path);
		return;
	}
	fp = dlsym(dp, "WinMain");
	if (!fp)
	{
		printf("Sym not found: WinMain\n");
		return;
	}
	fp(rootwp->hInstance, NULL, "", SW_SHOW);
}
#endif /* !EMSCRIPTEN*/

int
main(int ac, char **av)
{
	/* startup win32 system*/
	invoke_WinMain_Start(ac, av);
	//MwInitializeDialogs(); needs hInstance

#if EMSCRIPTEN
	/* linked mwin apps*/
	extern int WINAPI mwmine_WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
	extern int WINAPI mwdemo_WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
	extern int WINAPI mwdemo2_WinMain(HINSTANCE, HINSTANCE, LPSTR, int);

	/* init each application by calling its main entrypoint*/
	mwmine_WinMain(rootwp->hInstance, NULL, "", SW_SHOW);
	mwdemo_WinMain(rootwp->hInstance, NULL, "", SW_SHOW);
	mwdemo2_WinMain(rootwp->hInstance, NULL, "", SW_SHOW);
#else

	/* load app shared lib and call WinMain on each*/
	if (ac > 1)
	{
		do {
			load_app(av[1]);
			av++;
		} while (--ac > 1);
	}
	else
	{
		int i;

		for (i=0; i<NUMAPPS; i++)
			load_app(applist[i]);
	}
#endif

#if EMSCRIPTEN
	/* setup main loop to be called regularly by browser*/
	emscripten_set_main_loop(main_loop, 0, 0);

#else
	/* main loop - collect events and send to mwin app*/
	for (;;)
	{
		main_loop();
    }

	/* cleanup on real exit*/
	//invoke_WinMain_End();
#endif

	return 0;
}
