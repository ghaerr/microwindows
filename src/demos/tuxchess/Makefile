##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Modified for TuxChess, 2002, Steven J. Merrifield
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

ifeq ($(NANOWM), Y)
ifneq ($(ARCH), LINUX-TCC)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS = main.o board.o data.o eval.o search.o images.o

# Requires: MW_FEATURE_IMAGES, HAVE_FILEIO
TARGET = $(TOP)/bin/tuxchess

all: default $(TARGET)

endif
endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SHAREDLIBS), Y)
$(TOP)/bin/tuxchess: $(OBJS) $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(CCNANOXCLIENTLIBS)
else
$(TOP)/bin/tuxchess: $(OBJS) $(NANOXCLIENTLIBS) $(TOP)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS)
endif

