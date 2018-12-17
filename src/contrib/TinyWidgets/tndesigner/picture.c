#include "main.h"
void displaypictureprops(TN_WIDGET *widget)
{
	char *filename = NULL;
	TN_BOOL stretch;
	tnGetPictureProps(widget,&stretch,&filename);
	tnAddItemToListBox(proptag,"Stretch");
	tnAddItemToListBox(propval,stretch?"TRUE":"FALSE");
	tnAddItemToListBox(proptag,"Filename");
	if(filename)
	{
		tnAddItemToListBox(propval,filename);
		free(filename);
	}
	return;
		
		
}
void PictureListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for picture properties*/
}
void picturePropEditFunc(int operation)
{
	char *filename = NULL;
	char *comboitems[]={"TN_TRUE","TN_FALSE"};
	TN_BOOL stretch;
	char *string;
	
	switch(operation)
	{
		case TND_CREATE:
			tnGetPictureProps(active_widget,&stretch,&filename);
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Stretch",TN_END);
			pe_picture_stretchcb=tnCreateWidget(TN_COMBOBOX,propeditwin,85,pe_ypos,TN_LISTITEMS,comboitems,TN_COUNT,2,TN_END);
			tnSetSelectedComboItem(pe_picture_stretchcb,stretch?"TN_TRUE":"TN_FALSE",TN_TRUE);
			pe_ypos+=20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Filename",TN_END);
			pe_picture_filenametb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,filename,TN_END);			
			break;

		case TND_UPDATE:
			string = tnGetSelectedComboItem(pe_picture_stretchcb);
			tnSetPictureStretch(active_widget,(strcmp(string,"TN_TRUE"))?TN_FALSE:TN_TRUE);
			free(string);
			string = tnGetText(pe_picture_filenametb);
			if(strcmp(string,""))
				tnSetPicture(active_widget,string);
			free(string);
			break;
	}
}


void picture_write(RECORD *record,FILE *fp,int operation)
{
	char *filename = NULL;
	TN_BOOL stretch;
	tnGetPictureProps(record->widget,&stretch,&filename);
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"stretch %d\n",stretch);
			if(filename)
			{
				fprintf(fp,"filename \"%s\"\n",filename);
				free(filename);
			}
			break;
		case TND_CODEGEN:
			fputs(",TN_STRETCH,",fp);
			fputs(stretch?"TN_TRUE":"TN_FALSE",fp);
			if(filename)
			{
				fputs(",TN_FILENAME,\"",fp);
				fputs(filename,fp);
				fputs("\"",fp);
				free(filename);
			}
			break;
			break;
			
	}
}

