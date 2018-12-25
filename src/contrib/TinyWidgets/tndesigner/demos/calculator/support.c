#include "../../../include/tnWidgets.h"

TN_WIDGET *lookup_widget(TN_WIDGET *start,char *name)
{
	TN_WIDGET_PROPS wprops;
	TN_WIDGET *found=NULL;
	
	if(start==NULL||name==NULL)
		return NULL;

	if(strcmp((char *)tnGetAttachedData(start),name)==0)
		return start;
	else 
	{
		tnGetWidgetProps(start,&wprops);
		found = lookup_widget(wprops.child,name);
		if(found!=NULL)
			return found;		
		
		found=lookup_widget(wprops.sibling,name);
		return found;
	}
}