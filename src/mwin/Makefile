##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := mwin/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

ifeq ($(MICROWIN), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwin.a
LIBNAMESO = libmwin.so

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/mwin/winmain.o \
	$(MW_DIR_OBJ)/mwin/winuser.o \
	$(MW_DIR_OBJ)/mwin/wingdi.o \
	$(MW_DIR_OBJ)/mwin/winexpos.o \
	$(MW_DIR_OBJ)/mwin/winclip.o \
	$(MW_DIR_OBJ)/mwin/winevent.o \
	$(MW_DIR_OBJ)/mwin/windefw.o \
	$(MW_DIR_OBJ)/mwin/winrgn.o \
	$(MW_DIR_OBJ)/mwin/winfont.o \
	$(MW_DIR_OBJ)/mwin/winsbar.o

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

