##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC = ..
endif
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

ifeq ($(NANOX), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libnano-X.a
LIBNAMESO = libnano-X.so

# Nano-X server files
NANOXFILES = srvmain.o srvfunc.o srvutil.o srvevent.o srvclip.o

# Nano-X client files
OBJS = nxdraw.o nxutil.o nxtransform.o

ifeq ($(LINK_APP_INTO_SERVER), Y)
OBJS += $(NANOXFILES)
else

# Nano-X server files (network only)
NANOXFILES += srvnet.o

# Nano-X client files (network only)
OBJS += client.o nxproto.o error.o
ifeq ($(FRAMEBUFFER), Y)
OBJS += clientfb.o
else
ifeq ($(X11), Y)
OBJS += clientfb.o
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
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NANOXFILES) $(CCNANOXSERVERLIBS)
else
$(MW_DIR_BIN)/nano-X.o: $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) $(CONFIG)
	@echo "Linking $@ ..."
#	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) -Wl,-r -Wl,--retain-symbols-file -Wl,nanox.syms -Wl,--defsym -Wl,nanox_main=main -o XX.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) -Wl,-r -Wl,--defsym -Wl,nanox_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep -v Gr | grep -v nx | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
#	rm -f XX.o

$(MW_DIR_BIN)/nano-X: $(NANOXFILES) $(NANOXSERVERLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NANOXFILES) $(NANOXSERVERLIBS)
endif
