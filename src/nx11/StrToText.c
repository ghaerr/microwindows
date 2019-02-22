/* StrToText.c - String conversion and TextProperty functions */

/* Portions Copyright 2003, Jordan Crouse (jordan@cosmicpenguin.net) */

/* News and Notes:
   GTK+ in default mode uses the TextProperty functions to convert back and
   forth from wide char mode to the current locale mode.  Thus, we have included
   XwcTextListToTextProperty and XwcTextPropertyToTextList.  

   I am probably not using the wide char functions correctly, and if I am not,
   then please adjust them accordingly.  
*/

#include "nxlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "X11/Xutil.h"
#include "X11/Xatom.h"

Status
XStringListToTextProperty(char **argv, int argc, XTextProperty * ret)
{
	char *buffer;
	int count = 0, i;
	XTextProperty proto;

	for (i = 0; i < argc; i++)
		count += (argv[i] ? strlen(argv[i]) : 0) + 1;

	proto.encoding = XA_STRING;
	proto.format = 8;
	proto.value = 0;

	if (count > 0) {
		proto.nitems = count - 1;

		buffer = (char *) Xmalloc(count);

		if (!buffer)
			return 0;

		proto.value = (unsigned char *)buffer;

		for (i = 0; i < argc; i++) {
			if (argv[i]) {
				strcpy(buffer, argv[i]);
				buffer += strlen(argv[i] + 1);
			} else {
				*buffer++ = '\0';
			}
		}
	} else {
		proto.nitems = 0;
		if (!(proto.value = (unsigned char *) Xmalloc(1)))
			return 0;

		*proto.value = '\0';
	}

	memcpy(ret, &proto, sizeof(*ret));
	return Success;
}

/* 
   Just a proxy call to XStringListToTextProperty for the moment.
   Eventually we'll have to deal with the style 
*/
int
XmbTextListToTextProperty(Display *display, char **list, int count, 
	XICCEncodingStyle style, XTextProperty *text_prop_return)
{
	return XStringListToTextProperty(list, count, text_prop_return);
}

/* Ok - we try to convert this before storing.  We should be converting into our current
   locale, blah, blah - but I don't know what to do for that.  So we just convert from
   wchar_t back to char_t and store that.
*/
int
XwcTextListToTextProperty(Display *display, wchar_t **list, int count, 
	XICCEncodingStyle style, XTextProperty *text_prop_return)
{
	char **strs = 0;
	int i, l, ret;

	if (!count)
		return Success;
	
	strs = (char **)Xmalloc(count * sizeof(char *));
	if (!strs)
		return XNoMemory;
	       
	for(i = 0; i < count; i++) {
		wchar_t *wstr = list[i];
		char *ptr = 0;
#ifndef __DJGPP__		
		int len = wcslen(wstr);

		ptr = strs[i] = (char *)Xcalloc(wcslen(wstr) + 1, sizeof(char));
		if (!strs[i])
			continue;

		for(l = 0; l < len; l++) {
			int ch = wctob(*wstr++);
			if (ch != EOF) *ptr++ = ch;
		}
#else
		int len = 1;

		ptr = strs[i] = (char *)Xcalloc(len + 1, sizeof(char));
		if (!strs[i])
			continue;

		for(l = 0; l < len; l++) {
			int ch = *wstr++;
			if (ch != EOF) *ptr++ = ch;
		}

#endif		
	}

	ret = XStringListToTextProperty((char **) strs, count, text_prop_return);

	/* Free the converted functions */
	for(i = 0; i < count; i++) 
		if (strs[i])Xfree(strs[i]);
	
	Xfree(strs);
	return ret;       
}

/* This converts the text property into wide characters.  */
/* FIXME:  This does not take into account unconvertable characters */
int
XwcTextPropertyToTextList(Display* display, const XTextProperty* text_prop,
	wchar_t*** list_return, int* count_return)
{
	char *value = (char *)text_prop->value;
	wchar_t **ret = 0;
	int count = 0;

	/* Nothing to encode */
	if (!text_prop->nitems) {
		*count_return = 0;
		*list_return = 0;
		return Success;
	}

	while(1) {
		wchar_t *rptr;
		char *vptr = value;
		int l;

		if (!*value)
			break;

		if (!ret) 
			ret = (wchar_t **)Xmalloc((count + 1) * sizeof(wchar_t *));
		else
			ret = (wchar_t **)Xrealloc(ret, (count + 1) * sizeof(wchar_t *));

		if (!ret)
			return XNoMemory;
		rptr = ret[count] = (wchar_t *)Xcalloc(sizeof(wchar_t), strlen(value) + 1);

		if (!ret[count]) 
			goto next_string;

		for(l = 0; l < strlen(value); l++) 
#ifndef __DJGPP__		
			*rptr++ = btowc(*vptr++);
#else
            *rptr++ = *vptr++;
#endif		
		*rptr++ = L'\0';
		count++;

next_string:
		value += strlen(value) + 1;
	}

	ret = (wchar_t **)Xrealloc(ret, (count + 1) * sizeof(wchar_t *));
	ret[count] = 0;

	*list_return = ret;
	*count_return = count;

	return Success;
}

void
XwcFreeStringList(wchar_t **list)
{
	int i;

	if (!list)
		return;

	for(i = 0; list[i]; i++)
		Xfree(list[i]);
	Xfree(list);
}
