##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 Koninklijke Philips Electronics N.V.
##############################################################################

include $(CONFIG)

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwengine.a
#LIBNAMESO = libmwengine.so

ifeq ($(ZAURUSMOUSE), Y)
CFLAGS += -DFLIP_MOUSE_IN_PORTRAIT_MODE=0
endif

# List of objects to compile
OBJS =	devopen.o devdraw.o devfont.o devmouse.o devkbd.o devclip.o devrgn.o \
	devpal1.o devpal2.o devimage.o devlist.o selfont.o error.o \
	devrgn2.o devarc.o devpoly.o devstipple.o font_dbcs.o

ifeq ($(HAVE_FREETYPE_SUPPORT), Y)
OBJS += font_freetype.o
endif

ifeq ($(HAVE_FREETYPE_2_SUPPORT), Y)
OBJS += font_freetype2.o
endif

ifeq ($(HAVE_FNT_SUPPORT), Y)
OBJS += font_fnt.o
endif

ifeq ($(HAVE_PCF_SUPPORT), Y)
OBJS += font_pcf.o
endif

ifeq ($(HAVE_T1LIB_SUPPORT), Y)
OBJS += font_t1lib.o
endif

ifeq ($(HAVE_HZK_SUPPORT), Y)
OBJS += font_hzk.o
endif

ifeq ($(HAVE_EUCJP_SUPPORT), Y)
OBJS += font_eucjp.o
endif

#ifeq ($(UNIX), 1)
OBJS += devtimer.o
#endif

ifneq ($(ARCH), ELKS)
OBJS += devpal8.o
endif

ifeq ($(GRAYPALETTE), Y)
OBJS += devpalgray4.o
else
OBJS += devpal4.o
endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################
