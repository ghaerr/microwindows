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

# Nano-X client files
OBJS := \
	$(MW_DIR_OBJ)/nanox/nxdraw.o \
	$(MW_DIR_OBJ)/nanox/nxutil.o \
	$(MW_DIR_OBJ)/nanox/nxtransform.o

ifeq ($(LINK_APP_INTO_SERVER), Y)
OBJS += $(NANOXFILES)
else

# Nano-X server files (network only)
NANOXFILES += $(MW_DIR_OBJ)/nanox/srvnet.o

# Nano-X client files (network only)
OBJS += \
	$(MW_DIR_OBJ)/nanox/client.o \
	$(MW_DIR_OBJ)/nanox/nxproto.o \
	$(MW_DIR_OBJ)/nanox/error.o
ifeq ($(FRAMEBUFFER), Y)
OBJS += $(MW_DIR_OBJ)/nanox/clientfb.o
else
ifeq ($(X11), Y)
OBJS += $(MW_DIR_OBJ)/nanox/clientfb.o
endif
endif

ifeq ($(ARCH), ECOS) 
TARGET = $(MW_DIR_BIN)/nano-X.o
else
TARGET = $(MW_DIR_BIN)/nano-X
endif
all: default $(TARGET)
endif

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

# Server binary ...
ifeq ($(SHAREDLIBS), Y)
$(MW_DIR_BIN)/nano-X: $(NANOXFILES) $(NANOXSERVERLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NANOXFILES) $(CCNANOXSERVERLIBS)
else
$(MW_DIR_BIN)/nano-X.o: $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
#	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) -Wl,-r -Wl,--retain-symbols-file -Wl,nanox.syms -Wl,--defsym -Wl,nanox_main=main -o XX.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) -Wl,-r -Wl,--defsym -Wl,nanox_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep -v Gr | grep -v nx | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
#	rm -f XX.o

$(MW_DIR_BIN)/nano-X: $(NANOXFILES) $(NANOXSERVERLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NANOXFILES) $(NANOXSERVERLIBS)
endif
