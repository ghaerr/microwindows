#include "main.h"
TN_WIDGET *pe_textbox_texttb;
void displaytextboxprops(TN_WIDGET *widget)
{
	char *text;
	text = tnGetText(widget);
	tnAddItemToListBox(proptag,"Text");
	tnAddItemToListBox(propval,text);
	free(text);
	return;
}
	
void TextBoxListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for textbox properties*/
}
void textboxPropEditFunc(int operation)
{
	char *text;
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Text",TN_END);
			text = tnGetText(active_widget);
			pe_textbox_texttb = tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,text,TN_END);
			free(text);
			break;
			
		case TND_UPDATE:
			text=tnGetText(pe_textbox_texttb);
			tnSetText(active_widget,text);
			break;
	}
}


void textbox_write(RECORD *record,FILE *fp,int operation)
{
	char *text=tnGetText(record->widget);
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"defaulttext \"%s\"\n",text);
			break;
		case TND_CODEGEN:
			/*my stuff*/
			fputs(",TN_DEFAULTTEXT,\"",fp);
			fputs(text,fp);
			fputs("\"",fp);
			break;

	}
	free(text);
}

