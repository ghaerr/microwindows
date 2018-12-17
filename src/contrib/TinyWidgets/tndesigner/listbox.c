#include "main.h"
TN_WIDGET *pe_listbox_resizecb;

void displaylistboxprops(TN_WIDGET *widget)
{
	TN_BOOL resize;
	tnGetListBoxResize(widget,&resize);
	tnAddItemToListBox(proptag,"Resize");
	tnAddItemToListBox(propval,resize?"TRUE":"FALSE");
	return;
}

void ListBoxListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for listbox properties*/
}
void listboxPropEditFunc(int operation)
{
	char *combostrings[] = {"TN_TRUE","TN_FALSE"};
	TN_BOOL resize;
	char *string;
	
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Resize",TN_END);
			pe_listbox_resizecb = tnCreateWidget(TN_COMBOBOX,propeditwin,85,pe_ypos,TN_LISTITEMS,combostrings,TN_COUNT,2,TN_END);
			tnGetListBoxResize(active_widget,&resize);
			tnSetSelectedComboItem(pe_listbox_resizecb,resize ? "TN_TRUE":"TN_FALSE",TN_TRUE);
			break;
			
			
		case TND_UPDATE:
			string = tnGetSelectedComboItem(pe_listbox_resizecb);
			tnSetListBoxResize(active_widget,(strcmp(string,"TN_TRUE"))?TN_FALSE:TN_TRUE);
			free(string);
			break;
	}
	
}


void listbox_write(RECORD *record,FILE *fp,int operation)
{
	TN_BOOL resize;
	tnGetListBoxResize(record->widget,&resize);	
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"resize %d\n",resize);
			break;

		case TND_CODEGEN:
			fputs(",TN_RESIZE,",fp);
			fputs(resize?"TN_TRUE":"TN_FALSE",fp);
			break;
			
	}
}

