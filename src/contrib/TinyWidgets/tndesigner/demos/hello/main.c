/* This (main.c) file has been generated automatically by TinyDesigner. This file will be generated only once. Edit according to your need*/
#include "../../../include/tnWidgets.h"
#include "interface.h"
#include "callback.h"

int main(int argc,char *argv[])
{
TN_WIDGET *main_widget;
main_widget=tnAppInitialize(argc,argv);
/*This line has been put so that you see something on the screen on running the program. Remove it if you do not want this to be the case*/
create_window0(main_widget);
tnMainLoop();
return 0;
}
