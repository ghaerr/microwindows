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

all: default $(MW_DIR_BIN)/mwo-test

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

CCMWINLIBS = -lmwobjects -lmwin -lmwimages -lm

$(MW_DIR_BIN)/mwo-test: $(MW_DIR_OBJ)/demos/mwobjects/mwo-test.o
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(CCMWINLIBS)


endif
