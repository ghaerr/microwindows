#include "main.h"
char *ButtonTagStrings[BUTTON_PROPS]={"Face","Caption","Filename"};
TN_WIDGET *pe_button_facecb,*pe_button_captiontb,*pe_button_filenametb;

void setcaption(TN_WIDGET *widget,DATA_POINTER d)
{
	if(!strcmp(tnGetSelectedComboItem(widget),"Caption"))
		tnSetText(pe_button_captiontb,"Button");
}

void displaybuttonprops(TN_WIDGET *widget)
{
	int i;
	char caption[80];
	
	for(i=0;i<BUTTON_PROPS;++i)
		tnAddItemToListBox(proptag,ButtonTagStrings[i]);
	tnAddItemToListBox(propval,(TN_HASPIXMAP(widget))?"Pixmap":"Caption");
	
	tnGetButtonCaption(widget,caption);
	if(!TN_HASPIXMAP(widget))
	{
		 tnAddItemToListBox(propval,caption);
		 tnAddItemToListBox(propval,"NIL");
	}
	else
	{
		tnAddItemToListBox(propval,"NIL");
		tnAddItemToListBox(propval,caption);
	}
	
	
}

void buttonPropEditFunc(int operation)
{
	char *combostrings[]={"Caption","Pixmap"};
	char caption[80];
	char *string;
	
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Face",TN_END);
			pe_button_facecb=tnCreateWidget(TN_COMBOBOX,propeditwin,85,pe_ypos,TN_LISTITEMS,combostrings,TN_COUNT,2,TN_END);
			tnSetSelectedComboItem(pe_button_facecb,(TN_HASPIXMAP(active_widget))?"Pixmap":"Caption",TN_TRUE);

			tnGetButtonCaption(active_widget,caption);
			pe_ypos+=20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Caption",TN_END);
			pe_button_captiontb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_END);
			pe_ypos+=20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Filename",TN_END);
			pe_button_filenametb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_END);
			
			tnSetText(pe_button_captiontb,TN_HASPIXMAP(active_widget)?"NIL":caption);
			tnSetText(pe_button_filenametb,TN_HASPIXMAP(active_widget)?caption:"NIL");	
			tnRegisterCallBack(pe_button_facecb,SELECTED,setcaption,NULL);	
			break;
			

			
		case TND_UPDATE:
			string=tnGetSelectedComboItem(pe_button_facecb);
			if(strcmp(string,"Caption")==0)
			{
				if(TN_HASPIXMAP(active_widget))
					tnRemoveButtonPixmap(active_widget);
				free(string);
				string=tnGetText(pe_button_captiontb);
				tnSetButtonCaption(active_widget,string);
				free(string);
			}
			else
			{
				free(string);
				string=tnGetText(pe_button_filenametb);
				tnSetButtonPixmap(active_widget,string);
				free(string);				
			}
			break;
					
	}
}


/*This code is now obsolete*/
void ButtonListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
/*	int *index;
	int count;
	
	tnGetSelectedListPos(widget,&index,&count);
	printf("You have selected %s",ButtonTagStrings[index[0]]);fflush(stdout);
	free(index);
	return;*/
}

void button_write(RECORD *record,FILE *fp,int operation)
{
	char string[80];
	
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"haspixmap %d\n",TN_HASPIXMAP(record->widget));
			if(TN_HASPIXMAP(record->widget))
			{
				tnGetButtonFilename(record->widget,string);
				fprintf(fp,"filename \"%s\"\n",string);
			}
			else
			{
				tnGetButtonCaption(record->widget,string);
				fprintf(fp,"caption \"%s\"\n",string);

			}
			break;
		case TND_CODEGEN:
			if(TN_HASPIXMAP(record->widget))
			{
				fputs(",TN_PIXMAP,TN_TRUE",fp);
				fputs(",TN_FILENAME,\"",fp);
				tnGetButtonFilename(record->widget,string);
				fprintf(fp,"%s\"",string);
			}
			else
			{
				fputs(",TN_CAPTION,\"",fp);
				tnGetButtonCaption(record->widget,string);
				fprintf(fp,"%s\"",string);
				
			}
			break;	

	}
}

