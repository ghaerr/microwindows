##############################################################################
#
# MW Objects Makefile
#
# (c) Chris Johns 2000
#
##############################################################################

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

all: default $(TOP)/bin/mwo-test

endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

CCMWINLIBS = -lmwobjects -lmwin -lmwinlib -lmwengine -lmwdrivers -lmwfonts -lmwimages -lm

$(TOP)/bin/mwo-test: mwo-test.o
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(CCMWINLIBS)


endif
