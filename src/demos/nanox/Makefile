##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef TOP
TOP = ../..
CONFIG = $(TOP)/config
endif

include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS +=
# Defines for preprocessor
DEFINES +=

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Preprocessor flags OTHER than defines
CPPFLAGS +=
# Linking flags
LDFLAGS += -lm

############################# targets section ################################

ifeq ($(NANOX), Y)
ifeq ($(NANOXDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
ifeq ($(ARCH), ECOS)
OBJS = landmine.o
TARGETS = $(TOP)/bin/landmine.o
OBJS += ntetris.o
TARGETS += $(TOP)/bin/ntetris.o
OBJS += world.o
TARGETS += $(TOP)/bin/world.o
else
OBJS =	demo.o \
	move.o \
	landmine.o \
	world.o \
	nxclock.o \
	nxterm.o \
	nxview.o \
	nxlsclients.o \
	nxev.o \
	npanel.o \
	nterm.o \
	demo2.o \
	demo5.o \
	demo6.o \
	t1demo.o \
	ftdemo.o \
	logfont.o \
	slider.o \
	ntetris.o \
	launcher.o \
	nsaver.o \
	getselection.o \
	setselection.o

TARGETS = $(TOP)/bin/demo $(TOP)/bin/move $(TOP)/bin/landmine \
	$(TOP)/bin/world $(TOP)/bin/nxclock $(TOP)/bin/nxterm \
	$(TOP)/bin/nxview $(TOP)/bin/nxlsclients $(TOP)/bin/nxev \
	$(TOP)/bin/npanel $(TOP)/bin/nterm \
	$(TOP)/bin/demo2 $(TOP)/bin/demo5 $(TOP)/bin/demo6 \
	$(TOP)/bin/t1demo $(TOP)/bin/ftdemo $(TOP)/bin/logfont \
	$(TOP)/bin/slider $(TOP)/bin/ntetris $(TOP)/bin/launcher \
	$(TOP)/bin/nsaver $(TOP)/bin/getselection $(TOP)/bin/setselection
endif

all: default $(TARGETS)	
	$(CP) world.map nanogui.ppm ntetris.ppm launcher.cnf tux.gif $(TOP)/bin

endif
endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

ifeq ($(SHAREDLIBS), Y)
LD_NANOXCLIENTLIBS = $(CCNANOXCLIENTLIBS)
else
LD_NANOXCLIENTLIBS = $(NANOXCLIENTLIBS)
endif

######################## Tools targets section ###############################

$(TOP)/bin/demo: demo.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/move: move.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/landmine: landmine.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/world: world.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/nxclock: nxclock.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/nxterm: nxterm.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/nxview: nxview.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/nxlsclients: nxlsclients.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/nxev: nxev.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/npanel: npanel.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/nterm: nterm.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/demo2: demo2.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/demo5: demo5.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/demo6: demo6.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/t1demo: t1demo.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/ftdemo: ftdemo.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/logfont: logfont.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/slider: slider.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/ntetris: ntetris.o $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/launcher: launcher.o $($NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/nsaver: nsaver.o $($NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/getselection: getselection.o $($NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(TOP)/bin/setselection: setselection.o $($NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

ifeq ($(ARCH), ECOS)
# Special build rules for linked in applications
$(TOP)/bin/landmine.o: landmine.o
#	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--retain-symbols-file -Wl,landmine.syms -Wl,--defsym -Wl,landmine_main=main -o XX.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,landmine_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(TOP)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o

$(TOP)/bin/ntetris.o: ntetris.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,ntetris_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(TOP)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o

$(TOP)/bin/world.o: world.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,world_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(TOP)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o
endif

