##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := mwin/bmp/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

#all: $(MW_DIR_BIN)/convbmp
#HOSTCC=gcc
#HOSTCFLAGS=-I../../include

############################# targets section ################################

ifeq ($(MICROWIN), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwimages.a
LIBNAMESO = libmwimages.so

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/mwin/bmp/car8.o \
	$(MW_DIR_OBJ)/mwin/bmp/min4.o \
	$(MW_DIR_OBJ)/mwin/bmp/max4.o \
	$(MW_DIR_OBJ)/mwin/bmp/close4.o \
	$(MW_DIR_OBJ)/mwin/bmp/under4.o \
	$(MW_DIR_OBJ)/mwin/bmp/zion208.o \
	$(MW_DIR_OBJ)/mwin/bmp/cs1.o \
	$(MW_DIR_OBJ)/mwin/bmp/rle8.o \
	$(MW_DIR_OBJ)/mwin/bmp/microwin.o \
	$(MW_DIR_OBJ)/mwin/bmp/penguin.o

all: $(MW_DIR_BIN)/convbmp $(MW_DIR_BIN)/makebmp default

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

.PHONY: realclean

realclean: clean
ifeq ($(MW_DIR_OBJ),$(MW_DIR_SRC))
	$(RM) \
		$(MW_DIR_OBJ)/mwin/bmp/car8.c \
		$(MW_DIR_OBJ)/mwin/bmp/min4.c \
		$(MW_DIR_OBJ)/mwin/bmp/max4.c \
		$(MW_DIR_OBJ)/mwin/bmp/close4.c \
		$(MW_DIR_OBJ)/mwin/bmp/under4.c \
		$(MW_DIR_OBJ)/mwin/bmp/zion208.c \
		$(MW_DIR_OBJ)/mwin/bmp/cs1.c \
		$(MW_DIR_OBJ)/mwin/bmp/rle8.c \
		$(MW_DIR_OBJ)/mwin/bmp/microwin.c \
		$(MW_DIR_OBJ)/mwin/bmp/penguin.c
endif

$(MW_DIR_BIN)/makebmp: $(MW_DIR_SRC)/mwin/bmp/makebmp.c
	echo "Building $(patsubst $(MW_DIR_BIN)/%,%,$@) tool ..."
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

# Sigh.  Old versions of make are completely broken and need this.
# They should be able to pick it up from the pattern rule in
# Makefile.rules.
$(OBJS): $(MW_DIR_OBJ)/%.o: $(MW_DIR_OBJ)/%.c $(CONFIG)
	@echo "Compiling $(patsubst $(MW_DIR_OBJ)/%,%,$<) ..."
	@$(MW_RULE_MAKE_TARGET_DIR)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<
	$(MW_RULE_DEPEND_C)

$(OBJS:.o=.c):

