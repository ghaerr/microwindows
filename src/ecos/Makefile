##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := ecos/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS +=
# Defines for preprocessor
DEFINES +=

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Preprocessor flags OTHER than defines
CPPFLAGS +=
# Linking flags
LDFLAGS += -Wl,--gc-sections -Wl,-static -Wl,-N -g -O2 -nostdlib -L$(ECOS_PREFIX)/lib -Ttarget.ld

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME =
LIBNAMESO =

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/ecos/ecos_app.o \
	$(MW_DIR_OBJ)/ecos/ecos_init.o
NANO_OBJS := 
DEFINES += -DUSE_ROMDISK
OBJS += $(MW_DIR_OBJ)/ecos/nanox_thread.o
NANO_OBJS += $(MW_DIR_BIN)/nano-X.o $(MW_DIR_BIN)/nwidgets.o
OBJS += $(MW_DIR_OBJ)/ecos/nanowm_thread.o
NANO_OBJS += $(MW_DIR_BIN)/nanowm.o
OBJS += $(MW_DIR_OBJ)/ecos/nxkbd_thread.o
NANO_OBJS += $(MW_DIR_BIN)/nxkbd.o
DEFINES += -DUSE_NXSCRIBBLE
OBJS += $(MW_DIR_OBJ)/ecos/nxscribble_thread.o
NANO_OBJS += $(MW_DIR_BIN)/nxscribble.o 
DEFINES += -DUSE_LANDMINE
OBJS += $(MW_DIR_OBJ)/ecos/landmine_thread.o
NANO_OBJS += $(MW_DIR_BIN)/landmine.o 
DEFINES += -DUSE_NTETRIS
OBJS += $(MW_DIR_OBJ)/ecos/ntetris_thread.o
NANO_OBJS += $(MW_DIR_BIN)/ntetris.o 
DEFINES += -DUSE_WORLD
OBJS += $(MW_DIR_OBJ)/ecos/world_thread.o
NANO_OBJS += $(MW_DIR_BIN)/world.o 


# demos should be built after the libs !
all: $(MW_DIR_BIN)/ecos_app

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

# FIXME: Put a link rule for this binary.
$(MW_DIR_BIN)/ecos_app: $(OBJS) $(NANO_OBJS)

$(MW_DIR_OBJ)/ecos/ecos_app.o: $(MW_DIR_SRC)/ecos/Makefile


