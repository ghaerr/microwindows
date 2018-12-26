#include "../../../include/tnWidgets.h"
TN_WIDGET *lookup_widget(TN_WIDGET *start,char *name);

void on_window_closed(TN_WIDGET *, DATA_POINTER);
void on_7_clicked(TN_WIDGET *, DATA_POINTER);
void on_8_clicked(TN_WIDGET *, DATA_POINTER);
void on_9_clicked(TN_WIDGET *, DATA_POINTER);
void on_4_clicked(TN_WIDGET *, DATA_POINTER);
void on_5_clicked(TN_WIDGET *, DATA_POINTER);
void on_6_clicked(TN_WIDGET *, DATA_POINTER);
void on_1_clicked(TN_WIDGET *, DATA_POINTER);
void on_2_clicked(TN_WIDGET *, DATA_POINTER);
void on_3_clicked(TN_WIDGET *, DATA_POINTER);
void on_0_clicked(TN_WIDGET *, DATA_POINTER);
void on_dot_clicked(TN_WIDGET *, DATA_POINTER);
void on_mul_clicked(TN_WIDGET *, DATA_POINTER);
void on_minus_clicked(TN_WIDGET *, DATA_POINTER);
void on_div_clicked(TN_WIDGET *, DATA_POINTER);
void on_plus_clicked(TN_WIDGET *, DATA_POINTER);
void on_equal_clicked(TN_WIDGET *, DATA_POINTER);
void on_clear_clicked(TN_WIDGET *, DATA_POINTER);
void on_allclear_clicked(TN_WIDGET *, DATA_POINTER);
void append_string_to_textbox(TN_WIDGET *,char *);
double get_number_from_textbox(TN_WIDGET *);
void set_string_to_textbox(TN_WIDGET *,char *);
void perform_operation(void);
void oper_clicked(TN_WIDGET *,int);
void do_reset(TN_WIDGET *);
double number1,number2;
int state;
int oper;
int clearflag;

#define NONE 0
#define PLUS 1
#define MINUS 2
#define MULT 3
#define DIV 4
