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

OBJS := $(MW_DIR_OBJ)/fonts/obsolete.o

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

.PHONY: realclean

realclean: clean
ifeq ($(MW_DIR_OBJ),$(MW_DIR_SRC))
	$(RM) \
		$(MW_DIR_OBJ)/fonts/helvB10.c \
		$(MW_DIR_OBJ)/fonts/helvB12.c \
		$(MW_DIR_OBJ)/fonts/helvR10.c \
		$(MW_DIR_OBJ)/fonts/timBI18.c \
		$(MW_DIR_OBJ)/fonts/X5x7.c \
		$(MW_DIR_OBJ)/fonts/X6x13.c
endif


