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
INCLUDEDIRS += -I./bitmaps
# Defines for preprocessor
DEFINES +=

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Preprocessor flags OTHER than defines
CPPFLAGS +=
# Linking flags
LDFLAGS += -lm

############################# targets section ################################

ifeq ($(NANOWM), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS = nxroach.o

all: default $(MW_DIR_BIN)/nxroach

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SHAREDLIBS), Y)
$(MW_DIR_BIN)/nxroach: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(CCNANOXCLIENTLIBS)
else
$(MW_DIR_BIN)/nxroach: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS)
endif
