#
# Copyright (C) Chris Johns (ccj@acm.org)
#
# Microwindows C++ Wrappers.
#

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := mwin/mwobjects/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

ifeq ($(OBJFRAMEWORK), Y)

LIBNAME = libmwobjects.a
#LIBNAMESO = libmwobjects.so

CXXOBJS := $(MW_DIR_OBJ)/mwin/mwobjects/mwobjects.o

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
