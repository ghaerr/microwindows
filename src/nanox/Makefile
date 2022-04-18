##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := nanox/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

ifeq ($(NANOX), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libnano-X.a
LIBNAMESO = libnano-X.so

# Get list of core files (engine, fonts and drivers).
MW_CORE_OBJS :=
include $(MW_DIR_SRC)/engine/Objects.rules
include $(MW_DIR_SRC)/fonts/Objects.rules
include $(MW_DIR_SRC)/drivers/Objects.rules

# Nano-X server files
NANOXFILES := \
	$(MW_CORE_OBJS) \
	$(MW_DIR_OBJ)/nanox/srvmain.o \
	$(MW_DIR_OBJ)/nanox/srvfunc.o \
	$(MW_DIR_OBJ)/nanox/srvutil.o \
	$(MW_DIR_OBJ)/nanox/srvevent.o \
	$(MW_DIR_OBJ)/nanox/srvclip.o

NANOWMOBJS := \
	$(MW_DIR_OBJ)/nanox/wmaction.o \
	$(MW_DIR_OBJ)/nanox/wmclients.o \
	$(MW_DIR_OBJ)/nanox/wmevents.o \
	$(MW_DIR_OBJ)/nanox/wmutil.o

# Nano-X client files
OBJS := \
	$(MW_DIR_OBJ)/nanox/nxdraw.o \
	$(MW_DIR_OBJ)/nanox/nxutil.o \
	$(MW_DIR_OBJ)/nanox/nxtransform.o
OBJS += $(NANOWMOBJS)

ifeq ($(LINK_APP_INTO_SERVER), Y)
#
# LINK_APP_INTO_SERVER doesn't build bin/nano-X because the files are compiled with -DNONETWORK=1
# Instead, only the nano-X.a/.so is built, which links the Gr routines directly into the library
#
OBJS += $(NANOXFILES)
OBJS += $(MW_DIR_OBJ)/nanox/srvnonet.o
else

ifeq ($(NANOWM), Y)
NANOXFILES += $(OBJS)
endif

# Nano-X server files (network only)
NANOXFILES += $(MW_DIR_OBJ)/nanox/srvnet.o

# Nano-X client files (network only)
OBJS += \
	$(MW_DIR_OBJ)/nanox/client.o \
	$(MW_DIR_OBJ)/nanox/clientfb.o \
	$(MW_DIR_OBJ)/nanox/nxproto.o \
	$(MW_DIR_OBJ)/drivers/osdep.o

NANOXSERVERLIBS = $(EXTENGINELIBS)

ifeq ($(HAVE_VNCSERVER_SUPPORT), Y)
NANOXSERVERLIBS += $(LIBVNC) $(LIBJPEG) $(LIBZ)
endif 

ifeq ($(ARCH), ECOS) 
TARGET = $(MW_DIR_BIN)/nano-X.o
else
TARGET = $(MW_DIR_BIN)/nano-X
endif

all: default $(TARGET)

endif # !LINK_APP_INTO_SERVER
endif # NANOX

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

#$(info NANOXSERVERLIBS $(NANOXSERVERLIBS))

# Server binary ...
$(MW_DIR_BIN)/nano-X: $(NANOXFILES) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NANOXFILES) $(NANOXSERVERLIBS) $(LDFLAGS) $(LDLIBS)
ifeq ($(ARCH), COSMO)
	$(OBJCOPY) -S -O binary $@ $@.com
endif

ifeq ($(ARCH), ECOS) 
$(MW_DIR_BIN)/nano-X.o: $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
#	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) -Wl,-r -Wl,--retain-symbols-file -Wl,nanox.syms -Wl,--defsym -Wl,nanox_main=main -o XX.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) -Wl,-r -Wl,--defsym -Wl,nanox_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep -v Gr | grep -v nx | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
#	rm -f XX.o
endif
