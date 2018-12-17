# Generated automatically from Makefile.in by configure.
# /src Makefile.in
# For the Tiny Widgets Project

TOP=..
include $(TOP)/Make.rules

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))
DEPS := $(patsubst %.o,%.d,$(OBJS))

all: $(OBJS) $(TWLIB)

clean:
	@ rm -f $(OBJS) $(TWLIB) $(DEPS) *~ core

$(TWLIB): $(DEPS) $(OBJS)
	@ echo "Building $(TWLIB)..."
	@ $(AR) rc $(TWLIB) $(OBJS)
	@ $(RANLIB) $(TWLIB)
	@ rm -f $(TOP)/demos/*.d

depend: $(DEPS)
	@echo "Dependencies are now up to date"

-include $(DEPS)
