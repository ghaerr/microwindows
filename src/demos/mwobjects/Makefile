##############################################################################
#
# MW Objects Makefile
#
# (c) Chris Johns 2000
#
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC = ../..
endif
include $(MW_DIR_SRC)/Path.rules

include $(CONFIG)

# Do nothing for RTEMS 
ifneq ($(ARCH), RTEMS)

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

ifeq ($(OBJFRAMEWORK), Y)

# List of objects to compile
CXOBJS = mwo-test.o

all: default $(MW_DIR_BIN)/mwo-test

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

CCMWINLIBS = -lmwobjects -lmwin -lmwinlib -lmwengine -lmwdrivers -lmwfonts -lmwimages -lm

$(MW_DIR_BIN)/mwo-test: mwo-test.o
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(CCMWINLIBS)


endif
