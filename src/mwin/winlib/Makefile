##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef TOP
TOP = ../..
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

ifeq ($(MICROWIN), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwinlib.a
LIBNAMESO = libmwinlib.so

# List of objects to compile
OBJS = draw3d.o ptinsid.o insetr.o fastfill.o graph3d.o mwuser.o

ifneq ($(ARCH), ELKS)
OBJS += button.o edit.o listbox.o progbar.o static.o combobox.o caret.o \
	scrlbar.o medit.o
endif

endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

