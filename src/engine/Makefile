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

OBJS := $(MW_DIR_OBJ)/engine/obsolete.o


######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
