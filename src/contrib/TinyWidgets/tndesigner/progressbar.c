#include "main.h"
char *ProgressBarTagStrings[PROGRESSBAR_PROPS]={"Fill Color","Value","Style","Stepsize"};
TN_WIDGET *pe_pbar_stylecb,*pe_pbar_stepsizetb;

void pesetstepsize(TN_WIDGET *widget,DATA_POINTER d)
{
	if(!strcmp(tnGetText(pe_pbar_stepsizetb),"N/A"))
		tnSetText(pe_pbar_stepsizetb,"10");
}
void displayprogressbarprops(TN_WIDGET *widget)
{
	int i;
	char string[80];
	TN_COLOR color;
	
	for(i=0;i<PROGRESSBAR_PROPS-1;++i)
		tnAddItemToListBox(proptag,ProgressBarTagStrings[i]);
	
	color=tnGetProgressBarFillColor(widget);
	sprintf(string,"%d",color);
	tnAddItemToListBox(propval,string);


	sprintf(string,"%d",tnGetProgressBarValue(widget));
 	tnAddItemToListBox(propval,string);
	
	if(ISDISCRETE(widget))
	{
		tnAddItemToListBox(propval,"Discrete");
		tnAddItemToListBox(proptag,ProgressBarTagStrings[PROGRESSBAR_PROPS-1]);
		sprintf(string,"%d",tnGetProgressBarStepSize(widget));
		tnAddItemToListBox(propval,string);
	}
	else
		tnAddItemToListBox(propval,"Continuous");
	
	tnRegisterCallBack(proptag,CLICKED,ProgressBarListBoxHandler,NULL);
}

void ProgressBarListBoxHandler(TN_WIDGET *widget,DATA_POINTER p)
{
	int *index;
	int count;
	
	tnGetSelectedListPos(widget,&index,&count);
	printf("You have selected %s",ProgressBarTagStrings[index[0]]);fflush(stdout);
	free(index);
	return;
}
void progressbarPropEditFunc(int operation)
{
	char *comboitems[]={"Continuous","Discrete"};
	char *string;
	int color;
	int value;
	switch(operation)
	{
		case TND_CREATE:
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Fill Color",TN_END);
			sprintf(pe_string,"%d",tnGetProgressBarFillColor(active_widget));
			pe_pbar_fillcolor=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);
			pe_ypos+=20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Value",TN_END);
			sprintf(pe_string,"%d",tnGetProgressBarValue(active_widget));
			pe_pbar_value=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_DEFAULTTEXT,pe_string,TN_END);
			pe_ypos+=20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Style",TN_END);
			pe_pbar_stylecb=tnCreateWidget(TN_COMBOBOX,propeditwin,85,pe_ypos,TN_LISTITEMS,comboitems,TN_COUNT,2,TN_END);
			tnSetSelectedComboItem(pe_pbar_stylecb,ISDISCRETE(active_widget)?"Discrete":"Continuous",TN_TRUE);
			pe_ypos+=20;
			tnCreateWidget(TN_LABEL,propeditwin,20,pe_ypos,TN_CAPTION,"Step Size",TN_END);
			pe_pbar_stepsizetb=tnCreateWidget(TN_TEXTBOX,propeditwin,85,pe_ypos,TN_END);
			if(ISDISCRETE(active_widget))
			{
				
				sprintf(pe_string,"%d",tnGetProgressBarStepSize(active_widget));
				tnSetText(pe_pbar_stepsizetb,pe_string);
			}
			else
				tnSetText(pe_pbar_stepsizetb,"N/A");
			pe_ypos+=20;
			tnRegisterCallBack(pe_pbar_stylecb,SELECTED,pesetstepsize,NULL);
			break;
					
		case TND_UPDATE:
			string = tnGetText(pe_pbar_fillcolor);
			if(strcmp(string,""))
			{
				color=(int)strtol(string,&endp,10);
				if(*endp=='\0' && color>=0)
					tnSetProgressBarFillColor(active_widget,color);
			}
			free(string);
			string = tnGetText(pe_pbar_value);
			value=(int)strtol(string,&endp,10);
			if(*endp=='\0' && value>=0)
				tnProgressBarUpdate(active_widget,value);
			free(string);
			string = tnGetSelectedComboItem(pe_pbar_stylecb);
			tnSetProgressBarStyle(active_widget,(strcmp(string,"Discrete"))?TN_FALSE:TN_TRUE);
			free(string);
			if(ISDISCRETE(active_widget))
			{
				string = tnGetText(pe_pbar_stepsizetb);
				tnSetProgressBarStepSize(active_widget,atoi(string));
			}
			break;
	}
	
}


void progressbar_write(RECORD *record,FILE *fp,int operation)
{
	char string[100];
	TN_COLOR color;
	color=tnGetProgressBarFillColor(record->widget);
	switch(operation)
	{
		case TND_SAVE:
			fprintf(fp,"fillcolor %d\n",color);
			fprintf(fp,"discrete %d\n",ISDISCRETE(record->widget));
			if(ISDISCRETE(record->widget))
				fprintf(fp,"stepsize %d\n",tnGetProgressBarStepSize(record->widget));
			break;
		case TND_CODEGEN:
			fputs(",TN_FILLCOLOR,",fp);
			sprintf(string,"%d",color);
			fputs(string,fp);
			if(ISDISCRETE(record->widget))
			{
				fputs(",TN_STYLE,TN_DISCRETE,TN_STEPSIZE,",fp);
				sprintf(string,"%d",tnGetProgressBarStepSize(record->widget));
				fputs(string,fp);
			}
				
		break;

	}
}

