##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef TOP
TOP = ../..
CONFIG = $(TOP)/config
endif

include $(CONFIG)

#all: $(TOP)/bin/convbmp
#HOSTCC=gcc
#HOSTCFLAGS=-I../../include

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
LIBNAME = libmwimages.a
LIBNAMESO = libmwimages.so

# List of objects to compile
OBJS = car8.o min4.o max4.o close4.o under4.o\
	zion208.o cs1.o rle8.o microwin.o penguin.o

all: $(TOP)/bin/convbmp $(TOP)/bin/makebmp default

convbmp: convbmp.c
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

realclean: clean
	$(RM) car8.c min4.c max4.c close4.c under4.c zion208.c cs1.c rle8.c \
	microwin.c penguin.c

$(TOP)/bin/convbmp: convbmp.c
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

$(TOP)/bin/makebmp: makebmp.c
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@
