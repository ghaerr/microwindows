##############################################################################
# Microwindows template Makefile
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := contrib/nx11-test/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

include $(MW_DIR_SRC)/Arch.rules

############################# targets section ################################

# tools and file extensions
CC=$(TOOLSPREFIX)$(COMPILER)
ifeq ($(DOS_DJGPP), Y)
DOSEXT :=.exe
endif
ifeq ($(MINGW), Y)
DOSEXT :=.exe
endif

# X11 headers
ifneq ($(X11HDRLOCATION),)
INCLUDEDIRS += -I$(X11HDRLOCATION)
endif

NX11=NX11
#NX11=PX11

# NX11 and nano-X libs
LIBS = $(LDFLAGS) -L$(MW_DIR_LIB) -l$(NX11) -lnano-X -lm

ifeq ($(DOS_DJGPP), Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/lib$(NX11).a ../../lib/libnano-X.a $(CLIBS) -lm
endif

ifeq ($(MINGW), Y)
LIBS =$(LDFLAGS) -s -O2 ../../lib/lib$(NX11).a ../../lib/libnano-X.a $(CLIBS) -lm
endif

# targets
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

all: $(TARGETSX)

$(TARGETSX): $(SOURCES)
	@echo "Compiling $@.c ..."
	$(CC) $(CFLAGS) $(INCLUDEDIRS) $(DEFINES) -o $@ $(basename $@).c $(LIBS)
	
clean:
ifeq ($(DOS_DJGPP), Y)
	$(RM) $(TARGETSX)
else    
	-rm -f $(TARGETSX)
endif
