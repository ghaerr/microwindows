##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := mwin/winlib/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

ifeq ($(MICROWIN), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwinlib.a
LIBNAMESO = libmwinlib.so

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/mwin/winlib/draw3d.o \
	$(MW_DIR_OBJ)/mwin/winlib/ptinsid.o \
	$(MW_DIR_OBJ)/mwin/winlib/insetr.o \
	$(MW_DIR_OBJ)/mwin/winlib/fastfill.o \
	$(MW_DIR_OBJ)/mwin/winlib/graph3d.o \
	$(MW_DIR_OBJ)/mwin/winlib/mwuser.o

ifneq ($(ARCH), ELKS)
OBJS += \
	$(MW_DIR_OBJ)/mwin/winlib/button.o \
	$(MW_DIR_OBJ)/mwin/winlib/edit.o \
	$(MW_DIR_OBJ)/mwin/winlib/listbox.o \
	$(MW_DIR_OBJ)/mwin/winlib/progbar.o \
	$(MW_DIR_OBJ)/mwin/winlib/static.o \
	$(MW_DIR_OBJ)/mwin/winlib/combobox.o \
	$(MW_DIR_OBJ)/mwin/winlib/caret.o \
	$(MW_DIR_OBJ)/mwin/winlib/scrlbar.o \
	$(MW_DIR_OBJ)/mwin/winlib/medit.o
endif

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

