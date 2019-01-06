##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/nanox/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS +=
# Defines for preprocessor
DEFINES +=

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Preprocessor flags OTHER than defines
CPPFLAGS +=
# Linking flags
LDFLAGS +=

############################# targets section ################################

ifeq ($(NANOX), Y)

ifeq ($(LINK_APP_INTO_SERVER), N)
TARGETS += $(MW_DIR_BIN)/nanowm
endif

ifeq ($(NANOXDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of binaries to compile
ifneq ($(ARCH), ECOS)

# demos
TARGETS += \
	$(MW_DIR_BIN)/demo-arc \
	$(MW_DIR_BIN)/demo-blit \
	$(MW_DIR_BIN)/demo-composite \
	$(MW_DIR_BIN)/demo-monobitmap \
	$(MW_DIR_BIN)/demo-dash \
	$(MW_DIR_BIN)/demo-polygon \
	$(MW_DIR_BIN)/demo-region \
	$(MW_DIR_BIN)/demo-tilestipple \
	$(MW_DIR_BIN)/demo-grabkey \
	$(MW_DIR_BIN)/demo-ttfont \
	$(MW_DIR_BIN)/demo-font \
	$(MW_DIR_BIN)/demo-aafont \
	$(MW_DIR_BIN)/demo-hello

# games
TARGETS += \
	$(MW_DIR_BIN)/nxeyes \
	$(MW_DIR_BIN)/nxroach \
	$(MW_DIR_BIN)/ntetris \
	$(MW_DIR_BIN)/landmine \
	$(MW_DIR_BIN)/slider \
	$(MW_DIR_BIN)/world \
	$(MW_DIR_BIN)/tux

# tools
TARGETS += \
	$(MW_DIR_BIN)/nxmag \
	$(MW_DIR_BIN)/nxclock \
	$(MW_DIR_BIN)/nxview \
	$(MW_DIR_BIN)/nxlsclients \
	$(MW_DIR_BIN)/nxev \
	$(MW_DIR_BIN)/nxcal \
	$(MW_DIR_BIN)/nxsetportrait \
	$(MW_DIR_BIN)/show_font \
	$(MW_DIR_BIN)/show_ppm \
	$(MW_DIR_BIN)/snap_ppm \
	$(MW_DIR_BIN)/launcher \
	$(MW_DIR_BIN)/nsaver \
	$(MW_DIR_BIN)/npanel

NXKBDOBJS += \
	$(MW_DIR_OBJ)/demos/nanox/nxkbd.o \
	$(MW_DIR_OBJ)/demos/nanox/nxkbd_srvconn.o \
	$(MW_DIR_OBJ)/images/demos/nanox/nxkbd/keynorm.o \
	$(MW_DIR_OBJ)/images/demos/nanox/nxkbd/keyctrl.o \
	$(MW_DIR_OBJ)/images/demos/nanox/nxkbd/keyshft.o \
	$(MW_DIR_OBJ)/images/demos/nanox/nxkbd/keynum.o 
OBJS += $(NXKBDOBJS)
TARGETS += $(MW_DIR_BIN)/nxkbd

# terminal emulator
ifneq ($(ARCH),CYGWIN) 
  ifneq ($(ARCH),RTEMS)
    ifneq ($(ARCH),ANDROID)
      TARGETS += $(MW_DIR_BIN)/nxterm
    endif
  endif
endif

# screenshot
ifeq ($(HAVE_JPEG_SUPPORT), Y)
  ifeq ($(FRAMEBUFFER), Y)
    ifneq ($(LINK_APP_INTO_SERVER), Y)
      TARGETS += $(MW_DIR_BIN)/snap_jpg
    endif
  endif
endif

#
# C++ demos
#

# image conversion demo
CANNYOBJS += $(MW_DIR_OBJ)/demos/nanox/demo-convimage.o
CANNYOBJS += $(MW_DIR_OBJ)/demos/nanox/cannyedgedetect.o
CXXOBJS += $(CANNYOBJS)
TARGETS += $(MW_DIR_BIN)/demo-convimage

# Antigrain geometry demo
AGGOBJS += $(MW_DIR_OBJ)/demos/nanox/demo-agg.o
AGGOBJS += $(MW_DIR_OBJ)/demos/nanox/agglite.o
CXXOBJS += $(AGGOBJS)
TARGETS += $(MW_DIR_BIN)/demo-agg

else # ECOS
TARGETS += $(MW_DIR_BIN)/landmine.o $(MW_DIR_BIN)/ntetris.o $(MW_DIR_BIN)/world.o
endif

all: default $(TARGETS)	$(CXXTARGETS)
	$(CP) \
		$(MW_DIR_SRC)/demos/nanox/launcher.cnf \
		$(MW_DIR_SRC)/images/demos/nanox/world.map \
		$(MW_DIR_SRC)/images/demos/nanox/tux.gif \
		$(MW_DIR_SRC)/images/demos/nanox/slidebmp.bmp \
		$(MW_DIR_SRC)/images/demos/nanox/uponface.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/mwlogo.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/icons/*.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/icons/*.pgm \
		$(MW_DIR_SRC)/images/demos/nanox/aademo.txt \
		$(MW_DIR_BIN)

endif # NANOXDEMO
endif # NANOX

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

# These demos need -lm
NANOX_DEMOS_WITH_LIBM_LINK := \
	$(MW_DIR_BIN)/demo-font \
	$(MW_DIR_BIN)/demo-grabkey \
	$(MW_DIR_BIN)/nxeyes \
	$(MW_DIR_BIN)/nsaver

# These demos use a hardcoded link line.
NANOX_DEMOS_WITH_NONSTANDARD_LINK := \
	$(NANOX_DEMOS_WITH_LIBM_LINK) \
	$(MW_DIR_BIN)/nxkbd \
	$(MW_DIR_BIN)/demo-convimage \
	$(MW_DIR_BIN)/demo-agg

# All other demos use a standard link line, that just
# links against the Nano-X client libraries.
NANOX_DEMOS_WITH_STANDARD_LINK := \
	$(filter-out $(NANOX_DEMOS_WITH_NONSTANDARD_LINK),$(TARGETS))

# Standard link rule used for most demos.
$(NANOX_DEMOS_WITH_STANDARD_LINK): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/nanox/%.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(NANOXCLIENTLIBS) $(LDFLAGS)

# Link rule used for demos with libm.
$(NANOX_DEMOS_WITH_LIBM_LINK): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/nanox/%.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(NANOXCLIENTLIBS) $(LDFLAGS) $(LIBM)

$(MW_DIR_BIN)/nxkbd: $(NXKBDOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NXKBDOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)

$(MW_DIR_BIN)/demo-convimage: $(CANNYOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(CANNYOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)

$(MW_DIR_BIN)/demo-agg: $(AGGOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(AGGOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)

ifeq ($(ARCH), ECOS)
# Special build rules for linked in applications
$(MW_DIR_BIN)/landmine.o: $(MW_DIR_OBJ)/demos/nanox/landmine.o
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
#	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--retain-symbols-file -Wl,landmine.syms -Wl,--defsym -Wl,landmine_main=main -o XX.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,landmine_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o

$(MW_DIR_BIN)/ntetris.o: $(MW_DIR_OBJ)/demos/nanox/ntetris.o
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,ntetris_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o

$(MW_DIR_BIN)/world.o: $(MW_DIR_OBJ)/demos/nanox/world.o
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,world_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o

$(MW_DIR_BIN)/nxkbd.o: $(OBJS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -Wl,-r -Wl,--defsym -Wl,nxkbd_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o
endif

#
# special testing target
#$(CXXTARGETS): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/nanox/%.o $(CXXOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
#	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
#	echo $$@ $@
#	echo CXXOBJS $(CXXOBJS)
#	echo notdir $(notdir $@)
#	echo $(MW_DIR_OBJ)/$(MW_DIR_RELATIVE)$(notdir $@)
#	echo $(wildcard $(MW_DIR_OBJ)/$(MW_DIR_RELATIVE)$(notdir $@)*.o)
#	echo $(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(wildcard $(MW_DIR_OBJ)/$(MW_DIR_RELATIVE)$(notdir $@)*.o) $(NANOXCLIENTLIBS) $(LDFLAGS)
#	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(wildcard $(MW_DIR_OBJ)/$(MW_DIR_RELATIVE)$(notdir $@)*.o) $(NANOXCLIENTLIBS) $(LDFLAGS)
