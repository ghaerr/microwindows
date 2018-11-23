/*
 * Copyright (c) 2017 Greg Haerr <greg@censoft.com>
 *
 * application platform support
 */
#include "graphics.h"

// base class for derived application class
class platform {
public:
	// constructor
	platform() :
		m_updateflag(true),
		m_waitmode(true),
		m_curcursor(0),
		m_cursorx(-1),
		m_cursory(-1),
		m_keyModifiers(0),
		m_mouse_fd(-1),
		m_keyboard_fd(-1)
	{
	}

	~platform()
	{
		close();
	}

	// override with application routines
	virtual void on_draw() {}		// force_redraw() calls on_draw and gfx.update()
	virtual void on_idle() {}		// idle handler must call gfx.update() if needed
	virtual void on_mouse_button_down(int x, int y, unsigned flags) {}
	virtual void on_mouse_button_up(int x, int y, unsigned flags) {}
	virtual void on_mouse_button_dblclick(int x, int y, unsigned flags) {}
	virtual void on_mouse_move(int x, int y, unsigned flags) {}
	virtual void on_arrow_key(int key, int x, int y, unsigned flags) {}
	virtual void on_key_press(int key, int x, int y, unsigned flags) {}

	// apprun.cpp init/run processing routines
	bool init();
	void close();
	int run();
	void force_redraw() { m_updateflag = true; }
	bool wait_mode() { return m_waitmode; }
	void wait_mode(bool waitmode) { m_waitmode = waitmode; }

	MWCOORD mouse_x() { return m_cursorx; }
	MWCOORD mouse_y() { return m_cursory; }

private:
	int CheckEvents(bool mayWait);
	unsigned long GetTickCount();

	// appevent.c - event handling routines
	bool CheckMouseEvent(void);
	bool CheckKeyboardEvent(void);
	void HandleMouseStatus(MWCOORD newx, MWCOORD newy, int newbuttons);
	bool CheckDoubleClick(int buttons);
	void DeliverMouseEvent(int buttons, int changebuttons, MWKEYMOD modifiers);
	void DeliverKeyboardEvent(MWKEY keyvalue, MWKEYMOD modifiers, MWSCANCODE scancode, bool pressed);
	void MoveCursor(MWCOORD x, MWCOORD y);
	void SetCursor(PMWCURSOR cp);

	unsigned KeyModifiers() { return m_keyModifiers; }

protected:
	aggprocessing gfx;
	//processing gfx;

private:
	bool m_updateflag;			// force redraw if set
	bool m_waitmode;			// set false for non-block events/on_idle()
	PMWCURSOR m_curcursor;		// currently enabled cursor
	MWCOORD	m_cursorx;			// x position of cursor
	MWCOORD	m_cursory;			// y position of cursor
	MWKEYMOD m_keyModifiers;	// keyboard modifiers
	// UNIX-only, move to driver
	int		m_mouse_fd;			// mouse file descriptor
	int		m_keyboard_fd;		// keyboard file descriptor
};
