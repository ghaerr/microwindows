##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Modified for TuxChess, 2002, Steven J. Merrifield
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/tuxchess/
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

ifeq ($(NANOX), Y)
ifeq ($(NANOXDEMO), Y)
ifneq ($(ARCH), LINUX-TCC)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/demos/tuxchess/main.o \
	$(MW_DIR_OBJ)/demos/tuxchess/board.o \
	$(MW_DIR_OBJ)/demos/tuxchess/data.o \
	$(MW_DIR_OBJ)/demos/tuxchess/eval.o \
	$(MW_DIR_OBJ)/demos/tuxchess/search.o \
	$(MW_DIR_OBJ)/demos/tuxchess/images.o

# Requires: MW_FEATURE_IMAGES, HAVE_FILEIO
TARGET = $(MW_DIR_BIN)/tuxchess

all: default $(TARGET)

endif
endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SHAREDLIBS), Y)
$(MW_DIR_BIN)/tuxchess: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(CCNANOXCLIENTLIBS)
else
$(MW_DIR_BIN)/tuxchess: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS)
endif

