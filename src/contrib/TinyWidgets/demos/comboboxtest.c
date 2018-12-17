#include"../include/tnWidgets.h"

TN_WIDGET *combo2;

void endapp(TN_WIDGET *widget,DATA_POINTER d)
{
	tnDestroyWidget(widget);
	tnEndApp();
}
void buttonfunc(TN_WIDGET *button,DATA_POINTER dptr)
{
	TN_WIDGET *combo = (TN_WIDGET *)dptr;
	char *s = tnGetSelectedComboItem(combo);
	if(!s)
		return;
	tnAddItemToComboBox(combo2,s);
	tnDeleteItemFromComboBox(combo,s);
	free(s);
	return;
}

int main(int argc,char **argv)
{
	TN_WIDGET *main_app,*window,*combo1,*button;
	char *s[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	main_app=tnAppInitialize(argc,argv);
	window=tnCreateWidget(TN_WINDOW,main_app,50,50,TN_CAPTION,"Combo Box Test",TN_HEIGHT,300,TN_WIDTH,400,TN_END);
	combo1=tnCreateWidget(TN_COMBOBOX,window,50,50,TN_LISTITEMS,s,TN_COUNT,12,TN_END);
	combo2=tnCreateWidget(TN_COMBOBOX,window,230,50,TN_END);
	button=tnCreateWidget(TN_BUTTON,window,170,50,TN_HEIGHT,20,TN_WIDTH,30,TN_CAPTION,">",TN_END);
	//button=tnCreateWidget(TN_BUTTON,window,170,50,TN_FONTNAME, "cour", TN_FONTSIZE,20,TN_HEIGHT,20,TN_WIDTH,30,TN_CAPTION,"Hello",TN_END);
	tnRegisterCallBack(button,CLICKED,buttonfunc,combo1);
	tnRegisterCallBack(window,CLOSED,endapp,NULL);
	tnMainLoop();
	return 0;
}
