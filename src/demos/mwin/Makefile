##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/mwin/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS +=
# Defines for preprocessor
DEFINES +=

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Linking flags
LDFLAGS +=

############################# targets section ################################

ifeq ($(MICROWIN), Y)
ifeq ($(MICROWINDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of images for the mwmine demo.
MINEIMAGES := \
	$(MW_DIR_OBJ)/images/demos/mwin/mwmine/mineflag.o \
	$(MW_DIR_OBJ)/images/demos/mwin/mwmine/mineface.o \
	$(MW_DIR_OBJ)/images/demos/mwin/mwmine/minefacelost.o \
	$(MW_DIR_OBJ)/images/demos/mwin/mwmine/minebomb.o \
	$(MW_DIR_OBJ)/images/demos/mwin/mwmine/minedone.o \
	$(MW_DIR_OBJ)/images/demos/mwin/mwmine/minehitfalse.o

ifeq ($(ARCH), PSP)
DEMOS = $(MW_DIR_BIN)/mwdemo\
	$(MW_DIR_BIN)/mwdemo2\
	$(MW_DIR_BIN)/mwdemoalpha\
	$(MW_DIR_BIN)/mwmine\
	$(MW_DIR_BIN)/mwtest

else
DEMOS = \
	$(MW_DIR_BIN)/mwdemo\
	$(MW_DIR_BIN)/mwdemo2\
	$(MW_DIR_BIN)/mwmine\
	$(MW_DIR_BIN)/mwtest\
	$(MW_DIR_BIN)/mwsimple\
	$(MW_DIR_BIN)/mwhello\
	$(MW_DIR_BIN)/mwcontrols\
	$(MW_DIR_BIN)/mwstatictext\
	$(MW_DIR_BIN)/mwdraw\
	$(MW_DIR_BIN)/mwprogbar\
	$(MW_DIR_BIN)/mwbrush\
	$(MW_DIR_BIN)/mwlistcombo\
	$(MW_DIR_BIN)/mwmenu\
	$(MW_DIR_BIN)/mwpenstyles


ifeq ($(MICROWINMULTIAPP), Y)
MWAPP = $(MW_DIR_OBJ)/demos/mwin/mwapp.o
DEMOS += $(MW_DIR_BIN)/mwapp
else
DEMOS += \
	$(MW_DIR_BIN)/mwstretch\
	$(MW_DIR_BIN)/mwdemoalpha\
	$(MW_DIR_BIN)/mwmsgbox\
	$(MW_DIR_BIN)/mwopenfile

DEMOS += $(MW_DIR_BIN)/mwdialog
RES   += $(MW_DIR_BIN)/mwdialog.res

DEMOS += $(MW_DIR_BIN)/mwdvetest
RES   += $(MW_DIR_BIN)/mwdvetest.res
endif # !MICROWINMULTIAPP

ifneq ($(ARCH), RTEMS)
ifneq ($(ARCH), ANDROID)
#DEMOS += $(MW_DIR_BIN)/mwterm
endif
endif

endif # !PSP

all: default $(DEMOS) $(RES)

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

# These demos use a hardcoded link line.
MWIN_DEMOS_WITH_NONSTANDARD_LINK := \
	$(MW_DIR_BIN)/mwmine $(MW_DIR_BIN)/mwapp

# All other demos use a standard link line, that just
# links against the Microwindows libraries.
MWIN_DEMOS_WITH_STANDARD_LINK := \
	$(filter-out $(MWIN_DEMOS_WITH_NONSTANDARD_LINK),$(DEMOS))

$(MWIN_DEMOS_WITH_STANDARD_LINK): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/mwin/%.o $(MWINLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
ifeq ($(MICROWINMULTIAPP), Y)
  ifeq ($(ARCH), MACOSX)
	$(CC) -shared -fpic -undefined dynamic_lookup $< -o $@.so
  else
	$(CC) -shared -o $@.so -Wl,--whole-archive $< -Wl,--no-whole-archive
  endif
else # !MICROWINMULTIAPP
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(MWINLIBS) $(LDFLAGS)
endif

$(MW_DIR_BIN)/mwmine: $(MW_DIR_OBJ)/demos/mwin/mwmine.o $(MINEIMAGES) $(MWINLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
ifeq ($(MICROWINMULTIAPP), Y)
  ifeq ($(ARCH), MACOSX)
	$(CC) -shared -fpic -undefined dynamic_lookup $< $(MINEIMAGES) -o $@.so
  else
	$(CC) -shared -o $@.so -Wl,--whole-archive $< $(MINEIMAGES) -Wl,--no-whole-archive
  endif
else
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(MINEIMAGES) -o $@ $(MWINLIBS) $(LDFLAGS)
endif

$(MW_DIR_BIN)/mwapp: $(MWAPP) $(MWINLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(MWAPP) -o $@ $(MWINLIBS) $(LDFLAGS) -ldl

$(MW_DIR_BIN)/mwdialog.res:
	ln -s ../images/demos/mwin/mwdialog/mwdialog.res $@

$(MW_DIR_BIN)/mwdvetest.res:
	ln -s ../images/demos/mwin/mwdvetest/dveres.res $@
