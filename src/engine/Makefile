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

ifeq ($(ENGINE), Y)
# If you want to create a library with the objects files, define the name here
LIBNAME = libmwengine.a
LIBNAMESO =

# Get list of core files (engine, fonts and drivers).
MW_CORE_OBJS :=
include $(MW_DIR_SRC)/engine/Objects.rules
include $(MW_DIR_SRC)/drivers/Objects.rules

OBJS := $(MW_CORE_OBJS)
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
