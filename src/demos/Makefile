##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := demos/
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

ifeq ($(FBEMULATOR), Y)
all: default $(MW_DIR_BIN)/fbe
endif

ifeq ($(ARCH), PSP)
dirs = nanox mwin
endif

ifeq ($(ARCH), ANDROID)
dirs = nanox mwin
endif

ifeq ($(ARCH), AQUILA)
dirs = nanox mwin nuklear
endif

ifeq ($(ARCH), COSMO)
dirs = nanox nuklear
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

#
# Compilation target for framebuffer emulator tool
#
$(MW_DIR_BIN)/fbe: $(MW_DIR_SRC)/demos/fbe.c
	echo "Building $(patsubst $(MW_DIR_BIN)/%,%,$@) tool ..."
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@ $(HOSTLDFLAGS)
