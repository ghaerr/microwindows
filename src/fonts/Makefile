##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef TOP
TOP = ..
CONFIG = $(TOP)/config
endif

include $(CONFIG)

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwfonts.a
#LIBNAMESO = libmwfonts.so

# List of objects to compile
OBJS =	rom8x16.o rom8x8.o winFreeSansSerif11x13.o winFreeSystem14x16.o X6x13.o
#OBJS += X5x7.o helvB10.o helvB12.o helvR10.o symbol.o timBI18.o

ifeq ($(HAVE_BIG5_SUPPORT), Y)
OBJS += chinese/big5font.o
endif

ifeq ($(HAVE_GB2312_SUPPORT), Y)
OBJS += chinese/gb2312font.o
endif

ifeq ($(HAVE_JISX0213_SUPPORT), Y)
#OBJS += japanese/jisx0213-16x16.o
OBJS +=japanese/jisx0213-12x12.o
endif

ifeq ($(HAVE_KSC5601_SUPPORT), Y)
OBJS += korean/jo16x16.o korean/ksjo_tbl.o
endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

realclean: clean
	$(RM) helvB10.c helvB12.c helvR10.c timBI18.c X5x7.c X6x13.c
