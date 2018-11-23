/*
 * Copyright (c) 2017 Greg Haerr <greg@censoft.com>
 *
 * platform class event routines
 */
#include "device.h"
#include "platform.h"

#define DBLCLICKSPEED	400		// mouse dblclik speed msecs
#define DBLCLICKXY	 	2		// +/- double click position

/*
 * Update mouse status and issue events on it if necessary.
 * This function doesn't block, but is normally only called when
 * there is known to be some data waiting to be read from the mouse.
 */
bool platform::CheckMouseEvent()
{
	MWCOORD		rootx;		/* latest mouse x position */
	MWCOORD		rooty;		/* latest mouse y position */
	int		newbuttons;	/* latest buttons */
	int		mousestatus;	/* latest mouse status */

	/* Read the latest mouse status: */
	mousestatus = GdReadMouse(&rootx, &rooty, &newbuttons);
	if(mousestatus < 0) {
		return FALSE;
	} else if(mousestatus) {	/* Deliver events as appropriate: */	
		HandleMouseStatus(rootx, rooty, newbuttons);
		return TRUE;
	}
	return FALSE;
}

/*
 * Update keyboard status and issue events on it if necessary.
 * This function doesn't block, but is normally only called when
 * there is known to be some data waiting to be read from the keyboard.
 */
bool platform::CheckKeyboardEvent()
{
	MWKEY	 	mwkey;		/* latest character */
	MWKEYMOD 	modifiers;	/* latest modifiers */
	MWSCANCODE	scancode;
	int	 	keystatus;	/* latest keyboard status */

	/* Read the latest keyboard status: */
	keystatus = GdReadKeyboard(&mwkey, &modifiers, &scancode);
	if(keystatus < 0) {
		return FALSE;
	} else if(keystatus) {		/* Deliver events as appropriate: */	
		DeliverKeyboardEvent(mwkey, modifiers, scancode, keystatus==1? TRUE: FALSE);
		return TRUE;
	}
	return FALSE;
}

/*
 * Move the cursor to the specified absolute screen coordinates.
 * The coordinates are that of the defined hot spot of the cursor.
 */
void platform::MoveCursor(MWCOORD x, MWCOORD y)
{
	/*
	 * Move the cursor only if necessary, offsetting it to
	 * place the hot spot at the specified coordinates.
	 */
	if (x != m_cursorx || y != m_cursory) {
		if(m_curcursor)
			GdMoveCursor(x - m_curcursor->hotx, y - m_curcursor->hoty);
		m_cursorx = x;
		m_cursory = y;
	}

	/*
	 * Now check to see which window the mouse is in and whether or
	 * not the cursor shape should be changed.
	 */
	//MwCheckMouseWindow();
	//MwCheckCursor();
}

/*
 * Set the application cursor
 */
void platform::SetCursor(PMWCURSOR cp)
{
	if (cp == m_curcursor)
		return;

	m_curcursor = cp;
	GdMoveCursor(m_cursorx - cp->hotx, m_cursory - cp->hoty);
	GdSetCursor(cp);
}

/*
 * Handle all mouse events.  These are mouse enter, mouse exit, mouse
 * motion, mouse position, button down, and button up.  This also moves
 * the cursor to the new mouse position and changes its shape if needed.
 */
void platform::HandleMouseStatus(MWCOORD newx, MWCOORD newy, int newbuttons)
{
	int		changebuttons;	/* buttons that have changed */
	MWKEYMOD	modifiers;	/* latest modifiers */
	static int curbuttons;

	GdGetModifierInfo(NULL, &modifiers); /* Read kbd modifiers */

	/*
	 * First, if the mouse has moved, then position the cursor to the
	 * new location, which will send mouse enter, mouse exit, focus in,
	 * and focus out events if needed.  Check here for mouse motion and
	 * mouse position events.
	 */
	if (newx != m_cursorx || newy != m_cursory) {
		MoveCursor(newx, newy);
		DeliverMouseEvent(newbuttons, 0, modifiers);
	}

	/*
	 * Next, generate a button up event if any buttons have been released.
	 */
	changebuttons = (curbuttons & ~newbuttons);
	if (changebuttons)
		DeliverMouseEvent(newbuttons, changebuttons, modifiers);

	/*
	 * Finally, generate a button down event if any buttons have been
	 * pressed.
	 */
	changebuttons = (~curbuttons & newbuttons);
	if (changebuttons)
		DeliverMouseEvent(newbuttons, changebuttons, modifiers);

	curbuttons = newbuttons;
}

/* determine double click eligibility*/
bool platform::CheckDoubleClick(int buttons)
{
	bool result = false;
	unsigned long		tick;
	static int			lastbutton = 0;
	static unsigned long lasttick;
	static int			lastx, lasty;

	if(buttons & (MWBUTTON_L|MWBUTTON_M|MWBUTTON_R))
	{
		tick = GetTickCount();
		if(buttons == lastbutton &&
		    tick - lasttick < DBLCLICKSPEED &&
		    abs(m_cursorx-lastx) < DBLCLICKXY &&
		    abs(m_cursory-lasty) < DBLCLICKXY)
		{
			result = true;
			buttons = 0;		// disallow continuous dbl clicks
		}
		lastbutton = buttons;
		lasttick = tick;
		lastx = m_cursorx;
		lasty = m_cursory;
	}
	return result;
}

/*
 * Deliver a mouse button or motion event.
 */
void platform::DeliverMouseEvent(int buttons, int changebuttons, MWKEYMOD modifiers)
{
	m_keyModifiers = modifiers;	// save keyboard modifiers

	if(!changebuttons)
		on_mouse_move(m_cursorx, m_cursory, buttons);

	if(changebuttons)
	{
		if(CheckDoubleClick(buttons))
			on_mouse_button_dblclick(m_cursorx, m_cursory, buttons);
		else if(buttons)
			on_mouse_button_down(m_cursorx, m_cursory, buttons);
		else
			on_mouse_button_up(m_cursorx, m_cursory, buttons);
	}
}

/*
 * Deliver a keyboard event.
 */
void platform::DeliverKeyboardEvent(MWKEY keyvalue, MWKEYMOD modifiers, MWSCANCODE scancode, bool pressed)
{
	m_keyModifiers = modifiers;	// save keyboard modifiers

	if (pressed)
	{
		on_key_press(keyvalue, m_cursorx, m_cursory, modifiers);
		switch(keyvalue)
		{
		case MWKEY_LEFT:
		case MWKEY_RIGHT:
		case MWKEY_UP:
		case MWKEY_DOWN:
			on_arrow_key(keyvalue, m_cursorx, m_cursory, modifiers);
			break;
		}
	}
}
