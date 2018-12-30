#include "main.h"
char *LabelTagStrings[LABEL_PROPS]={"Caption","Caption Color"};
TN_WIDGET *pe_label_captiontb,*pe_label_captioncolortb;

void displaylabelprops(TN_WIDGET *widget)
{
	int i;
	char caption[100],temp[10];
	int color;
	
	for(i=0;i<LABEL_PROPS;++i)
		tnAddItemToListBox(proptag,LabelTagStrings[i]);
	
 	tnGetLabelCaption(widget,caption);
	tnAddItemToListBox(propval,caption);
	
	color=tnGetLabelCaptionColor(widget);
	sprintf(temp,"%d",color);
	tnAddItemToListBox(propval,temp);
}

void labelPropEditFunc(int operation)
{
	char *string;
	int color;
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Caption",TN_END);
			tnGetLabelCaption(active_widget,pe_string);
			pe_label_captiontb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);
			pe_ypos+=20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Caption Color",TN_END);
			sprintf(pe_string,"%d",tnGetLabelCaptionColor(active_widget));
			pe_label_captioncolortb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);
			pe_ypos+=20;
			break;
			
		case TND_UPDATE:
			string=tnGetText(pe_label_captiontb);
			if(strcmp(string,""))
				tnSetLabelCaption(active_widget,string);
			free(string);
			string=tnGetText(pe_label_captioncolortb);
			
			if(strcmp(string,""))
			{
				color=(int)strtol(string,&endp,10);
				if(*endp=='\0' && color >=0)
					tnSetLabelCaptionColor(active_widget,color);
			}
			free(string);
			break;
	}
}

void label_write(RECORD *record,FILE *fp,int operation)
{
	char string[100];
	TN_COLOR color;
	tnGetLabelCaption(record->widget,string);
	color=tnGetLabelCaptionColor(record->widget);
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"caption \"%s\"\n",string);
			fprintf(fp,"fgcolor %d\n",color);
			break;
		case TND_CODEGEN:
			fputs(",TN_CAPTION,\"",fp);
			fputs(string,fp);
			fputs("\",TN_FGCOLOR,",fp);
			sprintf(string,"%d",color);
			fputs(string,fp);
			break;
	}
}
