##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC = ../..
endif
include $(MW_DIR_SRC)/Path.rules
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

DEMOS = $(MW_DIR_BIN)/mdemo\
	$(MW_DIR_BIN)/mterm\
	$(MW_DIR_BIN)/malpha\
	$(MW_DIR_BIN)/mtest\
	$(MW_DIR_BIN)/mtest2\
	$(MW_DIR_BIN)/mine

ifneq ($(ARCH), ELKS)
DEMOS += $(MW_DIR_BIN)/muserfd
endif

all: default $(DEMOS)

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

$(MW_DIR_BIN)/mdemo: mdemo.o $(MWINLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) mdemo.o -o $@ $(CCMWINLIBS)

$(MW_DIR_BIN)/mterm: mterm.o $(MWINLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) mterm.o -o $@ $(CCMWINLIBS)

$(MW_DIR_BIN)/malpha: malpha.o $(MWINLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) malpha.o -o $@ $(CCMWINLIBS)

$(MW_DIR_BIN)/mtest: mtest.o $(MWINLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) mtest.o -o $@ $(CCMWINLIBS)

$(MW_DIR_BIN)/mtest2: mtest2.o $(MWINLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) mtest2.o -o $@ $(CCMWINLIBS)

$(MW_DIR_BIN)/mine: mine.o $(MINEIMAGES) $(MWINLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) mine.o $(MINEIMAGES) -o $@ $(CCMWINLIBS)

$(MW_DIR_BIN)/muserfd: muserfd.o $(MWINLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) muserfd.o -o $@ $(CCMWINLIBS)

