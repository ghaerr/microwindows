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

ifeq ($(X11), Y)
#LDFLAGS contains -lX11 which we don't want here
LDFLAGS=-L$(MW_DIR_LIB) 
endif

# should be e.g. -L/home/georg/microwindows/src/lib
#$(info    LDFLAGS is $(LDFLAGS))
#should be -lnano-X
#$(info    CCNANOXCLIENTLIBS is $(CCNANOXCLIENTLIBS))

ifeq ($(DEBUG), Y)
GDBFLAG = -g
else
GDBFLAG = -s
endif

ifeq ($(SHAREDLIBS), Y)
LIBS =$(LDFLAGS) -lNX11 -lnano-X $(CCNANOXCLIENTLIBS)
else
LIBS =$(LDFLAGS) -lNX11 -lnano-X $(NANOXCLIENTLIBS)
endif

#$(info LDFLAGS $(LDFLAGS))
#$(info LIBS $(LIBS))

ifeq ($(DOS_DJGPP), Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/libNX11.a ../../lib/libnano-X.a $(CLIBS)
endif

ifeq ($(MINGW), Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/libNX11.a ../../lib/libnano-X.a $(CLIBS)
endif

#for draw
LIBS += -lm 

ifeq ($(LINUX_LINARO), Y)
TARGETS = xhello xmouse xgreen
else
TARGETS = queryfont loadfont listfonts draw textbox xhello xmouse xgreen testarc xservervendor
endif

ifneq ($(DOS_DJGPP), Y)
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
	@echo "Compiling nxlib/test/$@.c ..."
	#echo "$(CC) $(GDBFLAG) $(CFLAGS) -o $@ $(INCLUDE) $(basename $@).c $(LIBS)"
	$(CC) $(GDBFLAG) $(CFLAGS) -o $@ $(INCLUDE) $(basename $@).c $(LIBS)
	
clean:
ifeq ($(DOS_DJGPP), Y)
	$(RM) $(TARGETSX)
else    
	-rm $(TARGETSX)
endif
