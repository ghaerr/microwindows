##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 Koninklijke Philips Electronics N.V.
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := engine/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwengine.a
#LIBNAMESO = libmwengine.so

ifeq ($(ZAURUSMOUSE), Y)
CFLAGS += -DFLIP_MOUSE_IN_PORTRAIT_MODE=0
endif

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/engine/devopen.o \
	$(MW_DIR_OBJ)/engine/devdraw.o \
	$(MW_DIR_OBJ)/engine/devfont.o \
	$(MW_DIR_OBJ)/engine/devmouse.o \
	$(MW_DIR_OBJ)/engine/devkbd.o \
	$(MW_DIR_OBJ)/engine/devclip.o \
	$(MW_DIR_OBJ)/engine/devrgn.o \
	$(MW_DIR_OBJ)/engine/devpal1.o \
	$(MW_DIR_OBJ)/engine/devpal2.o \
	$(MW_DIR_OBJ)/engine/devimage.o \
	$(MW_DIR_OBJ)/engine/devlist.o \
	$(MW_DIR_OBJ)/engine/selfont.o \
	$(MW_DIR_OBJ)/engine/error.o \
	$(MW_DIR_OBJ)/engine/devrgn2.o \
	$(MW_DIR_OBJ)/engine/devarc.o \
	$(MW_DIR_OBJ)/engine/devpoly.o \
	$(MW_DIR_OBJ)/engine/devstipple.o \
	$(MW_DIR_OBJ)/engine/font_dbcs.o

ifeq ($(HAVE_FREETYPE_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/engine/font_freetype.o
endif

ifeq ($(HAVE_FREETYPE_2_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/engine/font_freetype2.o
endif

ifeq ($(HAVE_FNT_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/engine/font_fnt.o
endif

ifeq ($(HAVE_PCF_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/engine/font_pcf.o
endif

ifeq ($(HAVE_T1LIB_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/engine/font_t1lib.o
endif

ifeq ($(HAVE_HZK_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/engine/font_hzk.o
endif

ifeq ($(HAVE_EUCJP_SUPPORT), Y)
OBJS += $(MW_DIR_OBJ)/engine/font_eucjp.o
endif

#ifeq ($(UNIX), 1)
OBJS += $(MW_DIR_OBJ)/engine/devtimer.o
#endif

ifneq ($(ARCH), ELKS)
OBJS += $(MW_DIR_OBJ)/engine/devpal8.o
endif

ifeq ($(GRAYPALETTE), Y)
OBJS += $(MW_DIR_OBJ)/engine/devpalgray4.o
else
OBJS += $(MW_DIR_OBJ)/engine/devpal4.o
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
