#include<stdio.h>
#include "../include/tnWidgets.h"
TN_WIDGET *rdbtn[3];

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}

void u_func(TN_WIDGET *c, DATA_POINTER p)
{
	int i;
	GR_COORD x,y;
	if(tnGetRadioButtonGroupClickedPos(c,&x,&y))
	{
		printf("\nYou clicked outside the radio buttons at position: X=%d,Y=%d (Unused=-1)\n",x,y);
		fflush(stdout);
	}
	
	for(i=0; i < 3; i++)
		if(TN_RADIOBUTTONACTIVE(rdbtn[i]))
		{
		      printf("You have selected Option %d\n",i+1);
	      	      break;
		}
	return;
}



int main(int argc,char **argv)
{
	
	TN_WIDGET *main_widget,*window1,*rdbtngrp;
	
	main_widget=tnAppInitialize(argc,argv);
	window1=tnCreateWidget(TN_WINDOW,main_widget,50,50,TN_HEIGHT,300,TN_WIDTH,400,TN_CAPTION,"Radio Test",TN_END);
	rdbtngrp=tnCreateWidget(TN_RADIOBUTTONGROUP,window1,50,50,TN_CAPTION,"Button Group",TN_HEIGHT,60,TN_WIDTH,80,TN_END);
	rdbtn[0]=tnCreateWidget(TN_RADIOBUTTON,rdbtngrp,10,13,TN_CAPTION,"First",TN_END);
	rdbtn[1]=tnCreateWidget(TN_RADIOBUTTON,rdbtngrp,10,27,TN_CAPTION,"Second",TN_END);
	rdbtn[2]=tnCreateWidget(TN_RADIOBUTTON,rdbtngrp,10,40,TN_CAPTION,"Third",TN_END);
	
	tnRegisterCallBack(rdbtngrp,CLICKED,u_func,NULL);
	tnRegisterCallBack(rdbtngrp,SELECTED,u_func,NULL);
	tnRegisterCallBack(window1,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}

