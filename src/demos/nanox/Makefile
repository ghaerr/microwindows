##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC = ../..
endif
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

# List of objects to compile
ifeq ($(ARCH), ECOS)
OBJS = landmine.o netris.o world.o
TARGETS = $(MW_DIR_BIN)/landmine.o $(MW_DIR_BIN)/ntetris.o $(MW_DIR_BIN)/world.o
else
OBJS =	demo.o \
	move.o \
	landmine.o \
	world.o \
	nxclock.o \
	nxterm.o \
	nxview.o \
	nxlsclients.o \
	nxev.o \
	nxmag.o \
	npanel.o \
	nterm.o \
	demo2.o \
	demo5.o \
	demo6.o \
	t1demo.o \
	ftdemo.o \
	logfont.o \
	slider.o \
	ntetris.o \
	getselection.o \
	setselection.o \
	dashdemo.o \
	tsdemo.o \
	pcfdemo.o \
	fontdemo.o \
	setportrait.o \
	arcdemo.o \
	polydemo.o \
	snap_ppm.o \
	bmpdemo.o \
	rgndemo.o \
	convpbm.o \
	launcher.o \
	nsaver.o \
	nxeyes.o \
	tux.o \
	nxcal.o \
	grabdemo.o

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
OBJS +=	snap_jpg.o
TARGETS += $(MW_DIR_BIN)/snap_jpg
endif
endif
endif

endif

all: default $(TARGETS)	
	$(CP) world.map launcher.cnf tux.gif slidebmp.bmp uponface.ppm \
		mwlogo.ppm icons/*.ppm icons/*.pgm ftdemo.txt\
		$(MW_DIR_BIN)
#		transtest.gif alphatest.png \

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

$(MW_DIR_BIN)/demo: demo.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/move: move.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/landmine: landmine.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/world: world.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nxclock: nxclock.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nxterm: nxterm.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nxview: nxview.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nxlsclients: nxlsclients.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nxev: nxev.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nxmag: nxmag.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/npanel: npanel.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nterm: nterm.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/demo2: demo2.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/demo5: demo5.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/demo6: demo6.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/t1demo: t1demo.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/ftdemo: ftdemo.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/logfont: logfont.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/slider: slider.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/ntetris: ntetris.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/launcher: launcher.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nsaver: nsaver.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS) $(LIBM)

$(MW_DIR_BIN)/getselection: getselection.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/setselection: setselection.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/dashdemo: dashdemo.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/tsdemo: tsdemo.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/pcfdemo: pcfdemo.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/fontdemo: fontdemo.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/pressure: pressure.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/setportrait: setportrait.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/oneshot: oneshot.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/periodic: periodic.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/windowproperties: windowproperties.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/ipcserver: ipcserver.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/ipcclient: ipcclient.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/arcdemo: arcdemo.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/polydemo: polydemo.o $($NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/snap_ppm: snap_ppm.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/snap_jpg: snap_jpg.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS) $(LIBJPEG)

$(MW_DIR_BIN)/nxcal: nxcal.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/bmpdemo: bmpdemo.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/rgndemo: rgndemo.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/convpbm: convpbm.o
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(MW_DIR_BIN)/tux: tux.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS)

$(MW_DIR_BIN)/nxeyes: nxeyes.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS) $(LIBM)

$(MW_DIR_BIN)/grabdemo: grabdemo.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LD_NANOXCLIENTLIBS) $(LIBM)

ifeq ($(ARCH), ECOS)
# Special build rules for linked in applications
$(MW_DIR_BIN)/landmine.o: landmine.o
	@echo "Linking $@ ..."
#	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--retain-symbols-file -Wl,landmine.syms -Wl,--defsym -Wl,landmine_main=main -o XX.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,landmine_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o

$(MW_DIR_BIN)/ntetris.o: ntetris.o
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,ntetris_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o

$(MW_DIR_BIN)/world.o: world.o
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $< -Wl,-r -Wl,--defsym -Wl,world_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o
endif
