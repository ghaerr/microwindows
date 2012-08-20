#
# $Id$
#

include ../config

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := rtems/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

#Defines for preprocessor
ifeq ($(LINK_APP_INTO_SERVER), Y)
DEFINES += -DNONETWORK=1
endif

# Compilation flags for C files OTHER than include directories
# CFLAGS += $(INCLUDEDIRS)
# Preprocessor flags OTHER than defines
# CPPFLAGS += $(INCLUDEDIRS)
# Linking flags
LDFLAGS += $(LIBJPEG)
LDFLAGS += $(LIBPNG)
LDFLAGS += $(LIBTIFF)
LDFLAGS += $(LIBFT2LIB)
LDFLAGS += $(LIBZ)
LDFLAGS += $(LIBT1LIB)

############################# targets section ################################

ifeq ($(ARCH), RTEMS)

# List of objects always needed by a RTEMS application
OBJS =	$(MW_DIR_OBJ)/FilesystemImage.o 
OBJS += $(MW_DIR_OBJ)/rtems_init.o 

all: default stamp-filesystem $(OBJS)

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
# let's adjust the libraries to match the API that you are using.

$(MW_DIR_OBJ)/FilesystemImage.o: stamp-filesystem \
	FilesystemImage.c FilesystemImage.h
	$(CC) -pipe $(CFLAGS) FilesystemImage.c -c -o $@

FilesystemImage.c: stamp-filesystem

FilesystemImage.h: stamp-filesystem

$(MW_DIR_OBJ)/rtems_init.o: stamp-filesystem rtems_init.c \
	FilesystemImage.h
	$(CC) $(CFLAGS) rtems_init.c -c -o $@

# XXX clean up --  how do we clean it up :)  Need to
# XXX    rm -rf rootfs, FilesystemImage, and stamp-filesystem
stamp-filesystem: Makefile
	rm -rf rootfs 
ifeq ($(HAVE_JPEG_SUPPORT),Y)
	# /logos
	mkdir -p rootfs/logos
	cp RTEMS_logo.jpg rootfs/logos
endif
ifeq ($(HAVE_PNG_SUPPORT),Y)
	# /logos
	mkdir -p rootfs/logos
	cp RTEMS_logo.png rootfs/logos
endif
ifeq ($(HAVE_TIFF_SUPPORT),Y)
	# /logos
	mkdir -p rootfs/logos
	cp RTEMS_logo.tiff rootfs/logos
endif
	# /bin
	mkdir -p rootfs/bin
	cp ../../src/demos/nanox/tux.gif rootfs/bin
	cp ../../src/demos/nxscribble/*.cl rootfs/bin
	# /demos/nanox
	mkdir -p rootfs/demos/nanox
	cp ../../src/demos/nanox/world.map rootfs/demos/nanox
	# /demos/tuxchess/images
	mkdir -p rootfs/demos/tuxchess/images
	cp ../../src/demos/tuxchess/images/*gif rootfs/demos/tuxchess/images
	# /fonts
	mkdir -p rootfs/fonts
	cp ../../src/fonts/*fon rootfs/fonts
	# /fonts/bdf
	mkdir -p rootfs/fonts/bdf
	cp ../../src/fonts/bdf/helvB10.bdf rootfs/fonts/bdf
	cp ../../src/fonts/bdf/helvB12.bdf rootfs/fonts/bdf
	cp ../../src/fonts/bdf/helvR10.bdf rootfs/fonts/bdf
	cp ../../src/fonts/bdf/lubI24.bdf rootfs/fonts/bdf
	cp ../../src/fonts/bdf/timBI18.bdf rootfs/fonts/bdf
	cp ../../src/fonts/bdf/X5x7.bdf rootfs/fonts/bdf
	# /fonts/pcf
	mkdir -p rootfs/$(PCF_FONT_DIR)
ifeq ($(HAVE_PCF_SUPPORT),Y)
	cp ../../src/fonts/pcf/lubI24.pcf rootfs/fonts/pcf
endif
ifeq ($(HAVE_PCFGZ_SUPPORT),Y)
	cp ../../src/fonts/pcf/6x13.pcf.gz rootfs/fonts/pcf
	cp ../../src/fonts/pcf/7x14.pcf.gz rootfs/fonts/pcf
	cp ../../src/fonts/pcf/9x15.pcf.gz rootfs/fonts/pcf
	cp ../../src/fonts/pcf/helvB12_lin.pcf.gz rootfs/fonts/pcf
	cp ../../src/fonts/pcf/helvB12.pcf.gz rootfs/fonts/pcf
	cp ../../src/fonts/pcf/vga.pcf.gz rootfs/fonts/pcf
endif
ifeq ($(HAVE_T1LIB_SUPPORT),Y)
	# /fonts/type1
	mkdir -p rootfs/fonts/type1
	cp ../../src/fonts/type1/FontDataBase rootfs/fonts/type1
	cp ../../src/fonts/type1/t1lib.config rootfs/fonts/type1
	mkdir -p rootfs/fonts/type1/afm
	cp ../../src/fonts/type1/*.afm rootfs/fonts/type1/afm
	mkdir -p rootfs/fonts/type1/enc
	cp ../../src/fonts/type1/*.enc rootfs/fonts/type1/enc
	mkdir -p rootfs/fonts/type1/pfb
	cp ../../src/fonts/type1/*.pfb rootfs/fonts/type1/pfb
endif
ifeq ($(HAVE_FREETYPE_2_SUPPORT),Y)
	# /fonts/truetype
	mkdir -p rootfs/fonts/truetype
	cp ../../src/fonts/truetype/arial*.ttf rootfs/fonts/truetype
	cp ../../src/fonts/truetype/times*.ttf rootfs/fonts/truetype
endif
	# add other user files
	cp add_to_root/* rootfs/ -r
	# Tar
	cd rootfs ; \
	    tar cf ../FilesystemImage --exclude CVS --exclude .cvsignore .
	$(RTEMS_MAKEFILE_PATH)/../../bin/rtems-bin2c \
		FilesystemImage FilesystemImage
	touch stamp-filesystem
endif
