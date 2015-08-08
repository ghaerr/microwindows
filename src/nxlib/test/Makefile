##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := nxlib/test/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)
##############################################################################

INCLUDE=-I..
DOSEXT :=

ifeq ($(SHAREDLIBS), Y)
LIBS =$(LDFLAGS) $(CCNANOXCLIENTLIBS) ../../lib/libNX11.so 
else
LIBS =$(LDFLAGS) $(NANOXCLIENTLIBS) ../../lib/libNX11.a 
endif

ifeq ($(DOS_DJGPP), Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/libNX11.a ../../lib/libnano-X.a
endif

TARGETS = queryfont loadfont xhello xmouse xgreen
ifeq ($(DOS_DJGPP), N)
TARGETS += williams
endif
SOURCES = $(TARGETS:=.c)
TARGETSX = $(addsuffix $(DOSEXT),$(TARGETS))

ifeq ($(DOS_DJGPP), Y)
DOSEXT :=.exe
endif

all: $(TARGETSX)

$(TARGETSX): $(SOURCES)
	@echo "Compiling nxlib/test/$@ ..."
	$(CC) $(CFLAGS) -o $@ $(INCLUDE) $(basename $@).c $(LIBS)
	
clean:
	-rm $(TARGETSX)
