##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

TOP = $(shell pwd)

# See if the user has a local config file. If so use that else
# use the one provided in the distribution.
ifeq ($(HOME)/microwin/config,$(wildcard $(HOME)/microwin/config))
CONFIG = $(HOME)/microwin/config
else
CONFIG = $(TOP)/config
endif

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
LIBNAMESO =

# List of objects to compile
OBJS =	

# demos should be built after the libs !
dirs = drivers mwin engine fonts nanox

all: default
	-$(MAKE) -C demos
ifeq ($(ARCH), ECOS)
	$(MAKE) -C ecos
endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################
