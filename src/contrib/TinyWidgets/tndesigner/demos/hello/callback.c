/*This (callback.c) file has been generated automatically by TinyDesigner. This file will always be APPENDED and NEVER OVERWRITTEN. Add code for callbacks in this file*/
#include "../../../include/tnWidgets.h"
#include "callback.h"

void on_button_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	/*TODO: Add code for callback here*/
	TN_WIDGET *textbox;
	textbox=lookup_widget((TN_WIDGET *)dptr,"textbox0");
	if(!textbox)
		exit(1);
	tnSetText(textbox,"Hello World!");
}
void on_window_close(TN_WIDGET *widget, DATA_POINTER dptr)
{
	/*TODO: Add code for callback here*/
	tnDestroyWidget(widget);
	tnEndApp();
}
void on_clear_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	/*TODO: Add code for callback here*/
	tnSetText((TN_WIDGET *)dptr,"");
}
