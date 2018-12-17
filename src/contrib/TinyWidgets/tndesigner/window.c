#include "main.h"
char *WindowTagStrings[WINDOW_PROPS]={"Title"};
TN_WIDGET *pe_window_titletb;

void displaywindowprops(TN_WIDGET *widget)
{
	/*TODO: add code for showing window properties here*/
	char caption[80];
	int i;
	
	for(i=0;i<WINDOW_PROPS;++i)
		tnAddItemToListBox(proptag,WindowTagStrings[i]);
	
	tnGetWindowTitle(widget,caption);
	tnAddItemToListBox(propval,caption);
		
}
void WindowListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
}

void windowPropEditFunc(int operation)
{
	char caption[80];
	char *string;
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Title",TN_END);
			tnGetWindowTitle(active_widget,caption);
			pe_window_titletb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,caption,TN_END);
			break;
		case TND_UPDATE:
			string=tnGetText(pe_window_titletb);
			if(strcmp(string,""))
			   tnSetWindowTitle(active_widget,string);
			free(string);
			break;
	}
}

void window_write(RECORD *record,FILE *fp,int operation)
{
	char caption[80];
	tnGetWindowTitle(record->widget,caption);
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"caption \"%s\"\n",caption);
			fprintf(fp,"menubarypos %d\n",record->menubarypos);
			break;
		case TND_CODEGEN:
			fputs(",TN_CAPTION,\"",fp);
			fputs(caption,fp);
			fputs("\"",fp);
			break;
			
	}
	return;
}


