##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/nuklear/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS +=
# Defines for preprocessor
DEFINES +=

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Linking flags
LDFLAGS +=

############################# targets section ################################

ifeq ($(NANOX), Y)
ifeq ($(NANOXDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of binaries to compile

# demos
TARGETS += \
	$(MW_DIR_BIN)/demo-nuklear-calculator \
	$(MW_DIR_BIN)/demo-nuklear-overview \
	$(MW_DIR_BIN)/demo-nuklear-node_editor \
	$(MW_DIR_BIN)/demo-nuklear-demo

all: default $(TARGETS)

endif # NANOXDEMO
endif # NANOX

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SCREEN), ALLEGRO5)
# don't link client apps with -lallegro
LDFLAGS := $(filter-out $(EXTENGINELIBS),$(LDFLAGS))
endif

$(TARGETS): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/nuklear/%.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(NANOXCLIENTLIBS) $(LDFLAGS) $(LDLIBS)
ifeq ($(ARCH), COSMO)
	$(OBJCOPY) -S -O binary $@ $@.com
endif

DEMO_NUKLEAR=$(MW_DIR_SRC)/demos/nuklear/demo-nuklear.c $(MW_DIR_SRC)/demos/nuklear/nuklear_nxlib.h nuklear.h

$(MW_DIR_OBJ)/demos/nuklear/demo-nuklear-calculator.o: $(MW_DIR_SRC)/demos/nuklear/calculator.c $(DEMO_NUKLEAR)
	[ -d $(MW_DIR_OBJ)/demos/nuklear ] || mkdir -p $(MW_DIR_OBJ)/demos/nuklear
	@echo "Compiling $(patsubst $(MW_DIR_SRC)/%,%,$<) ..."
	$(CC) $(COMPILEARGS) -c $(MW_DIR_SRC)/demos/nuklear/demo-nuklear.c -o $@ -DINCLUDE_CALCULATOR

$(MW_DIR_OBJ)/demos/nuklear/demo-nuklear-overview.o: $(MW_DIR_SRC)/demos/nuklear/overview.c $(DEMO_NUKLEAR)
	@echo "Compiling $(patsubst $(MW_DIR_SRC)/%,%,$<) ..."
	$(CC) $(COMPILEARGS) -c $(MW_DIR_SRC)/demos/nuklear/demo-nuklear.c -o $@ -DINCLUDE_OVERVIEW

$(MW_DIR_OBJ)/demos/nuklear/demo-nuklear-node_editor.o: $(MW_DIR_SRC)/demos/nuklear/node_editor.c $(DEMO_NUKLEAR)
	@echo "Compiling $(patsubst $(MW_DIR_SRC)/%,%,$<) ..."
	$(CC) $(COMPILEARGS) -c $(MW_DIR_SRC)/demos/nuklear/demo-nuklear.c -o $@ -DINCLUDE_NODE_EDITOR

$(MW_DIR_OBJ)/demos/nuklear/demo-nuklear-demo.o: $(MW_DIR_SRC)/demos/nuklear/demo.c $(DEMO_NUKLEAR)
	@echo "Compiling $(patsubst $(MW_DIR_SRC)/%,%,$<) ..."
	$(CC) $(COMPILEARGS) -c $(MW_DIR_SRC)/demos/nuklear/demo-nuklear.c -o $@ -DINCLUDE_DEMO
