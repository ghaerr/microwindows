##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := fonts/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwfonts.a
#LIBNAMESO = libmwfonts.so

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/fonts/rom8x16.o \
	$(MW_DIR_OBJ)/fonts/rom8x8.o \
	$(MW_DIR_OBJ)/fonts/winFreeSansSerif11x13.o \
	$(MW_DIR_OBJ)/fonts/winFreeSystem14x16.o \
	$(MW_DIR_OBJ)/fonts/X6x13.o

#	$(MW_DIR_OBJ)/fonts/X5x7.o
#	$(MW_DIR_OBJ)/fonts/helvB10.o
#	$(MW_DIR_OBJ)/fonts/helvB12.o
#	$(MW_DIR_OBJ)/fonts/helvR10.o
#	$(MW_DIR_OBJ)/fonts/symbol.o
#	$(MW_DIR_OBJ)/fonts/timBI18.o

ifeq ($(HAVE_BIG5_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/fonts/chinese/big5font.o
endif

ifeq ($(HAVE_GB2312_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/fonts/chinese/gb2312font.o
endif

ifeq ($(HAVE_JISX0213_SUPPORT), Y)
#OBJS += $(MW_DIR_OBJ)/fonts/japanese/jisx0213-16x16.o
OBJS += $(MW_DIR_OBJ)/fonts/japanese/jisx0213-12x12.o
endif

ifeq ($(HAVE_KSC5601_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/fonts/korean/jo16x16.o \
	$(MW_DIR_OBJ)/fonts/korean/ksjo_tbl.o
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

realclean: clean
	$(RM) \
		$(MW_DIR_OBJ)/fonts/helvB10.c \
		$(MW_DIR_OBJ)/fonts/helvB12.c \
		$(MW_DIR_OBJ)/fonts/helvR10.c \
		$(MW_DIR_OBJ)/fonts/timBI18.c \
		$(MW_DIR_OBJ)/fonts/X5x7.c \
		$(MW_DIR_OBJ)/fonts/X6x13.c

