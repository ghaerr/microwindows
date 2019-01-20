#include <sys/time.h>
#include <machine/console.h>
#include <vgl.h>

#include "device.h"

#define KBLEN		30
unsigned short kbuffer[KBLEN];
unsigned short klen=0;
int states[256];

const int quertycodes[48+1]={41, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,\
			  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 43,\
			  30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 44, 45,\
			  46, 47, 48, 49, 50, 51, 52, 53, 57, 0};
const char chars[48] =    {'`','1','2','3','4','5','6','7','8','9','0','-','=',\
			 'q','w','e','r','t','y','u','i','o','p','[',']','\\',\
			 'a','s','d','f','g','h','j','k','l',';','\'','z','x',\
			 'c','v','b','n','m',',','.','/',' '};


static int initkeyb(KBDDEVICE *pkd);
static void restorekeyb(void);
static  void TTY_GetModifierInfo(int *modifiers);
static int getkey(MWUCHAR *buf, int *modifiers);
static int kbhit(void);

KBDDEVICE kbddev = {
  initkeyb,
  restorekeyb,
  TTY_GetModifierInfo,
  getkey,
  kbhit
};

/*
 * Return the possible modifiers for the keyboard.
 */

static  void TTY_GetModifierInfo(int *modifiers)
{
	*modifiers = 0;			/* no modifiers available */
}

static int initkeyb(KBDDEVICE *pkd)
{
	VGLKeyboardInit(VGL_CODEKEYS);
	memset(states, FALSE, (sizeof states));
	return(0);

}

static void restorekeyb(void)
{
	VGLKeyboardEnd();
}

void ProcessKbd(void)
{
	unsigned short result, i;
	int isasymbol;
	int state;

	while((result = VGLKeyboardGetCh()) != 0) {

		if(result < 128)
			state = TRUE;
		else {
			state = FALSE;
			result -= 128;
		}

		isasymbol = FALSE;
		for(i=0;quertycodes[i]!=0;i++)
			if(result == quertycodes[i]) {
				result = chars[i];
				isasymbol = TRUE;
				break;
			}

		if (isasymbol == FALSE)
			result+=128;

		states[result] = state;

		if(state == TRUE)
			continue;

		if(klen == KBLEN) /* Buffer is full, drop some pieces */
			memcpy(kbuffer, kbuffer + 1, --klen);
		kbuffer[klen++] = result;
	}
}

int GetAsyncKeyState(int key)
{
	ProcessKbd();
	return(states[key]);
}

static int getkey(MWUCHAR *buf, int *modifiers)
{
	MWUCHAR result;
	
	while(kbhit() != TRUE);
	result = kbuffer[0];
	memcpy(kbuffer, kbuffer + 1, --klen);

	*buf=result;
	return(0);
}

static int kbhit(void)
{
	ProcessKbd();

	if (klen > 0)
		return(1);
	else
		return(0);
}

