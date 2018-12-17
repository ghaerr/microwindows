#ifndef _MAIN_H_
#define _MAIN_H_

#include "window.h"
#include "button.h"
#include "label.h"
#include "progressbar.h"
#include "checkbutton.h"
#include "radiobutton.h"
#include "radiobuttongroup.h"
#include "textbox.h"
#include "scrollbar.h"
#include "progressbar.h"
#include "listbox.h"
#include "picture.h"
#include "combobox.h"
#include "rawwidget.h"

/*here*/
#define MAXTOOLS 14 /*here*/
#define WIDGET_PROPS 7
#define MAXCALLBACKS 3

#define TND_CREATE 	0
#define TND_UPDATE 	1
#define TND_SAVE 	2
#define TND_CODEGEN 	3
#define TND_LOAD	4

int projectstatus;
int questionasked;


typedef void (*ToolSelectedFptr) (void);
typedef void (*PropsFptr) (TN_WIDGET *);
typedef void (*PropintFptr) (int);

int counter[MAXTOOLS];
int ToolSelected[MAXTOOLS];
PropintFptr PropEditFunc[MAXTOOLS];
PropsFptr DisplayProps[MAXTOOLS];

TN_WIDGET *main_widget;
TN_WIDGET *active_widget;
TN_WIDGET *proptag,*propval;
TN_WIDGET *callbacks_button;
TN_WIDGET *propedit_button;
TN_WIDGET *deletewidget_button;
TN_WIDGET *propeditwin;
int selected;
TN_WIDGET *pe_nametb,*pe_heighttb,*pe_widthtb,*pe_xpostb,*pe_ypostb,*pe_label_caption;
TN_WIDGET *pe_enabledcb,*pe_visiblecb;
TN_WIDGET *pe_picture_filenametb,*pe_picture_stretchcb;
char pe_string[100];

int pe_ypos;

/*defs for menu editor*/
TN_WIDGET *menueditwin;
TN_WIDGET *me_labellist,*me_namelist,*me_typelist,*me_parentlist;
TN_WIDGET *me_labeltb,*me_nametb,*me_callbacktb,*me_dptrtb,*me_enabledcb;
TN_WIDGET *me_exclusivecb,*me_checkablecb;
TN_WIDGET *me_typecb,*me_parentcb;
TN_WIDGET *me_exclusivelbl,*me_checkablelbl;
struct menumember	
{
	char label[80];
	char name[80];
	char type[30];
	char parentname[80];
	int written;
	char callbackname[80];
	char dptrname[80];
	int enabled;
	int exclusive;
	int checkable;
	struct menumember *next;
};
	
struct mhead
{
	char mbarname[80];
	struct menumember *member;
	struct mhead *down;
};

struct mhead *menuhead;

char strings[MAXTOOLS][20];	  
char btnpixmaps[MAXTOOLS][50];
void ShowProps(TN_WIDGET *);
void widget_clicked(TN_WIDGET *, DATA_POINTER );
void createproplist(TN_WIDGET *);
void window_clicked(TN_WIDGET *,DATA_POINTER );
void selector_clicked(TN_WIDGET *,DATA_POINTER );
void container_clicked(TN_WIDGET *,DATA_POINTER );
void window_selected(TN_WIDGET *,DATA_POINTER );
void Selected(TN_WIDGET *,DATA_POINTER);
void DisplayMessage(char *);
void DispMsgOkClicked(TN_WIDGET *, DATA_POINTER );	

void tndinit(void);
void createtools(TN_WIDGET *,TN_WIDGET **);
void createmainappwin(TN_WIDGET *);

typedef struct dbrecord{
	TN_WIDGET *widget;
	TN_BOOL enabled;
	TN_BOOL visible;

	int count;
	char **callbacks;
	char **funcnames;
	char **dptrnames;
	TN_BOOL written[MAXCALLBACKS];
	int menubarypos;	
	struct dbrecord *next;
	struct dbrecord *down;
} RECORD;

RECORD *head;

char *project_dir,*savefile,*loadfile;
int project_saved;
int code_built;

char *endp; 

typedef void (*WriteFileFptr) (RECORD *,FILE *,int);
WriteFileFptr WriteWidgetToFile[MAXTOOLS];

/*Note u cant add the following stuff to <specific widget>.h as RECORD is not defined b4 them - think of this later...looks ugly right now, eh ? - anyway, here goes...*/
void window_write(RECORD *,FILE *,int);
void label_write(RECORD *,FILE *,int);
void button_write(RECORD *,FILE *,int);
void checkbutton_write(RECORD *,FILE *,int);
void radiobuttongroup_write(RECORD *,FILE *,int);
void radiobutton_write(RECORD *,FILE *,int);
void textbox_write(RECORD *,FILE *,int);
void scrollbar_write(RECORD *,FILE *,int);
void progressbar_write(RECORD *,FILE *,int);
void listbox_write(RECORD *,FILE *,int);
void picture_write(RECORD *,FILE *,int);
void combobox_write(RECORD *,FILE *,int);
void rawwidget_write(RECORD *,FILE *,int);
void menubar_write(RECORD *,FILE *,int);

RECORD *GetFromProject(TN_WIDGET *);
TN_WIDGET *lookupwidget(char *);

void write_main_c_file(FILE *);
void write_callback_c_file(FILE *);
void write_callback_h_file(FILE *);
void write_support_c_file(FILE *);
void write_interface_c_file(FILE *);
void write_interface_h_file(FILE *);
void write_make_file(FILE *);

int DialogBoxWithTextbox(char *,char *,char *);
void build_code(TN_WIDGET *,DATA_POINTER );
void edit_callbacks_clicked(TN_WIDGET *,DATA_POINTER );
void ClearProject(void);
void invoke_menu_editor(TN_WIDGET *,DATA_POINTER);
void radiobutton_clicked(TN_WIDGET *, DATA_POINTER);
void SaveProject(void);
void LoadProject(void);
int loadrc(void);
void InputProjectDir(int);
void invoke_property_editor(RECORD *);
void invoke_callbacks_editor(RECORD *);
void delete_widget_clicked(TN_WIDGET *,DATA_POINTER);
RECORD *AddToProject(TN_WIDGET *,TN_WIDGET *);

#endif /*_MAIN_H_*/
