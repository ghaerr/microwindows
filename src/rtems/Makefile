##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
#
# Rosimildo da Silva, Andrey Astafiev
# Adapted to the RTEMS environment...
#
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := rtems/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
# Defines for preprocessor
DEFINES +=
# Defines for preprocessor
ifeq ($(LINK_APP_INTO_SERVER), Y)
DEFINES += -DNONETWORK=1
endif

# Compilation flags for C files OTHER than include directories
# CFLAGS += $(INCLUDEDIRS)
# Preprocessor flags OTHER than defines
# CPPFLAGS += $(INCLUDEDIRS)
# Linking flags
LDFLAGS +=


# PC386 specific stuff...
PROJECT_RELEASE     = $(RTEMS_BUILD)/$(RTEMS_TARGET)/$(RTEMS_BSP)
PROJECT_TOOLS       = $(PROJECT_RELEASE)/build-tools
START16FILE         = $(PROJECT_RELEASE)/lib/start16.bin
RELOCADDR           = 0x00100000
START16ADDR         = 0x00097C00
HEADERADDR          = 0x00097E00
EXTRA_LFLAGS        =-Wl,-Ttext,$(RELOCADDR)

############################# targets section ################################

ifeq ($(ARCH), RTEMS)

# If you want to create a library with the objects files, define the name here
LIBNAME =


# Name of the target which the image will be created to the downloaded to the
# target box.
RTEMS_TARGET=i386-rtems


# Main module -- this module defines the "main()" routine to get compiled.
# It basically selects which application is actually built to be loaded as
# the RTEMS system. 
# For example, select one:
# MAIN_OBJ = $(MW_DIR_OBJ)/rtems/mw_uid_test.o         -- Mouse & KBD test
# MAIN_OBJ = $(MW_DIR_OBJ)/demos/nanox/demo5.o         -- Nano-X demo
# MAIN_OBJ = $(MW_DIR_OBJ)/demos/mwobjects/mwo-test.o  -- C++ demo
# MAIN_OBJ = $(MW_DIR_OBJ)/demos/test/test.o           -- test driver

# If you want to build the "mineswipper" game, use the modules below.
# This should give you an idea of what you have to do to the others demos.
MINEIMAGES = $(MW_DIR_OBJ)/demos/mwin/mineflag.o \
		$(MW_DIR_OBJ)/demos/mwin/mineface.o \
	 	$(MW_DIR_OBJ)/demos/mwin/minefacelost.o \
		$(MW_DIR_OBJ)/demos/mwin/minebomb.o \
		$(MW_DIR_OBJ)/demos/mwin/minedone.o \
		$(MW_DIR_OBJ)/demos/mwin/minehitfalse.o
MAIN_OBJ = $(MW_DIR_OBJ)/demos/mwin/mine.o $(MINEIMAGES)

# List of objects always needed by a RTEMS application
OBJS =	$(MW_DIR_OBJ)/rtems/rtems_init.o 

# IMPORTANT NOTE: if MAIN_OBJ is a C++ code, add it to CXXOBJS instead.
OBJS += $(MAIN_OBJ)
# CXXOBJS += $(MAIN_OBJ)

all: default $(RTEMS_TARGET).bt


######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
# let's adjust the libraries to match the API that you are using.

NLIBS =

ifeq ($(NANOX), Y)
NLIBS += -lnano-X
endif

ifeq ($(NWIDGET), Y)
NLIBS += -lnwidget
endif

ifeq ($(OBJFRAMEWORK), Y)
NLIBS += -lmwobjects
endif

ifeq ($(MICROWIN), Y)
NLIBS += -lmwin -lmwinlib
endif

NLIBS += -lmwengine -lmwdrivers -lmwfonts -lmwimages -lm

ifeq ($(VGALIB), Y)
NLIBS += -lvga
endif

$(RTEMS_TARGET).bt: $(RTEMS_TARGET).exe
	$(OBJCOPY) -O binary $(RTEMS_TARGET).exe $(RTEMS_TARGET).bin;
	$(PROJECT_TOOLS)/bin2boot -v $(RTEMS_TARGET).bt $(HEADERADDR) $(START16FILE) \
	$(START16ADDR) 0 $(RTEMS_TARGET).bin $(RELOCADDR) 0;
	rm $(RTEMS_TARGET).bin
	mv *.exe *.bt $(MW_DIR_OBJ)/rtems

$(RTEMS_TARGET).exe: $(OBJS) $(CXXOBJS)
	$(CXX) $(CFLAGS) $(INCLUDEDIRS) $(LDFLAGS) $(EXTRA_LFLAGS) $^ -o $@ $(NLIBS)

endif
