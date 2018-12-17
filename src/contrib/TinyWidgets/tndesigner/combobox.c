#include "main.h"
void displaycomboboxprops(TN_WIDGET *widget)
{
	/*TODO: add code for showing combobox properties here*/
}
void ComboBoxListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for combobox properties*/
}
void comboboxPropEditFunc(int operation)
{
	switch(operation)
	{
		case TND_CREATE:
		case TND_UPDATE:
			break;
	}
}


void combobox_write(RECORD *record,FILE *fp,int operation)
{
	switch(operation)
	{
		case TND_SAVE:
			break;
		case TND_CODEGEN:
			break;
	}
}

