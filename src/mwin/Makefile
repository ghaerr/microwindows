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

ifeq ($(MICROWIN), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwin.a
LIBNAMESO = libmwin.so

# List of objects to compile
OBJS = winmain.o winuser.o wingdi.o winexpos.o winclip.o winevent.o\
	windefw.o winrgn.o winfont.o winsbar.o

endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

