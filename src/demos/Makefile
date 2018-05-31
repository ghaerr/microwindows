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
# Preprocessor flags OTHER than defines
CPPFLAGS +=
# Linking flags
LDFLAGS +=

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS =

ifeq ($(FBEMULATOR), Y)
all: default $(MW_DIR_BIN)/fbe
endif

#ifeq ($(ARCH), ECOS)
#dirs = nanowm nanox nxkbd nxscribble
#endif
ifeq ($(ARCH), UCLINUX-ARM)
dirs = nxkbd
endif

ifeq ($(ARCH), PSP)
dirs = nanox mwin
endif

ifeq ($(ARCH), ANDROID)
dirs = nanox mwin
endif

######################### Makefile.rules section #############################

#set to one to compile just the programs in the test directory
justtest = 0

ifeq ($(justtest), 1)
default:
	echo "Compiling programs in test"
	-$(MAKE) -C test -f Makefile-gcc
else

include $(MW_DIR_SRC)/Makefile.rules

endif

######################## Tools targets section ###############################

#
# Compilation target for framebuffer emulator tool
#
$(MW_DIR_BIN)/fbe: $(MW_DIR_SRC)/demos/fbe.c
	echo "Building $(patsubst $(MW_DIR_BIN)/%,%,$@) tool ..."
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@ $(X11LDFLAGS)
