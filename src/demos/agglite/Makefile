##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/agglite/
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

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS := $(MW_DIR_OBJ)/demos/agglite/nxaggdemo.o 
CXXOBJS := $(MW_DIR_OBJ)/demos/agglite/agglite.o

TARGET = $(MW_DIR_BIN)/nxaggdemo

all: default $(TARGET)

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SHAREDLIBS), Y)
$(MW_DIR_BIN)/nxaggdemo: $(OBJS) $(CXXOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJS) $(CXXOBJS) -o $@ $(CCNANOXCLIENTLIBS)
else
$(MW_DIR_BIN)/nxaggdemo: $(OBJS) $(CXXOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJS) $(CXXOBJS) -o $@ $(NANOXCLIENTLIBS)
endif

