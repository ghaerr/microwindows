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

# Get list of core files (engine, fonts and drivers).
MW_CORE_OBJS :=
include $(MW_DIR_SRC)/engine/Objects.rules
include $(MW_DIR_SRC)/fonts/Objects.rules
include $(MW_DIR_SRC)/drivers/Objects.rules

# List of objects to compile
OBJS := \
	$(MW_CORE_OBJS) \
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

OBJS += \
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

