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
LIBM = -lm

############################# targets section ################################

ifeq ($(NANOX), Y)
ifeq ($(NANOXDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of binaries to compile
ifeq ($(ARCH), ECOS)
TARGETS = $(MW_DIR_BIN)/landmine.o $(MW_DIR_BIN)/ntetris.o $(MW_DIR_BIN)/world.o
else
TARGETS = \
	$(MW_DIR_BIN)/demo \
	$(MW_DIR_BIN)/move \
	$(MW_DIR_BIN)/landmine \
	$(MW_DIR_BIN)/world \
	$(MW_DIR_BIN)/nxclock \
	$(MW_DIR_BIN)/nxlsclients \
	$(MW_DIR_BIN)/nxev \
	$(MW_DIR_BIN)/npanel \
	$(MW_DIR_BIN)/nterm \
	$(MW_DIR_BIN)/demo2 \
	$(MW_DIR_BIN)/demo5 \
	$(MW_DIR_BIN)/demo6 \
	$(MW_DIR_BIN)/t1demo \
	$(MW_DIR_BIN)/ftdemo \
	$(MW_DIR_BIN)/logfont \
	$(MW_DIR_BIN)/ntetris \
	$(MW_DIR_BIN)/getselection \
	$(MW_DIR_BIN)/setselection \
	$(MW_DIR_BIN)/dashdemo \
	$(MW_DIR_BIN)/pcfdemo \
	$(MW_DIR_BIN)/fontdemo \
	$(MW_DIR_BIN)/setportrait \
	$(MW_DIR_BIN)/arcdemo \
	$(MW_DIR_BIN)/polydemo \
	$(MW_DIR_BIN)/snap_ppm \
	$(MW_DIR_BIN)/bmpdemo \
	$(MW_DIR_BIN)/rgndemo \
	$(MW_DIR_BIN)/convpbm \
	$(MW_DIR_BIN)/nsaver \
	$(MW_DIR_BIN)/nxcal \
	$(MW_DIR_BIN)/grabdemo

ifneq ($(ARCH),CYGWIN)
TARGETS += \
	$(MW_DIR_BIN)/nxterm
endif

# Require MW_FEATURE_IMAGES support:
TARGETS += \
	$(MW_DIR_BIN)/nxview \
	$(MW_DIR_BIN)/slider \
	$(MW_DIR_BIN)/tsdemo \
	$(MW_DIR_BIN)/tux

# Require MW_FEATURE_TIMERS support:
TARGETS += \
	$(MW_DIR_BIN)/nxmag \
	$(MW_DIR_BIN)/nxeyes

# Require both MW_FEATURE_IMAGES and MW_FEATURE_TIMERS support:
TARGETS += \
	$(MW_DIR_BIN)/launcher \

ifeq ($(HAVE_JPEG_SUPPORT), Y)
ifeq ($(FRAMEBUFFER), Y)
ifneq ($(LINK_APP_INTO_SERVER), Y)
TARGETS += $(MW_DIR_BIN)/snap_jpg
endif
endif
endif

endif

all: default $(TARGETS)	
	$(CP) \
		$(MW_DIR_SRC)/demos/nanox/world.map \
		$(MW_DIR_SRC)/demos/nanox/launcher.cnf \
		$(MW_DIR_SRC)/demos/nanox/tux.gif \
		$(MW_DIR_SRC)/demos/nanox/slidebmp.bmp \
		$(MW_DIR_SRC)/demos/nanox/uponface.ppm \
		$(MW_DIR_SRC)/demos/nanox/mwlogo.ppm \
		$(MW_DIR_SRC)/demos/nanox/icons/*.ppm \
		$(MW_DIR_SRC)/demos/nanox/icons/*.pgm \
		$(MW_DIR_SRC)/demos/nanox/ftdemo.txt\
		$(MW_DIR_BIN)

#		$(MW_DIR_SRC)/demos/nanox/transtest.gif
#		$(MW_DIR_SRC)/demos/nanox/alphatest.png

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

ifeq ($(SHAREDLIBS), Y)
LD_NANOXCLIENTLIBS = $(CCNANOXCLIENTLIBS)
else
LD_NANOXCLIENTLIBS = $(NANOXCLIENTLIBS)
endif

######################## Tools targets section ###############################

# These demos need -lm
NANOX_DEMOS_WITH_LIBM_LINK := \
	$(MW_DIR_BIN)/nxeyes \
	$(MW_DIR_BIN)/grabdemo \
	$(MW_DIR_BIN)/nsaver

# These demos use a hardcoded link line.
NANOX_DEMOS_WITH_NONSTANDARD_LINK := \
	$(NANOX_DEMOS_WITH_LIBM_LINK) \
	$(MW_DIR_BIN)/snap_jpg \
	$(MW_DIR_BIN)/convpbm

# All other demos use a standard link line, that just
# links against the Nano-X client libraries.
NANOX_DEMOS_WITH_STANDARD_LINK := \
	$(filter-out $(NANOX_DEMOS_WITH_NONSTANDARD_LINK),$(TARGETS))

# Standard link rule used for most demos.
$(NANOX_DEMOS_WITH_STANDARD_LINK): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/nanox/%.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

# Link rule used for demos with libm.
$(NANOX_DEMOS_WITH_LIBM_LINK): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/nanox/%.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS) $(LIBM)

$(MW_DIR_BIN)/snap_jpg: $(MW_DIR_OBJ)/demos/nanox/snap_jpg.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS) $(LIBJPEG)

$(MW_DIR_BIN)/convpbm: $(MW_DIR_OBJ)/demos/nanox/convpbm.o
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

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
endif
