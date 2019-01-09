##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := images/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

ifeq ($(MICROWIN), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwimages.a
LIBNAMESO =

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/images/demos/mwin/car8.o \
	$(MW_DIR_OBJ)/images/demos/mwin/min4.o \
	$(MW_DIR_OBJ)/images/demos/mwin/max4.o \
	$(MW_DIR_OBJ)/images/demos/mwin/close4.o \
	$(MW_DIR_OBJ)/images/demos/mwin/under4.o \
	$(MW_DIR_OBJ)/images/demos/mwin/zion208.o \
	$(MW_DIR_OBJ)/images/demos/mwin/cs1.o \
	$(MW_DIR_OBJ)/images/demos/mwin/rle8.o \
	$(MW_DIR_OBJ)/images/demos/mwin/microwin.o \
	$(MW_DIR_OBJ)/images/demos/mwin/penguin.o \
	$(MW_DIR_OBJ)/images/demos/mwin/dragon.o

TOOLS = \
	$(MW_DIR_BIN)/convbmp \
	$(MW_DIR_BIN)/makebmp \
	$(MW_DIR_BIN)/convpbm

all: $(TOOLS) default

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

.PHONY: realclean

realclean: clean
ifeq ($(MW_DIR_OBJ),$(MW_DIR_SRC))
	$(RM) \
		$(MW_DIR_OBJ)/images/demos/mwin/car8.c \
		$(MW_DIR_OBJ)/images/demos/mwin/min4.c \
		$(MW_DIR_OBJ)/images/demos/mwin/max4.c \
		$(MW_DIR_OBJ)/images/demos/mwin/close4.c \
		$(MW_DIR_OBJ)/images/demos/mwin/under4.c \
		$(MW_DIR_OBJ)/images/demos/mwin/zion208.c \
		$(MW_DIR_OBJ)/images/demos/mwin/cs1.c \
		$(MW_DIR_OBJ)/images/demos/mwin/rle8.c \
		$(MW_DIR_OBJ)/images/demos/mwin/microwin.c \
		$(MW_DIR_OBJ)/images/demos/mwin/penguin.c \
		$(MW_DIR_OBJ)/images/demos/mwin/dragon.c
endif

$(MW_DIR_BIN)/makebmp: $(MW_DIR_SRC)/images/tools/makebmp.c
	echo "Building $(patsubst $(MW_DIR_BIN)/%,%,$@) tool ..."
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

$(MW_DIR_BIN)/convpbm: $(MW_DIR_SRC)/images/tools/convpbm.c
	echo "Building $(patsubst $(MW_DIR_BIN)/%,%,$@) tool ..."
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

# Sigh.  Old versions of make are completely broken and need this.
# They should be able to pick it up from the pattern rule in
# Makefile.rules.
#$(OBJS): $(MW_DIR_OBJ)/%.o: $(MW_DIR_OBJ)/%.c $(CONFIG)
#	@echo "Compiling $(patsubst $(MW_DIR_OBJ)/%,%,$<) ..."
#	@$(MW_RULE_MAKE_TARGET_DIR)
#	$(CC) -c $(COMPILEARGS) -o $@ $<
#	$(MW_RULE_DEPEND_C)
#
#$(OBJS:.o=.c):

