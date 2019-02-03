/* stub_winevent.c*/
#include "windows.h"

unsigned char MwKeyState[256];    /* asynchronous key state */

/* replacement function for original in mwin/winevent.c*/
SHORT WINAPI
GetKeyState(int nVirtKey) 
{
		return MwKeyState[nVirtKey & 0xff];
}

/* change the input key state for a given key */
static void
set_input_key_state(unsigned char *keystate, unsigned char key, int down)
{
	if (down)
	{
		if (!(keystate[key] & 0x80)) keystate[key] ^= 0x01;
		keystate[key] |= down;
	}
	else keystate[key] &= ~0x80;
}

/* update the input key state for a keyboard message */
void
update_input_key_state(unsigned char *keystate,UINT message,WPARAM wParam)
{
	unsigned char key;
	int down = 0;

	switch (message)
	{
	case WM_LBUTTONDOWN:
		down = (keystate == MwKeyState) ? 0xc0 : 0x80;
		/* fall through */
	case WM_LBUTTONUP:
		set_input_key_state(keystate, VK_LBUTTON, down);
		break;
	case WM_MBUTTONDOWN:
		down = (keystate == MwKeyState) ? 0xc0 : 0x80;
		/* fall through */
	case WM_MBUTTONUP:
		set_input_key_state(keystate, VK_MBUTTON, down);
		break;
	case WM_RBUTTONDOWN:
		down = (keystate == MwKeyState) ? 0xc0 : 0x80;
		/* fall through */
	case WM_RBUTTONUP:
		set_input_key_state(keystate, VK_RBUTTON, down);
		break;
	//case WM_XBUTTONDOWN:
	//	down = (keystate == MwKeyState) ? 0xc0 : 0x80;
		/* fall through */
	//case WM_XBUTTONUP:
	//	if (msg->wparam >> 16 == XBUTTON1) set_input_key_state(keystate, VK_XBUTTON1, down);
	//	else if (msg->wparam >> 16 == XBUTTON2) set_input_key_state(keystate, VK_XBUTTON2, down);
	//	break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		down = (keystate == MwKeyState) ? 0xc0 : 0x80;
		/* fall through */
	case WM_KEYUP:
	case WM_SYSKEYUP:
		key = (unsigned char)wParam;
		set_input_key_state(keystate, key, down);
		switch (key)
		{
		case VK_LCONTROL:
		case VK_RCONTROL:
			down = (keystate[VK_LCONTROL] | keystate[VK_RCONTROL]) & 0x80;
			set_input_key_state(keystate, VK_CONTROL, down);
			break;
		case VK_LMENU:
		case VK_RMENU:
			down = (keystate[VK_LMENU] | keystate[VK_RMENU]) & 0x80;
			set_input_key_state(keystate, VK_MENU, down);
			break;
		case VK_LSHIFT:
		case VK_RSHIFT:
			down = (keystate[VK_LSHIFT] | keystate[VK_RSHIFT]) & 0x80;
			set_input_key_state(keystate, VK_SHIFT, down);
			break;
		}
		break;
	}
}
