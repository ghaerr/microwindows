##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

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
LDFLAGS +=

############################# targets section ################################

ifeq ($(MICROWIN), Y)
ifeq ($(MICROWINDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS =	mdemo.o mterm.o malpha.o mtest.o mine.o mtest2.o
MINEIMAGES = mineflag.o mineface.o minefacelost.o minebomb.o minedone.o\
	minehitfalse.o

DEMOS = $(TOP)/bin/mdemo\
	$(TOP)/bin/mterm\
	$(TOP)/bin/malpha\
	$(TOP)/bin/mtest\
	$(TOP)/bin/mtest2\
	$(TOP)/bin/mine

ifneq ($(ARCH), ELKS)
DEMOS += $(TOP)/bin/muserfd
endif

all: default $(DEMOS)

endif
endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

$(TOP)/bin/mdemo: mdemo.o $(MWINLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) mdemo.o -o $@ $(CCMWINLIBS)

$(TOP)/bin/mterm: mterm.o $(MWINLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) mterm.o -o $@ $(CCMWINLIBS)

$(TOP)/bin/malpha: malpha.o $(MWINLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) malpha.o -o $@ $(CCMWINLIBS)

$(TOP)/bin/mtest: mtest.o $(MWINLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) mtest.o -o $@ $(CCMWINLIBS)

$(TOP)/bin/mtest2: mtest2.o $(MWINLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) mtest2.o -o $@ $(CCMWINLIBS)

$(TOP)/bin/mine: mine.o $(MINEIMAGES) $(MWINLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) mine.o $(MINEIMAGES) -o $@ $(CCMWINLIBS)

$(TOP)/bin/muserfd: muserfd.o $(MWINLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) muserfd.o -o $@ $(CCMWINLIBS)

