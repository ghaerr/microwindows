# Generated automatically from Makefile.in by configure.
TOP=..
include $(TOP)/Make.rules

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))
DEPS := $(patsubst %.o,%.d,$(OBJS))
DEMOS := $(patsubst %.c,%,$(SRCS))
DSYMS := $(patsubst %.c,%.dSYM,$(SRCS))

all: $(DEMOS)

clean:
	@rm -f $(DEMOS) $(DEPS) *~ core
	@rm -rf $(DSYMS)

$(DEMOS): $(DEPS)
	@echo "Compiling demos..."
	@ for demo in $(DEMOS); do $(CC) $(CFLAGS) -o $$demo $$demo.c -L$(LIBDIR) $(LIBS); done
       
depend: $(DEPS)
	@echo "Dependencies are now up to date"
