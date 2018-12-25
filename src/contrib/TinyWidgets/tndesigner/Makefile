# Generated automatically from Makefile.in by configure.
TOP=..
include $(TOP)/Make.rules

SRCS := $(wildcard *.c) y.tab.c lex.yy.c
OBJS := $(patsubst %.c,%.o,$(SRCS)) 

all: tndesigner

clean:
	@rm -f $(OBJS) y.tab.* lex.yy.c tndesigner *~ core

tndesigner: $(OBJS)
	@echo "Compiling tndesigner..."
ifeq ($(ARCH), MACOSX)	
	$(CC) -o $@ $(OBJS) -L$(LIBDIR) $(LIBS) -I$(INCDIR) -ll
else
	$(CC) -o $@ $(OBJS) -L$(LIBDIR) $(LIBS) -I$(INCDIR) -lfl
endif

lex.yy.c:
	$(LEX) fileio.l
y.tab.c:
	$(YACC) -d fileio.y
