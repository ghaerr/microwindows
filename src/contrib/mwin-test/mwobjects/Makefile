##############################################################################
#
# MW Objects Makefile
#
# (c) Chris Johns 2000
#
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/mwobjects/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS +=
# Defines for preprocessor
DEFINES +=

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Linking flags
LDFLAGS +=

############################# targets section ################################

ifeq ($(__DEPRECATED), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
CXXOBJS :=	$(MW_DIR_OBJ)/demos/mwobjects/mwo-test.o

all: default $(MW_DIR_BIN)/mwo-test

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

$(MW_DIR_BIN)/mwo-test: $(CXXOBJS) $(MWINLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CXX) $(CFLAGS) $(LDFLAGS) $(CXXOBJS) -o $@ $(MWINLIBS) $(LDFLAGS)
