##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/vnc/vncviewer/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS += -I$(MW_DIR_SRC)/demos/vnc/include
# Defines for preprocessor
DEFINES += -DNANOX=1

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Preprocessor flags OTHER than defines
CPPFLAGS +=
# Linking flags
LDFLAGS +=

############################# targets section ################################

ifeq ($(NANOX), Y)
ifeq ($(NANOXDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/demos/vnc/vncviewer/args.o \
	$(MW_DIR_OBJ)/demos/vnc/vncviewer/rfbproto.o \
	$(MW_DIR_OBJ)/demos/vnc/vncviewer/sockets.o \
	$(MW_DIR_OBJ)/demos/vnc/vncviewer/vncviewer.o \
	$(MW_DIR_OBJ)/demos/vnc/vncviewer/nanox.o
	
all: default $(MW_DIR_BIN)/vnc

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

NANOXCLIENTLIBS += $(MW_DIR_LIB)/libvncauth.a


$(MW_DIR_BIN)/vnc: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS)

