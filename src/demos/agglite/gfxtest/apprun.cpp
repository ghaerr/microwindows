/*
 * Copyright (c) 2017 Greg Haerr <greg@censoft.com>
 *
 * platform class init/run routines
 */
#include <sys/time.h>
#include "device.h"
#include "platform.h"

/*
 * Initialize the graphics and mouse devices at startup.
 * Returns nonzero with a message printed if the initialization failed.
 */
bool platform::init()
{
	PSD		psd;

	// open with no background clear
	psd = GdOpenScreenExt(FALSE);
	if (!psd)
		return false;

	m_keyboard_fd = GdOpenKeyboard();
	m_mouse_fd = GdOpenMouse();

	// delay x11 driver updates until preselect time for speed
	psd->flags |= PSF_DELAYUPDATE;

	// clear to background once
	psd->FillRect(psd, 0, 0, psd->xvirtres-1, psd->yvirtres-1, GdFindColor(psd, MWRGB(0, 128, 128)));

	if (m_mouse_fd >= 0)
	{
		static MWCURSOR arrow = {	/* default arrow cursor*/
			16, 16, 0,  0, MWRGB(255, 255, 255), MWRGB(0, 0, 0),
			{ 0xe000, 0x9800, 0x8600, 0x4180,
		  	0x4060, 0x2018, 0x2004, 0x107c,
		  	0x1020, 0x0910, 0x0988, 0x0544,
		  	0x0522, 0x0211, 0x000a, 0x0004 },
			{ 0xe000, 0xf800, 0xfe00, 0x7f80,
		  	0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
		  	0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
		  	0x073e, 0x021f, 0x000e, 0x0004 }
		};

		// init cursor
		MWCOORD cursorx = psd->xvirtres / 2;
		MWCOORD cursory = psd->yvirtres / 2;
		GdShowCursor(psd);
		MoveCursor(cursorx, cursory);
		SetCursor(&arrow);

		// configure mouse driver
		GdRestrictMouse(0, 0, psd->xvirtres - 1, psd->yvirtres - 1);
		GdMoveMouse(cursorx, cursory);
	}

	// init graphics class
	gfx.init();

	return true;
}

void platform::close()
{
	GdCloseScreen(&scrdev);
	GdCloseMouse();
	GdCloseKeyboard();
}

int platform::run()
{
	bool quit = false;

	while(!quit)
	{
		if (m_updateflag)				// force_redraw() calls on_draw() and gfx.update()
		{
			on_draw();					// app draw code
			gfx.update();				// update display
			m_updateflag = false;
		}

		if (!m_waitmode)
		{
			// poll for event
			if (CheckEvents(false) == 0)
			{
				on_idle();				// handler must call gfx.update() if needed
				continue;
			}
		}

		// wait or poll for event
		CheckEvents(m_waitmode);
	}
	return 0;
}

// return # events handled
int platform::CheckEvents(bool mayWait)
{
	fd_set	rfds;
	int	setsize = 0;
	int nevents = 0;
	long timeout;
	struct timeval to;

	if(scrdev.PreSelect)				// flush output queue
		scrdev.PreSelect(&scrdev);

#if 1	// UNIX-only code needs moving to driver
	if (mayWait) {
		/* setup read FD for use in select*/
		FD_ZERO(&rfds);
		if(m_mouse_fd >= 0) {
			FD_SET(m_mouse_fd, &rfds);
			if(m_mouse_fd > setsize)
				setsize = m_mouse_fd;
		}
		if(m_keyboard_fd >= 0) {
			FD_SET(m_keyboard_fd, &rfds);
			if(m_keyboard_fd > setsize)
				setsize = m_keyboard_fd;
		}
		++setsize;

		//timeout = MwGetNextTimeoutValue();	// returns ms or -1 for no timer
timeout = -1;

		if (timeout != -1)
		{
			to.tv_sec = timeout / 1000;
			to.tv_usec = (timeout % 1000) * 1000;
		}

		// wait for event or timeout
		select(setsize, &rfds, NULL, NULL, (timeout != -1)? &to: NULL);
	}
#endif

	// non-blocking handle mouse and keyboard input
	while(CheckMouseEvent())
		++nevents;

	while(CheckKeyboardEvent())
		++nevents;

	// handle timers
	//MwHandleTimers();
	return nevents;
}

/*
 * Return time in 25 millisecond ticks
 */
unsigned long platform::GetTickCount()
{
#if 1	// UNIX-only code move to driver
	struct timeval t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 25000) * 25);
#endif
}
