#include "../../../include/tnWidgets.h"
#include "interface.h"
#include "callback.h"

int main(int argc,char *argv[])
{
TN_WIDGET *main_widget;
TN_WIDGET *window0;
main_widget=tnAppInitialize(argc,argv);
window0=create_window0(main_widget);
do_reset(window0);
tnMainLoop();
return 0;
}
