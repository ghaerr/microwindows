##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC = ../..
endif
include $(MW_DIR_SRC)/Path.rules

include $(CONFIG)

#all: $(MW_DIR_BIN)/convbmp
#HOSTCC=gcc
#HOSTCFLAGS=-I../../include

############################# targets section ################################

ifeq ($(MICROWIN), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwimages.a
LIBNAMESO = libmwimages.so

# List of objects to compile
OBJS = car8.o min4.o max4.o close4.o under4.o\
	zion208.o cs1.o rle8.o microwin.o penguin.o

all: $(MW_DIR_BIN)/convbmp $(MW_DIR_BIN)/makebmp default

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

realclean: clean
	$(RM) car8.c min4.c max4.c close4.c under4.c zion208.c cs1.c rle8.c \
	microwin.c penguin.c

$(MW_DIR_BIN)/makebmp: $(MW_DIR_SRC)/mwin/bmp/makebmp.c
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

