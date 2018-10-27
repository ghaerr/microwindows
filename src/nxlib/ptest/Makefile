##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := nxlib/ptest/
include $(MW_DIR_SRC)/Path.rules
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

ifeq ($(DEBUG), Y)
GDBFLAG = -g
else
GDBFLAG = -s
endif

ifeq ($(SHAREDLIBS), Y)
LIBS =$(LDFLAGS) $(CCNANOXCLIENTLIBS) ../../lib/libPX11.so 
else
LIBS =$(LDFLAGS) $(NANOXCLIENTLIBS) ../../lib/libPX11.a 
endif

ifeq ($(DOS_DJGPP),Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/libPX11.a $(CLIBS)
endif

ifeq ($(MINGW), Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/libPX11.a $(CLIBS) -lz
endif

ifdef MINGW #workaround
LIBS =$(LDFLAGS) -s -O2 ../../lib/libPX11.a $(CLIBS) -lz
endif

#for draw
LIBS += -lm 

TARGETS = queryfont loadfont listfonts draw textbox xhello xmouse xgreen testarc xservervendor
ifneq ($(DOS_DJGPP), Y)
TARGETS += williams
endif
SOURCES = $(addprefix ../test/,$(TARGETS:=.c))
TARGETSX = $(addsuffix $(DOSEXT),$(TARGETS))

ifeq ($(DOS_DJGPP),Y)
DOSEXT :=.exe
endif

ifeq ($(MINGW), Y)
DOSEXT :=.exe
endif

ifdef MINGW #workaround
DOSEXT :=.exe
endif

all: $(TARGETSX) 

$(TARGETSX): $(SOURCES)
	@echo "Compiling nxlib/ptest/$@.c ..."
	#echo "$(CC) $(GDBFLAG) $(CFLAGS) -o $@ $(INCLUDE) ../test/$(basename $@).c $(LIBS)"
	$(CC) $(GDBFLAG) $(CFLAGS) -o $@ $(INCLUDE) ../test/$(basename $@).c $(LIBS)
	
clean:
ifeq ($(DOS_DJGPP), Y)
	$(RM) $(TARGETSX)
else    
	-rm $(TARGETSX)
endif
