##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := drivers/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwdrivers.a
#LIBNAMESO = libmwdrivers.so

# List of objects to compile

OBJ :=

# for UNIX/Linux flavours
ifneq ($(ARCH), ELKS)
ifneq ($(ARCH), DJGPP)

ifeq ($(EM8400), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_em84xx.o \
	$(MW_DIR_OBJ)/drivers/genfont.o \
	$(MW_DIR_OBJ)/drivers/genmem.o
endif

ifeq ($(FRAMEBUFFER), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/fblin8.o \
	$(MW_DIR_OBJ)/drivers/fblin16.o \
	$(MW_DIR_OBJ)/drivers/fblin24.o \
	$(MW_DIR_OBJ)/drivers/fblin32.o \
	$(MW_DIR_OBJ)/drivers/fblin32alpha.o \
	$(MW_DIR_OBJ)/drivers/genmem.o \
	$(MW_DIR_OBJ)/drivers/fb.o

ifeq ($(FBREVERSE), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/fblin1rev.o \
	$(MW_DIR_OBJ)/drivers/fblin2rev.o
else
OBJS += \
	$(MW_DIR_OBJ)/drivers/fblin1.o \
	$(MW_DIR_OBJ)/drivers/fblin2.o
endif

ifeq ($(FBVGA), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/vgaplan4.o \
	$(MW_DIR_OBJ)/drivers/memplan4.o
endif

ifeq ($(NOFONTSORCLIPPING), Y)
OBJS += $(MW_DIR_OBJ)/drivers/nofont.o
else
OBJS += $(MW_DIR_OBJ)/drivers/genfont.o
endif

ifeq ($(ARCH), ECOS)
OBJS += $(MW_DIR_OBJ)/drivers/scr_ecos.o
else
ifeq ($(ARCH), RTEMS)
OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_rtems.o \
	$(MW_DIR_OBJ)/drivers/romfont.o
else
OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_fb.o \
	$(MW_DIR_OBJ)/drivers/fbportrait_left.o \
	$(MW_DIR_OBJ)/drivers/fbportrait_right.o \
	$(MW_DIR_OBJ)/drivers/fbportrait_down.o
endif
endif

ifeq ($(FBREVERSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/fblin4rev.o
else
ifeq ($(SA1100_LCD_LTLEND), Y)
OBJS += $(MW_DIR_OBJ)/drivers/fblin4sa1100.o
else
OBJS += $(MW_DIR_OBJ)/drivers/fblin4.o
endif
endif

endif # FRAMEBUFER

ifeq ($(VTSWITCH), Y)
OBJS += $(MW_DIR_OBJ)/drivers/vtswitch.o
endif

ifeq ($(X11), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/genmem.o \
	$(MW_DIR_OBJ)/drivers/fb.o \
	$(MW_DIR_OBJ)/drivers/scr_x11.o \
	$(MW_DIR_OBJ)/drivers/fblin1.o \
	$(MW_DIR_OBJ)/drivers/fblin2.o \
	$(MW_DIR_OBJ)/drivers/fblin4.o \
	$(MW_DIR_OBJ)/drivers/fblin8.o \
	$(MW_DIR_OBJ)/drivers/fblin16.o \
	$(MW_DIR_OBJ)/drivers/fblin24.o \
	$(MW_DIR_OBJ)/drivers/fblin32.o \
	$(MW_DIR_OBJ)/drivers/fblin32alpha.o \
	$(MW_DIR_OBJ)/drivers/mou_x11.o 

ifneq ($(PIPEKBD), Y)
OBJS += $(MW_DIR_OBJ)/drivers/kbd_x11.o
else
OBJS += $(MW_DIR_OBJ)/drivers/kbd_pipe.o
endif

ifeq ($(NOFONTSORCLIPPING), Y)
OBJS += $(MW_DIR_OBJ)/drivers/nofont.o
else
OBJS += $(MW_DIR_OBJ)/drivers/genfont.o
endif

endif # X11 stuff

ifeq ($(ARCH), FREEBSD-X86)
OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_fbsd.o \
	$(MW_DIR_OBJ)/drivers/genfont.o \
	$(MW_DIR_OBJ)/drivers/genmem.o \
	$(MW_DIR_OBJ)/drivers/fb.o \
	$(MW_DIR_OBJ)/drivers/fblin1.o \
	$(MW_DIR_OBJ)/drivers/fblin2.o \
	$(MW_DIR_OBJ)/drivers/fblin4.o \
	$(MW_DIR_OBJ)/drivers/fblin8.o \
	$(MW_DIR_OBJ)/drivers/fblin16.o \
	$(MW_DIR_OBJ)/drivers/fblin24.o \
	$(MW_DIR_OBJ)/drivers/fblin32.o
endif

ifeq ($(VGALIB), Y)
OBJS +=	\
	$(MW_DIR_OBJ)/drivers/scr_svga.o \
	$(MW_DIR_OBJ)/drivers/genfont.o \
	$(MW_DIR_OBJ)/drivers/genmem.o
endif

ifeq ($(HWVGA), Y)
OBJS +=	\
	$(MW_DIR_OBJ)/drivers/scr_bios.o \
	$(MW_DIR_OBJ)/drivers/vgainit.o \
	$(MW_DIR_OBJ)/drivers/genfont.o \
	$(MW_DIR_OBJ)/drivers/genmem.o
endif

ifneq ($(X11), Y)

### Desktop GPM mouse driver
ifeq ($(GPMMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_gpm.o
endif

### serial mouse driver
ifeq ($(SERMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_ser.o
endif

### Generic touchscreen driver
ifeq ($(ADSMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_ADS=1
OBJS += $(MW_DIR_OBJ)/drivers/mou_touchscreen.o
endif

ifeq ($(ADS7846MOUSE), Y)
CFLAGS += -DTOUCHSCREEN_ADS7846=1
OBJS += $(MW_DIR_OBJ)/drivers/mou_touchscreen.o
endif

ifeq ($(IPAQMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_IPAQ=1
OBJS += $(MW_DIR_OBJ)/drivers/mou_touchscreen.o
endif

ifeq ($(TUXSCREENMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_TUXSCREEN=1
OBJS += $(MW_DIR_OBJ)/drivers/mou_touchscreen.o
endif

ifeq ($(ZAURUSMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_ZAURUS=1
OBJS += $(MW_DIR_OBJ)/drivers/mou_touchscreen.o
endif

### Embedded Planet touchscreen
ifeq ($(EPMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_EPLANET
OBJS += $(MW_DIR_OBJ)/drivers/mou_vrlinux.o
endif

### Vtech Helio touchscreen
ifeq ($(VHMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_HELIO=1
OBJS += $(MW_DIR_OBJ)/drivers/mou_vrlinux.o
endif

#### Yopy touchscreen
ifeq ($(YOPYMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_yopy.o
endif

#### Psion 5 touchscreen
ifeq ($(PSIONMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_ps5.o
endif

#### NEC Harrier touchscreen
ifeq ($(HARRIERMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_harrier.o
endif

#### MicroTouch serial touchscreen
ifeq ($(MTMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_mt.o
endif

ifeq ($(SUNMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_sun.o
endif

ifeq ($(ARCH), FREEBSD-X86)
OBJS += $(MW_DIR_OBJ)/drivers/mou_fbsd.o
endif

ifeq ($(NOMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_null.o
endif

#
# Keyboard driver specific for every system
#
ifneq ($(PIPEKBD), Y)

ifeq ($(TTYKBD), Y)
OBJS += $(MW_DIR_OBJ)/drivers/kbd_tty.o
endif

ifeq ($(IPAQKBD), Y)
OBJS += $(MW_DIR_OBJ)/drivers/kbd_ipaq.o
endif

ifeq ($(ZAURUSKBD),Y)
CFLAGS += -DKBD_ZAURUS=1
OBJS += $(MW_DIR_OBJ)/drivers/kbd_ttyscan.o
endif

ifeq ($(SCANKBD), Y)
OBJS += $(MW_DIR_OBJ)/drivers/kbd_ttyscan.o
endif

ifeq ($(ARCH), FREEBSD-X86)
#OBJS += $(MW_DIR_OBJ)/drivers/kbd_fbsd.o
OBJS += $(MW_DIR_OBJ)/drivers/kbd_tty.o
endif

ifeq ($(LIRCKBD), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/mwlirc.o \
	$(MW_DIR_OBJ)/drivers/kbd_lirc.o
endif

ifeq ($(NOKBD), Y)
OBJS += $(MW_DIR_OBJ)/drivers/kbd_null.o
endif

else

OBJS += $(MW_DIR_OBJ)/drivers/kbd_pipe.o

endif # PIPEKBD != Y

ifeq ($(ARCH), RTEMS)
OBJS += $(MW_DIR_OBJ)/drivers/input_rtems.o

ifeq ($(NANOX), Y)
OBJS += $(MW_DIR_OBJ)/drivers/gsselect_rtems.o
endif

ifeq ($(MICROWIN), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mwselect_rtems.o
endif

endif # RTEMS architecture


endif # !X11

ifeq ($(LIRCKBD2), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/mwlirc.o \
	$(MW_DIR_OBJ)/drivers/kbd_lirc.o
endif

endif # !DJGPP
endif # !ELKS

# For DJGPP flavor
ifeq ($(ARCH), DJGPP)

OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_djgr.o \
	$(MW_DIR_OBJ)/drivers/genfont.o \
	$(MW_DIR_OBJ)/drivers/mou_dos.o \
	$(MW_DIR_OBJ)/drivers/kbd_djgr.o

endif

# For ELKS flavor
ifeq ($(ARCH), ELKS)

OBJS += \
	$(MW_DIR_OBJ)/drivers/elksutil.o \
	$(MW_DIR_OBJ)/drivers/romfont.o

ifeq ($(ASMVGADRIVER), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_bios.o \
	$(MW_DIR_OBJ)/drivers/elkplan4.o \
	$(MW_DIR_OBJ)/drivers/memplan4.o \
	$(MW_DIR_OBJ)/drivers/genmem.o
endif

ifeq ($(CVGADRIVER), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_bios.o \
	$(MW_DIR_OBJ)/drivers/vgaplan4.o \
	$(MW_DIR_OBJ)/drivers/memplan4.o \
	$(MW_DIR_OBJ)/drivers/genmem.o
endif

ifeq ($(HERCDRIVER), Y)
OBJS += $(MW_DIR_OBJ)/drivers/scr_herc.o
endif

ifeq ($(DBGDRIVER), Y)
OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_debug.o \
	$(MW_DIR_OBJ)/drivers/genfont.o \
	$(MW_DIR_OBJ)/drivers/genmem.o
endif

ifeq ($(SERMOUSE), Y)
OBJS += $(MW_DIR_OBJ)/drivers/mou_ser.o
endif

OBJS +=	$(MW_DIR_OBJ)/drivers/kbd_tty.o

endif

# uClinux on an Isicad Prisma
ifeq ($(ARCH), UCLINUX-PRISMA)
OBJS += \
	$(MW_DIR_OBJ)/drivers/scr_prsm.o \
	$(MW_DIR_OBJ)/drivers/genfont.o \
	$(MW_DIR_OBJ)/drivers/genmem.o
ASMOBJS += \
	$(MW_DIR_OBJ)/drivers/asm_prsm.o
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
