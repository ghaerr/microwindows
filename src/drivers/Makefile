##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
##############################################################################

ifndef TOP
TOP = ..
CONFIG = $(TOP)/config
endif

include $(CONFIG)

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME = libmwdrivers.a
#LIBNAMESO = libmwdrivers.so

# List of objects to compile

# for UNIX/Linux flavours
ifneq ($(ARCH), ELKS)
ifneq ($(ARCH), DJGPP)

ifeq ($(EM8400), Y)
OBJS += scr_em84xx.o genfont.o genmem.o
endif

ifeq ($(FRAMEBUFFER), Y)
OBJS += fblin8.o fblin16.o fblin24.o fblin32.o fblin32alpha.o genmem.o fb.o

ifeq ($(FBREVERSE), Y)
OBJS += fblin1rev.o fblin2rev.o
else
OBJS += fblin1.o fblin2.o
endif

ifeq ($(FBVGA), Y)
OBJS += vgaplan4.o memplan4.o
endif

ifeq ($(NOFONTSORCLIPPING), Y)
OBJS += nofont.o
else
OBJS += genfont.o
endif

ifeq ($(ARCH), ECOS)
OBJS += scr_ecos.o
else
ifeq ($(ARCH), RTEMS)
OBJS += scr_rtems.o romfont.o
else
OBJS += scr_fb.o fbportrait_left.o fbportrait_right.o fbportrait_down.o
endif
endif

ifeq ($(FBREVERSE), Y)
OBJS += fblin4rev.o
else
ifeq ($(SA1100_LCD_LTLEND), Y)
OBJS += fblin4sa1100.o
else
OBJS += fblin4.o
endif
endif

endif # FRAMEBUFER

ifeq ($(VTSWITCH), Y)
OBJS += vtswitch.o
endif

ifeq ($(X11), Y)
OBJS += genmem.o fb.o scr_x11.o \
	fblin1.o fblin2.o fblin4.o fblin8.o fblin16.o fblin24.o fblin32.o \
	fblin32alpha.o mou_x11.o 

ifneq ($(PIPEKBD), Y)
OBJS += kbd_x11.o
else
OBJS += kbd_pipe.o
endif

ifeq ($(NOFONTSORCLIPPING), Y)
OBJS += nofont.o
else
OBJS += genfont.o
endif

endif # X11 stuff

ifeq ($(ARCH), FREEBSD-X86)
OBJS += scr_fbsd.o
OBJS += genfont.o genmem.o fb.o \
	fblin1.o fblin2.o fblin4.o fblin8.o fblin16.o fblin24.o fblin32.o
endif

ifeq ($(VGALIB), Y)
OBJS +=	scr_svga.o genfont.o genmem.o
endif

ifeq ($(HWVGA), Y)
OBJS +=	scr_bios.o vgainit.o genfont.o genmem.o
endif

ifneq ($(X11), Y)

### Desktop GPM mouse driver
ifeq ($(GPMMOUSE), Y)
OBJS += mou_gpm.o
endif

### serial mouse driver
ifeq ($(SERMOUSE), Y)
OBJS += mou_ser.o
endif

### Generic touchscreen driver
ifeq ($(ADSMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_ADS=1
OBJS += mou_touchscreen.o
endif

ifeq ($(ADS7846MOUSE), Y)
CFLAGS += -DTOUCHSCREEN_ADS7846=1
OBJS += mou_touchscreen.o
endif

ifeq ($(IPAQMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_IPAQ=1
OBJS += mou_touchscreen.o
endif

ifeq ($(TUXSCREENMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_TUXSCREEN=1
OBJS += mou_touchscreen.o
endif

ifeq ($(ZAURUSMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_ZAURUS=1
OBJS += mou_touchscreen.o
endif

### Embedded Planet touchscreen
ifeq ($(EPMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_EPLANET
OBJS += mou_vrlinux.o
endif

### Vtech Helio touchscreen
ifeq ($(VHMOUSE), Y)
CFLAGS += -DTOUCHSCREEN_HELIO=1
OBJS += mou_vrlinux.o
endif

#### Yopy touchscreen
ifeq ($(YOPYMOUSE), Y)
OBJS += mou_yopy.o
endif

#### Psion 5 touchscreen
ifeq ($(PSIONMOUSE), Y)
OBJS += mou_ps5.o
endif

#### NEC Harrier touchscreen
ifeq ($(HARRIERMOUSE), Y)
OBJS += mou_harrier.o
endif

#### MicroTouch serial touchscreen
ifeq ($(MTMOUSE), Y)
OBJS += mou_mt.o
endif

ifeq ($(SUNMOUSE), Y)
OBJS += mou_sun.o
endif

ifeq ($(ARCH), FREEBSD-X86)
OBJS += mou_fbsd.o
endif

ifeq ($(NOMOUSE), Y)
OBJS += mou_null.o
endif

#
# Keyboard driver specific for every system
#
ifneq ($(PIPEKBD), Y)

ifeq ($(TTYKBD), Y)
OBJS += kbd_tty.o
endif

ifeq ($(IPAQKBD), Y)
OBJS += kbd_ipaq.o
endif

ifeq ($(ZAURUSKBD),Y)
CFLAGS += -DKBD_ZAURUS=1
OBJS += kbd_ttyscan.o
endif

ifeq ($(SCANKBD), Y)
OBJS += kbd_ttyscan.o
endif

ifeq ($(ARCH), FREEBSD-X86)
#OBJS += kbd_fbsd.o
OBJS += kbd_tty.o
endif

ifeq ($(LIRCKBD), Y)
OBJS += mwlirc.o kbd_lirc.o
endif

ifeq ($(NOKBD), Y)
OBJS += kbd_null.o
endif

else

OBJS += kbd_pipe.o

endif # PIPEKBD != Y

ifeq ($(ARCH), RTEMS)
OBJS += input_rtems.o

ifeq ($(NANOX), Y)
OBJS += gsselect_rtems.o
endif

ifeq ($(MICROWIN), Y)
OBJS += mwselect_rtems.o
endif

endif # RTEMS architecture


endif # !X11

endif # !DJGPP
endif # !ELKS

# For DJGPP flavor
ifeq ($(ARCH), DJGPP)

OBJS += scr_djgr.o genfont.o mou_dos.o kbd_djgr.o

endif

# For ELKS flavor
ifeq ($(ARCH), ELKS)

OBJS += elksutil.o romfont.o

ifeq ($(ASMVGADRIVER), Y)
OBJS += scr_bios.o elkplan4.o memplan4.o genmem.o
endif

ifeq ($(CVGADRIVER), Y)
OBJS += scr_bios.o vgaplan4.o memplan4.o genmem.o
endif

ifeq ($(HERCDRIVER), Y)
OBJS += scr_herc.o
endif

ifeq ($(DBGDRIVER), Y)
OBJS += scr_debug.o genfont.o genmem.o
endif

ifeq ($(SERMOUSE), Y)
OBJS += mou_ser.o
endif

OBJS +=	kbd_tty.o

endif

# uClinux on an Isicad Prisma
ifeq ($(ARCH), UCLINUX-PRISMA)
OBJS += scr_prsm.o genfont.o genmem.o
ASMOBJS += asm_prsm.o
endif

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################
