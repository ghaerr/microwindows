##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

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
OBJS = ecos_app.o ecos_init.o
NANO_OBJS = 
DEFINES += -DUSE_ROMDISK
OBJS += nanox_thread.o
NANO_OBJS += $(TOP)/bin/nano-X.o $(TOP)/bin/nwidgets.o
OBJS += nanowm_thread.o
NANO_OBJS += $(TOP)/bin/nanowm.o
OBJS += nxkbd_thread.o
NANO_OBJS += $(TOP)/bin/nxkbd.o
DEFINES += -DUSE_NXSCRIBBLE
OBJS += nxscribble_thread.o
NANO_OBJS += $(TOP)/bin/nxscribble.o 
DEFINES += -DUSE_LANDMINE
OBJS += landmine_thread.o
NANO_OBJS += $(TOP)/bin/landmine.o 
DEFINES += -DUSE_NTETRIS
OBJS += ntetris_thread.o
NANO_OBJS += $(TOP)/bin/ntetris.o 
DEFINES += -DUSE_WORLD
OBJS += world_thread.o
NANO_OBJS += $(TOP)/bin/world.o 


# demos should be built after the libs !
all: ecos_app

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

ecos_app: $(OBJS) $(NANO_OBJS)

ecos_app.o: Makefile


