##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef TOP
TOP = ..
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
LIBNAME = libmwfonts.a
LIBNAMESO = libmwfonts.so

# List of objects to compile
OBJS =	helvB10.o helvB12.o helvR10.o symbol.o rom8x16.o rom8x8.o\
	winFreeSansSerif11x13.o winFreeSystem14x16.o X5x7.o X6x13.o
#	timBI18.o

ifeq ($(HAVEMSFONTS), Y)
OBJS += winMSSansSerif11x13.o winSystem14x16.o winTerminal8x12.o
endif

ifeq ($(HAVE_BIG5_SUPPORT), Y)
OBJS += chinese/big5font.o
endif

ifeq ($(HAVE_GB2312_SUPPORT), Y)
OBJS += chinese/gb2312font.o
endif

%.c:%.bdf
	echo "Generating $@ from font file ..."
	./convbdf $< > $@

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

realclean: clean
	$(RM) helvB10.c helvB12.c helvR10.c timBI18.c X5x7.c X6x13.c
