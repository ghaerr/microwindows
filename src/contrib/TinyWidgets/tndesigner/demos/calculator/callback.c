#include "../../../include/tnWidgets.h"
#include "callback.h"

void on_window_closed(TN_WIDGET *widget, DATA_POINTER dptr)
{
	tnDestroyWidget(widget);
	tnEndApp();
}
void on_7_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"7");
	return;
}
void on_8_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"8");
	return;
}
void on_9_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"9");
	return;
}
void on_4_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"4");
	return;
}
void on_5_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"5");
	return;
}
void on_6_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"6");
	return;
}
void on_1_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"1");
	return;
}
void on_2_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"2");
	return;
}
void on_3_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"3");
	return;
}
void on_0_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,"0");
	return;
}
void on_dot_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	append_string_to_textbox((TN_WIDGET *)dptr,".");
	return;
}
void on_mul_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	oper_clicked((TN_WIDGET *)dptr,MULT);
}
void on_minus_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	oper_clicked((TN_WIDGET *)dptr,MINUS);
}
void on_div_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	oper_clicked((TN_WIDGET *)dptr,DIV);
}
void on_plus_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	oper_clicked((TN_WIDGET *)dptr,PLUS);
}
void on_equal_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	char outstr[80];
	
	switch(state)
	{
		case 0:	oper=NONE;
			clearflag=1;
			break;
			
		case 1:
			number2=get_number_from_textbox((TN_WIDGET *)dptr);
			perform_operation();
			sprintf(outstr,"%lf",number1);
			set_string_to_textbox((TN_WIDGET *)dptr,outstr);
			clearflag=1;
			oper=NONE;
			state=0;
			break;
	}
			
	return;		
}

void on_clear_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	switch(state)
	{
		case 0:
			number1=0;
			clearflag=1;
			oper=NONE;
			set_string_to_textbox((TN_WIDGET *)dptr,"0");
			break;
		case 1:
			number2=0;
			clearflag=1;
			set_string_to_textbox((TN_WIDGET *)dptr,"0");
			break;
	}
	return;
}
void on_allclear_clicked(TN_WIDGET *widget, DATA_POINTER dptr)
{
	do_reset((TN_WIDGET *)dptr);
	return;
}

void do_reset(TN_WIDGET *window)
{
	number1=number2=0;
	clearflag=1;
	state=0;
	oper=NONE;
	set_string_to_textbox(window,"0");
	return;
}

void append_string_to_textbox(TN_WIDGET *window,char *c)
{
	
	char *instr;
	char *outstr;
	TN_WIDGET *textbox;
	
	textbox=lookup_widget(window,"textbox0");
	if(textbox==NULL)
	{
		printf("Unable to find widget!\n");
		exit(1);
	}
	instr=tnGetText(textbox);
	
	outstr=(char *)malloc((strlen(instr)+2)*sizeof(char));
	if(clearflag)
	{
		strcpy(outstr,"");
		clearflag=0;
	}
	else
		strcpy(outstr,instr);
	
	if((strcmp(c,"."))==0)
	{
		if((strchr(outstr,'.'))==NULL)
			strcat(outstr,c);
	}
	else
		strcat(outstr,c);
	tnSetText(textbox,outstr);
	
	free(instr);
	free(outstr);
	return;	
}

double get_number_from_textbox(TN_WIDGET *window)
{
	TN_WIDGET *textbox;
	char *text;
	double number;
	
	textbox=lookup_widget(window,"textbox0");
        if(textbox==NULL)
        {
		printf("Unable to find widget!\n");
		exit(1);
	}
	
	text=tnGetText(textbox);
	number=atof(text);
	free(text);
	return number;
}

void set_string_to_textbox(TN_WIDGET *window,char *string)
{
	TN_WIDGET *textbox;
	
	textbox=lookup_widget(window,"textbox0");
        if(textbox==NULL)
        {
		printf("Unable to find widget!\n");
		exit(1);
	}
	tnSetText(textbox,string);
	return;
}

void perform_operation(void)
{
	switch(oper)
	{
		case NONE: break;
		case PLUS: number1+=number2;
			   break;
		case MINUS:number1-=number2;
			   break;
		case MULT: number1*=number2;
			   break;
		case DIV: number1/=number2;
			  break;
	}
	return;
}

void oper_clicked(TN_WIDGET *window,int op)
{
	char outstr[80];
	
	switch(state)
	{
		case 0:	number1=get_number_from_textbox(window);
			set_string_to_textbox(window,"0");
			state=1;
			oper=op;
			clearflag=1;
			break;
			
		case 1:	if(clearflag)
				{
					oper=op;
					break;
				}
			
			number2=get_number_from_textbox(window);
			perform_operation();
			sprintf(outstr,"%lf",number1);
			set_string_to_textbox(window,outstr);
			clearflag=1;
			oper=op;
			break;
	}
			
	return;		
}

