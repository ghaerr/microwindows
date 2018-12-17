#include "main.h"
char *ScrollbarTagStrings[SCROLLBAR_PROPS]={"Orientation","Minval","Maxval","Pagestep","Linestep"};
TN_WIDGET *pe_scrollbar_maxvaltb,*pe_scrollbar_minvaltb;
TN_WIDGET *pe_scrollbar_linesteptb,*pe_scrollbar_pagesteptb;
TN_WIDGET *pe_scrollbar_orientationcb;   

void displayscrollbarprops(TN_WIDGET *widget)
{
	/*TODO: add code for showing scrollbar properties here*/
	int i;
	int orientation,minval,maxval,pagestep,linestep;
	char string[80];
	
	for(i=0;i<SCROLLBAR_PROPS;i++)
		tnAddItemToListBox(proptag,ScrollbarTagStrings[i]);
	tnGetScrollBarOrientation(widget,&orientation);
	tnAddItemToListBox(propval,(orientation == TN_VERTICAL)?"TN_VERTICAL":"TN_HORIZONTAL");
	tnGetScrollRange(widget,&minval,&maxval);
	sprintf(string,"%d",minval);
	tnAddItemToListBox(propval,string);
	sprintf(string,"%d",maxval);
        tnAddItemToListBox(propval,string);
	tnGetScrollStepSizes(widget,&pagestep,&linestep);
	sprintf(string,"%d",pagestep);
        tnAddItemToListBox(propval,string);
	sprintf(string,"%d",linestep);
        tnAddItemToListBox(propval,string);
	return;
	
}

/*This code is now obsolete*/
void ScrollBarListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	/*TODO: add code for handling listbox for scrollbar properties*/
}
void scrollbarPropEditFunc(int operation)
{
	char *combostrings[] = { "TN_VERTICAL","TN_HORIZONTAL"};
	char string[80];
	static int minval,maxval;
	static int pagestep,linestep;
	int newminval,newmaxval,newpagestep,newlinestep;
	int orientation;
	char *str;	
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Orientation",TN_END);
			pe_scrollbar_orientationcb = tnCreateWidget(TN_COMBOBOX,propeditwin,85,pe_ypos,TN_LISTITEMS,combostrings,TN_COUNT,2,TN_END);
			tnGetScrollBarOrientation(active_widget,&orientation);
			tnSetSelectedComboItem(pe_scrollbar_orientationcb,(orientation == TN_VERTICAL) ? "TN_VERTICAL":"TN_HORIZONTAL",TN_TRUE);

			pe_ypos += 20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Minval",TN_END);
			tnGetScrollRange(active_widget,&minval,&maxval);
			sprintf(string,"%d",minval);
			pe_scrollbar_minvaltb = tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,string,TN_END);

			pe_ypos += 20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Maxval",TN_END);
			sprintf(string,"%d",maxval);
			pe_scrollbar_maxvaltb = tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,string,TN_END);

			pe_ypos += 20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Pagestep",TN_END);
			tnGetScrollStepSizes(active_widget,&pagestep,&linestep);
			sprintf(string,"%d",pagestep);
			pe_scrollbar_pagesteptb = tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,string,TN_END);

			pe_ypos += 20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Linestep",TN_END);
			sprintf(string,"%d",linestep);
			pe_scrollbar_linesteptb = tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,string,TN_END);
			break;
		
		case TND_UPDATE:
			str=tnGetSelectedComboItem(pe_scrollbar_orientationcb);
			tnSetScrollBarOrientation(active_widget,(strcmp(str,"TN_VERTICAL")==0)?TN_VERTICAL:TN_HORIZONTAL);
			free(str);
			str=tnGetText(pe_scrollbar_minvaltb);
			if(strcmp(str,"")) 
			{
				newminval=(int)strtol(str,&endp,10);
				if(*endp=='\0' && newminval>=0)
					minval=newminval;
					
			}
			else
				minval=0;
			free(str);
			
			str=tnGetText(pe_scrollbar_maxvaltb);
			if(strcmp(str,"")) 
			{
				newmaxval=(int)strtol(str,&endp,10);
				if(*endp=='\0' && newmaxval>=0)
					maxval=newmaxval;
					
			}
			else
				maxval=0;

			free(str);

			tnSetScrollRange(active_widget,minval,maxval);
			
			str=tnGetText(pe_scrollbar_linesteptb);
			if(strcmp(str,"")) 
			{
				newlinestep=(int)strtol(str,&endp,10);
				if(*endp=='\0' && newlinestep>0)
					linestep=newlinestep;
					
			}
			else
				linestep=0;

			free(str);
			
			str=tnGetText(pe_scrollbar_pagesteptb);
			if(strcmp(str,"")) 
			{
				newpagestep=(int)strtol(str,&endp,10);
				if(*endp=='\0' && newpagestep>0)
					pagestep=newpagestep;
					
			}
			else
				pagestep=0;

			free(str);

			tnSetScrollStepSizes(active_widget,pagestep,linestep);
			break;
			
					
	}
}


void scrollbar_write(RECORD *record,FILE *fp,int operation)
{
	int orientation;
	int pagestep,linestep,minval,maxval;
	
	tnGetScrollRange(record->widget,&minval,&maxval);
	tnGetScrollStepSizes(record->widget,&pagestep,&linestep);
	tnGetScrollBarOrientation(record->widget,&orientation);
	
	switch(operation)
	{
		case TND_SAVE:
			
			fprintf(fp,"orientation %d\n",orientation);
			fprintf(fp,"minval %d\n",minval);
			fprintf(fp,"maxval %d\n",maxval);
			fprintf(fp,"pagestep %d\n",pagestep);
			fprintf(fp,"linestep %d\n",linestep);
			
			break;
		case TND_CODEGEN:
			fputs(",TN_ORIENTATION,",fp);
			if(orientation==TN_VERTICAL)
				fputs("TN_VERTICAL",fp);
			else
				fputs("TN_HORIZONTAL",fp);
			fprintf(fp,",TN_MINVAL,%d,TN_MAXVAL,%d,TN_PAGESTEP,%d,TN_LINESTEP,%d",minval,maxval,pagestep,linestep);
			break;
			
	}
}

