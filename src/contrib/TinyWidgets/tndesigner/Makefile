# Generated automatically from Makefile.in by configure.
TOP=..
include $(TOP)/Make.rules

SRCS := $(wildcard *.c) y.tab.c lex.yy.c
#OBJS := $(patsubst %.c,%.o,$(SRCS)) 
OBJS := button.o callback_editor.o checkbutton.o codegen.o combobox.o label.o \
		listbox.o loadsave.o main.o menu_editor.o picture.o progressbar.o \
		property_editor.o radiobutton.o radiobuttongroup.o rawwidget.o scrollbar.o textbox.o window.o \
		y.tab.o lex.yy.o

ifneq ($(DEBUG), Y)
SILENCE_WARNINGS = -Wno-unused-function -Wno-unused-label
endif

all: tndesigner

clean:
	@rm -f $(OBJS) y.tab.* lex.yy.c tndesigner *~ core

tndesigner: $(OBJS)
	@echo "Compiling tndesigner..."
	$(CC) -o $@ $(OBJS) -L$(LIBDIR) $(LIBS) -I$(INCDIR) $(LIBFLEX)

lex.yy.c:
	@echo $(LEX) fileio.l
	$(LEX) fileio.l
y.tab.c:
	@echo $(YACC) -d fileio.y
	$(YACC) -d fileio.y

lex.yy.o: lex.yy.c
	@ echo "Compiling $<..."
	@ $(CC) $(CFLAGS) $(SILENCE_WARNINGS) -o $@ -c $<

y.tab.o: y.tab.c
	@ echo "Compiling $<..."
	@ $(CC) $(CFLAGS) $(SILENCE_WARNINGS) -o $@ -c $<
