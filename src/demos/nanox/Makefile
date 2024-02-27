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

ifeq ($(ARCH), ANDROID)
AAPT=$(ANDROID_SDK_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)/aapt
DX=$(ANDROID_SDK_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)/dx
ZIPALIGN=$(ANDROID_SDK_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)/zipalign
JAVAC=$(JAVA_HOME)/bin/javac
JARSIGNER=$(JAVA_HOME)/bin/jarsigner
endif


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

ifneq ($(ARCH), COSMO)

# tools
TARGETS += \
	$(MW_DIR_BIN)/nxmag \
	$(MW_DIR_BIN)/nxclock \
	$(MW_DIR_BIN)/nxview \
	$(MW_DIR_BIN)/nxlsclients \
	$(MW_DIR_BIN)/nxev \
	$(MW_DIR_BIN)/nxcal \
	$(MW_DIR_BIN)/nxsetportrait \
	$(MW_DIR_BIN)/show-font \
	$(MW_DIR_BIN)/show-ppm \
	$(MW_DIR_BIN)/screenshot-ppm \
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
      ifneq ($(ARCH),AQUILA)
        #TARGETS += $(MW_DIR_BIN)/nxterm
      endif
    endif
  endif
endif

# screenshot
ifeq ($(HAVE_JPEG_SUPPORT), Y)
  TARGETS += $(MW_DIR_BIN)/screenshot-jpg
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

endif # COSMO

else # ECOS
TARGETS += $(MW_DIR_BIN)/landmine.o $(MW_DIR_BIN)/ntetris.o $(MW_DIR_BIN)/world.o
endif

# needed to create android skel diroctories
ifeq ($(ARCH), ANDROID)
LDFLAGS+=-shared
ANDROID_SKEL := $(TARGETS:=-apk )
ANDROID_PACK := $(TARGETS:=-pack )
endif

all: default $(ANDROID_SKEL) $(TARGETS) $(CXXTARGETS) $(ANDROID_PACK)
ifneq ($(ARCH), ANDROID)
	$(CP) \
		$(MW_DIR_SRC)/demos/nanox/launcher.cnf \
		$(MW_DIR_SRC)/images/demos/nanox/world.map \
		$(MW_DIR_SRC)/images/demos/nanox/tux.gif \
		$(MW_DIR_SRC)/images/demos/nanox/slidebmp.bmp \
		$(MW_DIR_SRC)/images/demos/nanox/uponface.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/mwlogo.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/icons/*.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/icons/*.pgm \
		$(MW_DIR_BIN)
endif

endif # NANOXDEMO
endif # NANOX

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SCREEN), ALLEGRO5)
# don't link client apps with -lallegro
LDFLAGS := $(filter-out $(EXTENGINELIBS),$(LDFLAGS))
endif

ifneq ($(ARCH), COSMO)
# These demos need -lm
NANOX_DEMOS_WITH_LIBM_LINK := \
	$(MW_DIR_BIN)/demo-font \
	$(MW_DIR_BIN)/demo-grabkey \
	$(MW_DIR_BIN)/nxeyes \
	$(MW_DIR_BIN)/nsaver
endif

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
#	echo $(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(NANOXCLIENTLIBS) $(LDFLAGS)
ifeq ($(ARCH), ANDROID)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@-apk/lib/$(ANDROID_ARCH_VER)/$(subst -,,$(patsubst $(MW_DIR_BIN)/%,%,$@)).so $(NANOXCLIENTLIBS) $(LDFLAGS)
else
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(NANOXCLIENTLIBS) $(LDFLAGS) $(LDLIBS)
endif
ifeq ($(ARCH), COSMO)
	$(OBJCOPY) -S -O binary $@ $@.com
endif

# Link rule used for demos with libm.
$(NANOX_DEMOS_WITH_LIBM_LINK): $(MW_DIR_BIN)/%: $(MW_DIR_OBJ)/demos/nanox/%.o $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
ifeq ($(ARCH), ANDROID)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@-apk/lib/$(ANDROID_ARCH_VER)/$(subst -,,$(patsubst $(MW_DIR_BIN)/%,%,$@)).so $(NANOXCLIENTLIBS) $(LDFLAGS) $(LIBM)
else
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(NANOXCLIENTLIBS) $(LDFLAGS) $(LIBM) $(LDLIBS)
endif

$(MW_DIR_BIN)/nxkbd: $(NXKBDOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
ifeq ($(ARCH), ANDROID)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@-apk/lib/$(ANDROID_ARCH_VER)/$(subst -,,$(patsubst $(MW_DIR_BIN)/%,%,$@)).so $(NXKBDOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)
else
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(NXKBDOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)
endif

$(MW_DIR_BIN)/demo-convimage: $(CANNYOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
ifeq ($(ARCH), ANDROID)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@-apk/lib/$(ANDROID_ARCH_VER)/$(subst -,,$(patsubst $(MW_DIR_BIN)/%,%,$@)).so $(CANNYOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)
else
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(CANNYOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)
endif

$(MW_DIR_BIN)/demo-agg: $(AGGOBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
ifeq ($(ARCH), ANDROID)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@-apk/lib/$(ANDROID_ARCH_VER)/$(subst -,,$(patsubst $(MW_DIR_BIN)/%,%,$@)).so $(AGGOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)
else
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(AGGOBJS) $(NANOXCLIENTLIBS) $(LDFLAGS)
endif



ifeq ($(ARCH), ANDROID)
.PHONY: $(ANDROID_SKEL)
$(ANDROID_SKEL)/%:
	test -d $@ || mkdir -p $@
	$(CP) -r $(MW_DIR_SRC)/demos/nanox/android-skel/* $@/
	test -d $@/assets || mkdir -p $@/assets
	$(CP) \
		$(MW_DIR_SRC)/demos/nanox/launcher.cnf \
		$(MW_DIR_SRC)/images/demos/nanox/world.map \
		$(MW_DIR_SRC)/images/demos/nanox/tux.gif \
		$(MW_DIR_SRC)/images/demos/nanox/slidebmp.bmp \
		$(MW_DIR_SRC)/images/demos/nanox/uponface.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/mwlogo.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/icons/*.ppm \
		$(MW_DIR_SRC)/images/demos/nanox/icons/*.pgm \
		$@/assets
	test -d $@/lib/$(ANDROID_ARCH_VER) || mkdir -p $@/lib/$(ANDROID_ARCH_VER)
	$(CP) $(ALLEGRO5_LIB)/liballegro.so $@/lib/$(ANDROID_ARCH_VER)/
	$(CP) -r $(ALLEGRO5_SRC)/android/gradle_project/allegro/src/main/java/org/* $@/java/

$(MW_DIR_BIN)/%:
	find $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk -type f -exec sed -i 's/_skeleton/'"$(subst -,,$(patsubst $(MW_DIR_BIN)/%-pack,%,$@))"'/g' {} \;
	cd $(MW_DIR_BIN); \
	$(AAPT) package -f -m -J $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk -M $(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/AndroidManifest.xml \
	-I $(ANDROID_SDK_ROOT)/platforms/$(ANDROID_PLATFORM_VER)/android.jar  -F $(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk.unaligned

	test -d $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/classes || mkdir $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/classes
	$(JAVAC) -classpath $(ANDROID_SDK_ROOT)/platforms/$(ANDROID_PLATFORM_VER)/android.jar -sourcepath 'src:$(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/java' \
	-d '$(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/classes' -target 1.7 -source 1.7  $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/java/*.java

	$(DX) --dex --output=$(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/classes.dex $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/classes
	$(AAPT) package -f -M $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/AndroidManifest.xml -I $(ANDROID_SDK_ROOT)/platforms/$(ANDROID_PLATFORM_VER)/android.jar -F $(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk.unaligned
	cd $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk && $(AAPT) add $(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk.unaligned classes.dex
	cd $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk && $(AAPT) add -v $(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk.unaligned lib/$(ANDROID_ARCH_VER)/$(subst -,,$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)).so
	cd $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk && $(AAPT) add -v $(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk.unaligned lib/$(ANDROID_ARCH_VER)/liballegro.so

	cd $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk && find assets -type f -follow -exec  $(AAPT) add -v $(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk.unaligned {} \;

	$(JARSIGNER) -keystore $(ANDROID_KEYSTORE_PATH) -storepass '$(ANDROID_KEYSTORE_PW)'  $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk.unaligned  $(ANDROID_KEYSTORE_NAME)
	cd $(MW_DIR_BIN)/$(patsubst $(MW_DIR_BIN)/%-pack,%,$@)-apk && $(ZIPALIGN) -f 4 $(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk.unaligned $(patsubst $(MW_DIR_BIN)/%-pack,%,$@).apk
endif


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
