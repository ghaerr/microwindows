#
# Copyright (C) Chris Johns (ccj@acm.org)
#
# Microwindows C++ Wrappers.
#

ifndef MW_DIR_SRC
MW_DIR_SRC = ../..
endif
include $(MW_DIR_SRC)/Path.rules

include $(CONFIG)

LIBNAME = libmwobjects.a
#LIBNAMESO = libmwobjects.so

ifeq ($(OBJFRAMEWORK), Y)
CXXOBJS += mwobjects.o
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
