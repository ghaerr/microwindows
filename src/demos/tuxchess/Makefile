##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Modified for TuxChess, 2002, Steven J. Merrifield
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

ifeq ($(NANOWM), Y)
ifneq ($(ARCH), LINUX-TCC)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS = main.o board.o data.o eval.o search.o images.o

# Requires: MW_FEATURE_IMAGES, HAVE_FILEIO
TARGET = $(MW_DIR_BIN)/tuxchess

all: default $(TARGET)

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SHAREDLIBS), Y)
$(MW_DIR_BIN)/tuxchess: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(CCNANOXCLIENTLIBS)
else
$(MW_DIR_BIN)/tuxchess: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS)
endif

