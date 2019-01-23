/*
 * Keyboard Driver, DOS & DJGPP & GRX version
 *
 * adapted to version 0.93 by Georg Potthast
 */

#include "device.h"

#include <bios.h>
#include <dpmi.h>
//alert -> getch() seems to pull in conio.h which contains a "window" variable which causes
//conflicts with some fltk examples, e.g. boxtype.cxx
//#include <stdio.h>
//int     getch(void); //#include <conio.h>

extern void toggle_text_mode();

static int  KBD_Open(KBDDEVICE *pkd);
static void KBD_Close(void);
static void KBD_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers);
static int KBD_Read (MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
static int	KBD_Poll(void);

static int update_modstate(void);
static int get_extended_key(MWKEY *bios_keyvalue);
static int convert_alt_key(MWKEY *bios_keyvalue);

extern void toggle_text_mode();

static MWKEYMOD key_modstate=0; /* typedef unsigned int */
static int breakwaiting=0; /* do we need to send a break code ?*/

/* bit definitions for modifier test */
#define bit0 0x1
#define bit1 0x2
#define bit2 0x4
#define bit3 0x8
#define bit4 0x10
#define bit5 0x20
#define bit6 0x40
#define bit7 0x80
#define bit8 0x100
#define bit9 0x200
#define bita 0x400
#define bitb 0x800
#define bitc 0x1000
#define bitd 0x2000
#define bite 0x4000
#define bitf 0x8000

KBDDEVICE kbddev = {
	KBD_Open,
	KBD_Close,
	KBD_GetModifierInfo,
	KBD_Read,
	KBD_Poll
};

/*
This contains the data needed to translate from ibm codepage 850 to iso8859-1
SOURCE "ibm850" DESTINATION "iso8859-1"
*/
static MWKEY bioskeyvalue_to_iso8859_1[256] = {
  /*000-015*/  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 
  /*016-031*/  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31, 
  /*032-047*/  32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47, 
  /*048-063*/  48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63, 
  /*064-079*/  64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79, 
  /*080-095*/  80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95, 
  /*096-111*/  96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111, 
  /*112-127*/  112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127, 
  /*128-143*/  199,252,233,226,228,224,229,231,234,235,232,239,238,236,196,197, 
  /*144-159*/  201,230,198,244,246,242,251,249,255,214,220,248,163,216,215,159, 
  /*160-175*/  225,237,243,250,241,209,170,186,191,174,172,189,188,161,171,187, 
  /*176-191*/  128,129,130,131,132,193,194,192,169,133,134,135,136,162,165,137, 
  /*192-207*/  138,139,140,141,142,143,227,195,144,145,146,147,148,149,150,164, 
  /*208-223*/  240,208,202,203,200,151,205,206,207,152,153,154,155,166,204,156, 
  /*224-239*/  211,223,212,210,245,213,181,254,222,218,219,217,253,221,175,180, 
  /*240-255*/  173,177,157,190,182,167,247,184,176,168,183,185,179,178,158,160 
};

static MWKEY alt_scan_to_value[40] = {
/* 0x10 - 0x1B */ 0x51,0x57,0x45,0x52,0x54,0x59,0x55,0x49,0x4F,0x50,0x7B,0x7D,0,0,
/* 0x1E - 0x29 */ 0x41,0x53,0x44,0x46,0x47,0x48,0x4A,0x4B,0x3C,0x3A,0x22,0x7E,0,
/* 0x2B - 0x35 */ 0x7C,0x5A,0x58,0x43,0x56,0x42,0x4E,0x4D,0x3C,0x3E,0x3F
};

/*
 * Open the keyboard.
 */
static int
KBD_Open(KBDDEVICE *pkd)
{
	if(update_modstate() < 0)
    	return DRIVER_FAIL;
	return DRIVER_OKNOTFILEDESC;
}

/*
 * Close the keyboard.
 */
static void
KBD_Close(void)
{
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
KBD_GetModifierInfo(MWKEYMOD *modifiers, MWKEYMOD *curmodifiers)
{
	if (modifiers)
		*modifiers = MWKMOD_SHIFT | MWKMOD_CTRL | MWKMOD_ALT;
	if (curmodifiers)
		*curmodifiers = key_modstate; //bioskey(0x12);
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 */
static int
KBD_Read (MWKEY *buf, MWKEYMOD *modifiers, MWSCANCODE *scancode)
{
	__dpmi_regs r; //needed if not using getch()

	MWKEY bios_value;
	static MWKEY bios_keyvalue;
	MWKEY dos_keyvalue;
	static MWSCANCODE bios_scanvalue;
//return 0; //test
	/* wait until a char is ready or send key release*/
	if(!bioskey(0x11)){
		//bios_keyvalue = 0; //test
		if (bios_keyvalue == 0) { //set to 0 if break has been sent below
			return 0; //no key pressed
		}else {
	  		*modifiers = key_modstate;
			*buf = bios_keyvalue;
			*scancode = bios_scanvalue | bit7; //generate break code
// EPRINTF("key_release key_modstate:%X buf:%X scan:%x\n",key_modstate,*buf,*scancode);
			bios_keyvalue=0;
			bios_scanvalue=0;
			breakwaiting=0;
			return 2; //key released
		}
	}
	breakwaiting=1; //key received, have to send break later

	/* read keyboard shift status*/
	update_modstate();
	*modifiers = key_modstate; //bioskey(0x12);

	/* read keyboard character using bios without removing from buffer*/
	bios_value=bioskey(0x11);

#if 1
	/* read key again with DOS to use keyboard driver installed */
	/* and remove from keyboard buffer by doing that */
	r.x.ax = 0x0600; /*direct console io */
    r.x.dx = 0x00FF; /*select input */
    __dpmi_int(0x21, &r);
	if ((r.x.flags & 0x0040)==0) { //test zero flag (bit 6 zero based) whether key available
		if (r.h.al == 0) {	//read again if extended key
			r.x.ax = 0x0600; /*direct console io */
			r.x.dx = 0x00FF; /*select input */
			__dpmi_int(0x21, &r);
		}
	    dos_keyvalue=r.h.al;
	}
#else //avoid __dpmi_int()
	dos_keyvalue=getch();
	if (dos_keyvalue==0) dos_keyvalue=getch();
#endif
	bios_keyvalue=bios_value & 0xFF;
	bios_scanvalue=(MWSCANCODE) (bios_value >> 8);
	//EPRINTF("Bios-value:%X buf:%X scan:%x dosvalue:%x\n",bios_value,bios_keyvalue,bios_scanvalue,dos_keyvalue);

	if ((bios_keyvalue == 0)||(bios_keyvalue == 0xE0)) //extended key
	{   bios_keyvalue = bios_scanvalue; //move scanvalue to key value and modify that
		get_extended_key(&bios_keyvalue);
		if (key_modstate & MWKMOD_ALT) {convert_alt_key(&bios_keyvalue);}
	}
	else {
		bios_keyvalue=dos_keyvalue; //for none-extended keys
		bios_keyvalue = bioskeyvalue_to_iso8859_1[bios_keyvalue]; //convert latin1 to iso
	}

	if ((key_modstate & MWKMOD_ALT) && (bios_scanvalue == 0x33)) { /* Alt-Comma - toggle text mode*/ 
			toggle_text_mode();}

	//EPRINTF("key_modstate:%X Bios-value:%X bios-keyvalue:%X scan:%x\n",key_modstate,bios_value,bios_keyvalue,bios_scanvalue);
	if (key_modstate & MWKMOD_CTRL) bios_keyvalue &= 0x1f;	/* Control code - allow ctrl-extended-key*/ 

	
	*buf = bios_keyvalue;
	*scancode = bios_scanvalue;

	 // EPRINTF("key_modstate:%X buf:%X scan:%x\n",key_modstate,*buf,*scancode);
	 
	if(*buf == 0x1b)	/* special case ESC*/
		return -2;
	return 1;
}

/*
**
*/
static int
KBD_Poll(void)
{
	//return 0; //test
	if ((!kbhit())&&(breakwaiting==0)) //(bioskey(0x11)==0)
	  return 0;
	else
	  return 1;
}


/* update key_modstate */ 
static int update_modstate(void)
{
	unsigned int state;
	
	/* read keyboard shift status*/
	state = bioskey(0x12);
		// EPRINTF("bios state: %X -",state);
	/* calculate kbd modifiers*/
	//key_modstate &= (MWKMOD_NUM|MWKMOD_CAPS|MWKMOD_SCR); /*XOR-clear all but these bits*/
	key_modstate = 0;
	if (state & bit0)
		key_modstate |= MWKMOD_RSHIFT; 
	if (state & bit1)
		key_modstate |= MWKMOD_LSHIFT; 
	/* do not query bits 2-3 since we do ctrl+alt separately */
	if (state & bit4)
		key_modstate |= MWKMOD_SCR; 
	if (state & bit5)
		key_modstate |= MWKMOD_NUM; 
	if (state & bit6)
		key_modstate |= MWKMOD_CAPS; 
	//if (state & bit7)
	//	; //insert lock not handled 
	if (state & bit8)
		key_modstate |= MWKMOD_LCTRL; 
	if (state & bit9)
		key_modstate |= MWKMOD_LALT; 
	if (state & bita)
		key_modstate |= MWKMOD_RCTRL; 
	if (state & bitb) {
		key_modstate |= MWKMOD_RALT; //}
		key_modstate |= MWKMOD_ALTGR;} //right alt is alt-gr
	if (state & bitc)
		key_modstate |= MWKMOD_SCR; 
	if (state & bitd)
		key_modstate |= MWKMOD_NUM; 
	if (state & bite)
		key_modstate |= MWKMOD_CAPS; 
	//if (state & bitf)
	//		; //sys request not handled  
	// EPRINTF("key_modstate: %X\n",key_modstate);
	return 0;
}

int get_extended_key(MWKEY *bios_keyvalue) {	
	
	switch (*bios_keyvalue) {
	    case 83:
		    *bios_keyvalue = MWKEY_DELETE;
		    break;
	    case 71:
		    *bios_keyvalue = MWKEY_HOME;
		    break;
	    case 75:
		    *bios_keyvalue = MWKEY_LEFT;
		    break;
	    case 72:
		    *bios_keyvalue = MWKEY_UP;
		    break;
	    case 77:
		    *bios_keyvalue = MWKEY_RIGHT;
		    break;
	    case 80:
		    *bios_keyvalue = MWKEY_DOWN;
		    break;
	    case 73:
		    *bios_keyvalue = MWKEY_PAGEUP;
		    break;
	    case 81:
		    *bios_keyvalue = MWKEY_PAGEDOWN;
		    break;
	    case 79:
		    *bios_keyvalue = MWKEY_END;
		    break;
	    case 82:
		    *bios_keyvalue = MWKEY_INSERT;
		    break;
	    /*
		case XK_Pause:
	    case XK_Break:
		case XK_F15:
		    *bios_keyvalue = MWKEY_QUIT;
		    break;
	    case XK_Print:
	    case XK_Sys_Req:
		    *bios_keyvalue = MWKEY_PRINT;
		    break;
	    */
		/*
		case XK_Menu:
		    *bios_keyvalue = MWKEY_MENU;
		    break;
	    case XK_Cancel:
		    *bios_keyvalue = MWKEY_CANCEL;
		    break;
	    case XK_KP_Enter:
		    *bios_keyvalue = MWKEY_KP_ENTER;
		    break;
	    case XK_KP_Home:
		    *bios_keyvalue = MWKEY_KP7;
		    break;
	    case XK_KP_Left:
		    *bios_keyvalue = MWKEY_KP4;
		    break;
	    case XK_KP_Up:
		    *bios_keyvalue = MWKEY_KP8;
		    break;
	    case XK_KP_Right:
		    *bios_keyvalue = MWKEY_KP6;
		    break;
	    case XK_KP_Down:
		    *bios_keyvalue = MWKEY_KP2;
		    break;
	    case XK_KP_Page_Up:
		    *bios_keyvalue = MWKEY_KP9;
		    break;
	    case XK_KP_Page_Down:
		    *bios_keyvalue = MWKEY_KP3;
		    break;
	    case XK_KP_End:
		    *bios_keyvalue = MWKEY_KP1;
		    break;
	    case XK_KP_Insert:
		    *bios_keyvalue = MWKEY_KP0;
		    break;
	    case XK_KP_Delete:
		    *bios_keyvalue = MWKEY_KP_PERIOD;
		    break;
	    case XK_KP_Equal:
		    *bios_keyvalue = MWKEY_KP_EQUALS;
		    break;
	    case XK_KP_Multiply:
		    *bios_keyvalue = MWKEY_KP_MULTIPLY;
		    break;
	    case XK_KP_Add:
		    *bios_keyvalue = MWKEY_KP_PLUS;
		    break;
	    case XK_KP_Subtract:
		    *bios_keyvalue = MWKEY_KP_MINUS;
		    break;
	    case XK_KP_Decimal:
		    *bios_keyvalue = MWKEY_KP_PERIOD;
		    break;
	    case XK_KP_Divide:
		    *bios_keyvalue = MWKEY_KP_DIVIDE;
		    break;
	    case XK_KP_5:
	    case XK_KP_Begin:
		    *bios_keyvalue = MWKEY_KP5;
		    break;
	    */
		case 59:
		    *bios_keyvalue = MWKEY_F1;
		    break;
	    case 60:
		    *bios_keyvalue = MWKEY_F2;
		    break;
	    case 61:
		    *bios_keyvalue = MWKEY_F3;
		    break;
	    case 62:
		    *bios_keyvalue = MWKEY_F4;
		    break;
	    case 63:
		    *bios_keyvalue = MWKEY_F5;
		    break;
	    case 64:
		    *bios_keyvalue = MWKEY_F6;
		    break;
	    case 65:
		    *bios_keyvalue = MWKEY_F7;
		    break;
	    case 66:
		    *bios_keyvalue = MWKEY_F8;
		    break;
	    case 67:
		    *bios_keyvalue = MWKEY_F9;
		    break;
	    case 68:
		    *bios_keyvalue = MWKEY_F10;
		    break;
	    case 133:
		    *bios_keyvalue = MWKEY_F11;
		    break;
	    case 134:
		    *bios_keyvalue = MWKEY_F12;
		    break;

	    /* state modifiers*/
	    //not implemented - see X11 driver
		
		} //end of switch

	return 0;
}

int convert_alt_key(MWKEY *bios_keyvalue){
	if ((*bios_keyvalue > 0x77) && (*bios_keyvalue < 0x81)) {
		*bios_keyvalue = *bios_keyvalue - 0x47; //number 1-9
		return 0;
	}
	if ((*bios_keyvalue > 0x0F) && (*bios_keyvalue < 0x36)) {
		*bios_keyvalue = alt_scan_to_value[*bios_keyvalue-0x10]; 
		return 0;
	}	

	switch (*bios_keyvalue) {
		case 0x81:
		    *bios_keyvalue = 0x30; //0
		    break;
	    case 0x82:
		    *bios_keyvalue = 0x2D; //-
		    break;
	    case 0x83:
		    *bios_keyvalue = 0x3D; //=
		    break;
	}
	return 0;
}
