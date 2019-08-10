##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
# Portions Copyright (c) 2002 Koninklijke Philips Electronics N.V.
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/..
endif
MW_DIR_RELATIVE := nx11/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

#
# NXLIB Makefile
#
EXTRAFLAGS += -Wno-missing-prototypes			# nx11/*.c needs cleaning up
EXTRAFLAGS += -Wno-incompatible-pointer-types	# for Image.c
EXTRAFLAGS += -Wno-deprecated-declarations		# for XKeycodeToKeysym

# Compile-time X11 include directory, colornames and fonts locations
#
# Although X11 headers haven't changed structurally in years,
# these locations should match headers on target system.
# NXLIB includes local X11R6 headers for compilation testing in case X11
# isn't installed on the cross-compiling host.
#
# if X11 installed on host:
#	X11_INCLUDE=<from config file>
# if X11 not installed on host:
#	X11_INCLUDE=./X11-local
#
X11_INCLUDE=$(X11HDRLOCATION)
#X11_INCLUDE=./X11-local

# Run-time font directory:
# Modify FONT_DIR_LIST[] in fontlist.c to add font search directories!
# by default, fontlist.c includes "fonts/" as a relative search path

# Run-time font directory and rgb.txt file location:
# For testing, relative paths are ok, default is local fonts/rgb.txt
# For release, full paths are required matching target system locations
#	X11_RGBTXT=/usr/share/X11/rgb.txt
#

ifeq ($(X11_RGBTXT),)
X11_RGBTXT=$(MW_DIR_SRC)/nx11/fonts/rgb.txt
endif
#X11_RGBTXT=/usr/share/X11/rgb.txt

# set INCLUDE_XRM=Y to include (unmodifed X11) Xrm routines
#INCLUDE_XRM=Y

############################# targets section ################################

ifeq ($(NX11), Y)
LIBNAME = libNX11.a
LIBNAMESO = libNX11.so

ifneq ($(X11_INCLUDE),)
INCLUDEDIRS += -I$(X11_INCLUDE)
endif
EXTRAFLAGS += -DX11_RGBTXT=\"$(X11_RGBTXT)\"

NXOBJS = DestWind.o MapWindow.o NextEvent.o OpenDis.o ClDisplay.o\
	Window.o CrGC.o FreeGC.o StName.o Sync.o Flush.o CrWindow.o\
	Text.o DrLine.o DrLines.o DrPoint.o DrRect.o DrArc.o\
	MapRaised.o RaiseWin.o LowerWin.o FillRct.o CrPixmap.o Clear.o\
	MoveWin.o ClearArea.o UnmapWin.o RepWindow.o\
	ChWindow.o Backgnd.o BdrWidth.o Border.o PmapBgnd.o\
	fontlist.o font_find.o UnloadFont.o QueryFont.o\
	DefCursor.o UndefCurs.o CrCursor.o FontCursor.o\
	CrBFData.o CrPFBData.o Region.o SelInput.o Atom.o\
	QueryTree.o Image.o WindowProperty.o Misc.o SetWMProps.o Bell.o\
	Copy.o SetClip.o Visual.o StrToText.o SetAttributes.o FillPolygon.o\
	StrKeysym.o ChProperty.o QueryPointer.o ErrorHandler.o\
	ListPix.o GetGeom.o SetIFocus.o Shape.o\
	ClassHint.o Text16.o TextExt.o\
	AllocColor.o ParseColor.o QueryColor.o Colormap.o Colorname.o\
	Selection.o XMisc.o Free.o stub.o \
	Request.o Context.o Grab.o Screen.o Extension.o XKB.o Locale.o \
	Resource.o GetGCVals.o Threads.o SetWMProto.o FontInfo.o \
	GetPntMap.o GetWMProps.o ChSaveSet.o TextToStr.o QueryBest.o

# compile in X extension dummy stubs into NXLIB
NXOBJS += NXext.o
# add Xinerama functions
NXOBJS += Xinerama.o

# incompatible routine: missing real X11/Xlcint.h header on installed X11
#NXOBJS += IM.o

ifeq ($(INCLUDE_XRM), Y)
NXOBJS += Quarks.o Xrm.o ParseCmd.o
#NXOBJS += xrm/Xrm.o xrm/ParseCmd.o xrm/Misc.o xrm/Quarks.o xrm/lcWrap.o \
    xrm/lcInit.o xrm/lcGenConv.o xrm/SetLocale.o xrm/lcConv.o xrm/lcUTF8.o \
    xrm/lcDefConv.o xrm/lcPubWrap.o xrm/lcDynamic.o xrm/lcCharSet.o \
    xrm/lcDB.o xrm/lcGeneric.o xrm/lcUtil.o xrm/lcCT.o xrm/lcFile.o \
    xrm/lcPublic.o xrm/lcRM.o xrm/imInt.o
endif

OBJS = $(patsubst %.o,$(MW_DIR_OBJ)/$(MW_DIR_RELATIVE)%.o,$(NXOBJS))

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

distclean: clean
	rm -f keysymstr.h

keysymstr.h: 
	perl ./keymap.pl $(X11_INCLUDE)/X11 > ./keysymstr.h

