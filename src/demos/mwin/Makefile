##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/mwin/
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

ifeq ($(MICROWIN), Y)
ifeq ($(MICROWINDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of images for the mine demo.
MINEIMAGES := \
	$(MW_DIR_OBJ)/demos/mwin/mineflag.o \
	$(MW_DIR_OBJ)/demos/mwin/mineface.o \
	$(MW_DIR_OBJ)/demos/mwin/minefacelost.o \
	$(MW_DIR_OBJ)/demos/mwin/minebomb.o \
	$(MW_DIR_OBJ)/demos/mwin/minedone.o \
	$(MW_DIR_OBJ)/demos/mwin/minehitfalse.o

DEMOS = $(MW_DIR_BIN)/mdemo\
	$(MW_DIR_BIN)/mterm\
	$(MW_DIR_BIN)/malpha\
	$(MW_DIR_BIN)/mtest\
	$(MW_DIR_BIN)/mtest2\
	$(MW_DIR_BIN)/mine

ifneq ($(ARCH), ELKS)
DEMOS += $(MW_DIR_BIN)/muserfd
endif

all: default $(DEMOS)

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

# These demos use a hardcoded link line.
MWIN_DEMOS_WITH_NONSTANDARD_LINK := \
	$(MW_DIR_BIN)/mine

# All other demos use a standard link line, that just
# links against the Microwindows libraries.
MWIN_DEMOS_WITH_STANDARD_LINK := \
	$(filter-out $(MWIN_DEMOS_WITH_NONSTANDARD_LINK),$(DEMOS))

$(MWIN_DEMOS_WITH_STANDARD_LINK): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/mwin/%.o $(MWINLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(CCMWINLIBS)

$(MW_DIR_BIN)/mine: $(MW_DIR_OBJ)/demos/mwin/mine.o $(MINEIMAGES) $(MWINLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(MINEIMAGES) -o $@ $(CCMWINLIBS)

