##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC = ../..
CONFIG = $(MW_DIR_SRC)/config
endif

include $(CONFIG)

ifeq (This file is,broken)
ifeq ($(NANOX), Y)
ifeq ($(NANOXDEMO), Y)

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
LDFLAGS += -lm

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS = animate.o ball.o bat.o brick.o cheat.o draw.o dump.o event.o game.o \
	init.o levels.o loader.o misc.o nbreaker.o powers.o scores.o sprite.o
all: default $(MW_DIR_SRC)/bin/nbreaker

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SHAREDLIBS), Y)
$(MW_DIR_SRC)/bin/nbreaker: $(OBJS) $(NANOXCLIENTLIBS) $(MW_DIR_SRC)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(CCNANOXCLIENTLIBS)
else
$(MW_DIR_SRC)/bin/nbreaker: $(OBJS) $(NANOXCLIENTLIBS) $(MW_DIR_SRC)/config
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS)
endif

endif
endif
endif
