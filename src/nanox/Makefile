##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
##############################################################################

ifndef TOP
TOP = ..
CONFIG = $(TOP)/config
endif

include $(CONFIG)

############################# targets section ################################

ifeq ($(NANOX), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME = libnano-X.a
LIBNAMESO = libnano-X.so

# Nano-X server files
NANOXFILES = srvmain.o srvfunc.o srvutil.o srvevent.o srvclip.o
NETFILES = srvnet.o
UTILFILES = nxdraw.o nxutil.o

OBJS =
ifeq ($(LINK_APP_INTO_SERVER), Y)
NANOXFILES += $(UTILFILES)
OBJS += $(NANOXFILES)
else
NANOXFILES += $(NETFILES)
ifeq ($(FRAMEBUFFER), Y)
OBJS += clientfb.o
else
ifeq ($(X11), Y)
OBJS += clientfb.o
endif
endif

OBJS += client.o nxproto.o nxtransform.o error.o $(UTILFILES)
ifeq ($(ARCH), ECOS) 
TARGET = $(TOP)/bin/nano-X.o
else
TARGET = $(TOP)/bin/nano-X
endif
all: default $(TARGET)
endif

endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################

# Server binary ...
ifeq ($(SHAREDLIBS), Y)
$(TOP)/bin/nano-X: $(NANOXFILES) $(NANOXSERVERLIBS) $(TOP)/config
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NANOXFILES) $(CCNANOXSERVERLIBS)
else
$(TOP)/bin/nano-X.o: $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) $(TOP)/config
	@echo "Linking $@ ..."
#	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) -Wl,-r -Wl,--retain-symbols-file -Wl,nanox.syms -Wl,--defsym -Wl,nanox_main=main -o XX.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(NANOXFILES) $(NANOXSERVERLIBS) -Wl,-r -Wl,--defsym -Wl,nanox_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep -v Gr | grep -v nx | grep ' T' | awk -f $(TOP)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
#	rm -f XX.o

$(TOP)/bin/nano-X: $(NANOXFILES) $(NANOXSERVERLIBS) $(TOP)/config
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NANOXFILES) $(NANOXSERVERLIBS)
endif
