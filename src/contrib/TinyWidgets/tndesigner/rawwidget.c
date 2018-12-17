#include "main.h"
void displayrawwidgetprops(TN_WIDGET *widget)
{
	/*TODO: add code for showing rawwidget properties here*/
}
void RawWidgetListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for rawwidget properties*/
}
void rawwidgetPropEditFunc(int operation)
{
	switch(operation)
	{
		case TND_CREATE:
		case TND_UPDATE:
		      break;
	}
}


void rawwidget_write(RECORD *record,FILE *fp,int operation)
{
	switch(operation)
	{
		case TND_SAVE:
			break;
		case TND_CODEGEN:
		        break;
	}
}

