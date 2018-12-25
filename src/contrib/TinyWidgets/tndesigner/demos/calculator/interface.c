#include "../../../include/tnWidgets.h"
#include "interface.h"
#include "callback.h"

TN_WIDGET * create_window0(TN_WIDGET *parent)
{
TN_WIDGET *window0;
TN_WIDGET *textbox0;
TN_WIDGET *button7;
TN_WIDGET *button8;
TN_WIDGET *button9;
TN_WIDGET *button4;
TN_WIDGET *button5;
TN_WIDGET *button6;
TN_WIDGET *button1;
TN_WIDGET *button2;
TN_WIDGET *button3;
TN_WIDGET *button0;
TN_WIDGET *button_dot;
TN_WIDGET *button_mul;
TN_WIDGET *button_minus;
TN_WIDGET *button_div;
TN_WIDGET *button_plus;
TN_WIDGET *button_equal;
TN_WIDGET *button_clear;
TN_WIDGET *button_allclear;

window0=tnCreateWidget(TN_WINDOW,parent,3,16,TN_WIDTH,310,TN_HEIGHT,247,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"Calculator",TN_END);
tnAttachData(window0,strdup("window0"));
textbox0=tnCreateWidget(TN_TEXTBOX,window0,44,20,TN_WIDTH,230,TN_HEIGHT,15,TN_ENABLED,TN_FALSE,TN_VISIBLE,TN_TRUE,TN_DEFAULTTEXT,"",TN_END);
tnAttachData(textbox0,strdup("textbox0"));
button7=tnCreateWidget(TN_BUTTON,window0,44,55,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"7",TN_END);
tnAttachData(button7,strdup("button7"));
button8=tnCreateWidget(TN_BUTTON,window0,94,55,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"8",TN_END);
tnAttachData(button8,strdup("button8"));
button9=tnCreateWidget(TN_BUTTON,window0,144,55,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"9",TN_END);
tnAttachData(button9,strdup("button9"));
button4=tnCreateWidget(TN_BUTTON,window0,44,105,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"4",TN_END);
tnAttachData(button4,strdup("button4"));
button5=tnCreateWidget(TN_BUTTON,window0,94,105,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"5",TN_END);
tnAttachData(button5,strdup("button5"));
button6=tnCreateWidget(TN_BUTTON,window0,144,105,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"6",TN_END);
tnAttachData(button6,strdup("button6"));
button1=tnCreateWidget(TN_BUTTON,window0,44,155,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"1",TN_END);
tnAttachData(button1,strdup("button1"));
button2=tnCreateWidget(TN_BUTTON,window0,94,155,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"2",TN_END);
tnAttachData(button2,strdup("button2"));
button3=tnCreateWidget(TN_BUTTON,window0,144,155,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"3",TN_END);
tnAttachData(button3,strdup("button3"));
button0=tnCreateWidget(TN_BUTTON,window0,44,205,TN_WIDTH,80,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"0",TN_END);
tnAttachData(button0,strdup("button0"));
button_dot=tnCreateWidget(TN_BUTTON,window0,144,205,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,".",TN_END);
tnAttachData(button_dot,strdup("button_dot"));
button_mul=tnCreateWidget(TN_BUTTON,window0,194,105,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"*",TN_END);
tnAttachData(button_mul,strdup("button_mul"));
button_minus=tnCreateWidget(TN_BUTTON,window0,244,155,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"-",TN_END);
tnAttachData(button_minus,strdup("button_minus"));
button_div=tnCreateWidget(TN_BUTTON,window0,244,105,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"/",TN_END);
tnAttachData(button_div,strdup("button_div"));
button_plus=tnCreateWidget(TN_BUTTON,window0,194,155,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"+",TN_END);
tnAttachData(button_plus,strdup("button_plus"));
button_equal=tnCreateWidget(TN_BUTTON,window0,194,205,TN_WIDTH,80,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"=",TN_END);
tnAttachData(button_equal,strdup("button_equal"));
button_clear=tnCreateWidget(TN_BUTTON,window0,194,55,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"C",TN_END);
tnAttachData(button_clear,strdup("button_clear"));
button_allclear=tnCreateWidget(TN_BUTTON,window0,244,55,TN_WIDTH,30,TN_HEIGHT,30,TN_ENABLED,TN_TRUE,TN_VISIBLE,TN_TRUE,TN_CAPTION,"AC",TN_END);
tnAttachData(button_allclear,strdup("button_allclear"));
tnRegisterCallBack(window0,CLOSED,on_window_closed,NULL);
tnRegisterCallBack(button7,CLICKED,on_7_clicked,window0);
tnRegisterCallBack(button8,CLICKED,on_8_clicked,window0);
tnRegisterCallBack(button9,CLICKED,on_9_clicked,window0);
tnRegisterCallBack(button4,CLICKED,on_4_clicked,window0);
tnRegisterCallBack(button5,CLICKED,on_5_clicked,window0);
tnRegisterCallBack(button6,CLICKED,on_6_clicked,window0);
tnRegisterCallBack(button1,CLICKED,on_1_clicked,window0);
tnRegisterCallBack(button2,CLICKED,on_2_clicked,window0);
tnRegisterCallBack(button3,CLICKED,on_3_clicked,window0);
tnRegisterCallBack(button0,CLICKED,on_0_clicked,window0);
tnRegisterCallBack(button_dot,CLICKED,on_dot_clicked,window0);
tnRegisterCallBack(button_mul,CLICKED,on_mul_clicked,window0);
tnRegisterCallBack(button_minus,CLICKED,on_minus_clicked,window0);
tnRegisterCallBack(button_div,CLICKED,on_div_clicked,window0);
tnRegisterCallBack(button_plus,CLICKED,on_plus_clicked,window0);
tnRegisterCallBack(button_equal,CLICKED,on_equal_clicked,window0);
tnRegisterCallBack(button_clear,CLICKED,on_clear_clicked,window0);
tnRegisterCallBack(button_allclear,CLICKED,on_allclear_clicked,window0);
return window0;
}

