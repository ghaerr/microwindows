##############################################################################
# Makefile to compile NX11 demos
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

# use local X11 headers to compile with no X11 or SDL
X11HDRLOCATION=$(MW_DIR_SRC)/nx11/X11-local
ifneq ($(X11HDRLOCATION),)
INCLUDEDIRS += -I$(X11HDRLOCATION)
endif

NX11=NX11
#NX11=PX11

# NX11 and nano-X libs
LIBS = $(LDFLAGS) -Wl,-rpath=$(MW_DIR_LIB) -L$(MW_DIR_LIB) -l$(NX11) -lnano-X -lm

ifeq ($(ARCH), MACOSX)
# OSX can use shared libs and just work without -rpath
LIBS = $(LDFLAGS) -L$(MW_DIR_LIB) -lNX11 -lnano-X -lm
endif

ifeq ($(DOS_DJGPP), Y)
LIBS =$(LDFLAGS) -s -O2 $(MW_DIR_LIB)/lib$(NX11).a $(MW_DIR_LIB)/libnano-X.a $(CLIBS) -lm
endif

ifeq ($(MINGW), Y)
LIBS =$(LDFLAGS) -s -O2 $(MW_DIR_LIB)/lib$(NX11).a $(MW_DIR_LIB)/libnano-X.a $(CLIBS) -lm
endif

# targets
ifeq ($(LINUX_LINARO), Y)
TARGETS = xhello xmouse xgreen
else
TARGETS = queryfont loadfont listfonts xdraw xtextbox xhello xmouse xgreen xtestarc xservervendor
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
