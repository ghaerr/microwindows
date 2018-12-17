#ifndef _PROGRESSBAR_H_ 
#define _PROGRESSBAR_H_
#define PROGRESSBAR_PROPS 4 
#include "../include/tnWidgets.h"
TN_WIDGET *pe_pbar_fillcolor,*pe_pbar_value,*pe_pbar_style,*pe_pbar_stepsize;
void displayprogressbarprops(TN_WIDGET *);
void ProgressBarListBoxHandler(TN_WIDGET *,DATA_POINTER );
void progressbarPropEditFunc(int);
#endif /*_PROGRESSBAR_H_*/
