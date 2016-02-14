##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := nxlib/test/
#include $(MW_DIR_SRC)/Path.rules
#include $(MW_DIR_SRC)/Makefile.rules
#include $(MW_DIR_SRC)/Arch.rules
include $(CONFIG)
##############################################################################

INCLUDE=-I..
DOSEXT :=
#avoid to use default CC which is cc
CC=$(TOOLSPREFIX)gcc 

ifeq ($(SHAREDLIBS), Y)
LIBS =$(LDFLAGS) $(CCNANOXCLIENTLIBS) ../../lib/libNX11.so  ../../lib/libnano-X.so
else
LIBS =$(LDFLAGS) $(NANOXCLIENTLIBS) ../../lib/libNX11.a ../../lib/libnano-X.a
endif

ifeq ($(DOS_DJGPP), Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/libNX11.a ../../lib/libnano-X.a $(CLIBS)
endif

ifeq ($(MINGW), Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/libNX11.a ../../lib/libnano-X.a $(CLIBS)
endif

ifeq ($(LINUX_LINARO), Y)
TARGETS = xhello xmouse xgreen
else
TARGETS = queryfont loadfont xhello xmouse xgreen
endif

ifeq ($(DOS_DJGPP), N)
TARGETS += williams
endif
SOURCES = $(TARGETS:=.c)
TARGETSX = $(addsuffix $(DOSEXT),$(TARGETS))

ifeq ($(DOS_DJGPP), Y)
DOSEXT :=.exe
endif

ifeq ($(MINGW), Y)
DOSEXT :=.exe
endif

all: $(TARGETSX)

$(TARGETSX): $(SOURCES)
#	@echo "Compiling nxlib/test/$@ ..."
#	echo "$(CC) $(CFLAGS) -o $@ $(INCLUDE) $(basename $@).c $(LIBS)"
	$(CC) $(CFLAGS) -o $@ $(INCLUDE) $(basename $@).c $(LIBS)
	
clean:
ifeq ($(DOS_DJGPP), Y)
	$(RM) $(TARGETSX)
else    
	-rm $(TARGETSX)
endif
