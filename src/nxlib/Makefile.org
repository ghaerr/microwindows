# 10 Dec 2018
#
# NXLIB library makefile - X11 Conversion Library for Nano-X
# Greg Haerr <greg@censoft.com>
#
# Note: if build fails, rebuild keysymstr.h by "make distclean", then "make"
#

# get BIGENDIAN, OPTFLAGS settings
include $(CONFIG)

# Compile-time nano-X include and lib directories. (relative paths ok)
#
MWIN_INCLUDE=../include -I/usr/local/include -I./include
MWIN_LIB=../lib

X11_INCLUDE=.
X11_RGBTXT=fonts/rgb.txt

ifeq ($(DOS_DJGPP), Y)
MWIN_INCLUDE=/djgpp/include/mwin -I/myprogs/nxlib
endif

# Compile-time X11 include directory and X11 release location
#
# Although X11 headers haven't changed structurally in years,
# these locations should match headers on target system.
# NXLIB includes local X11R6 headers in case X11 isn't installed
# on the cross-compiling host.
#
# if X11 not installed:
#	X11_INCLUDE=.
# if X11 installed:
#	X11_INCLUDE=/usr/include/X11
#
#X11_INCLUDE=/usr/X11/include

# Run-time font directory and rgb.txt file location:
# For testing, relative paths are ok, default is local rgb.txt and fonts
# Modify FONT_DIR_LIST in fontlist.c to add font search directories
#
# For release, full paths are required matching target system locations
#	X11_RGBTXT=/usr/share/X11/rgb.txt
#
#X11_RGBTXT=/usr/share/X11/rgb.txt

# Xrm routines can be compiled in with INCLUDE_XRM=Y
# set to Y to include (unmodifed X11) Xrm routines
#
#INCLUDE_XRM=Y

# NXLIB library name and installation directory for "make install"
# Installation is only required when wanting to completely
# create a libX11.so that allows unmodified changes to X11
# application makefiles. In the normal case, and when
# cross-compiling on a system with X11, no installation
# is possible (as libX11.so would be replaced), and
# the link command is changed from '-lX11' to '-lNX11 -lnano-X'
LIBNAME=NX11
PLIBNAME=PX11
ELIBNAME=NXext

SOLIBREV=0.48
MAJVERSION=0
SOEXTRALIBS = -L$(MWIN_LIB) -lnano-X

ifeq ($(DEBUG), Y)
	CFLAGS += -DDEBUG=1 $(OPTFLAGS)
	CFLAGS += -Wall
else
	CFLAGS += -DDEBUG=0 -O3
endif
CFLAGS += -I$(MWIN_INCLUDE) -I$(X11_INCLUDE)
CFLAGS += -DX11_FONT_DIR1=\"$(X11_FONT_DIR1)\"
CFLAGS += -DX11_FONT_DIR2=\"$(X11_FONT_DIR2)\"
CFLAGS += -DX11_FONT_DIR3=\"$(X11_FONT_DIR3)\"
CFLAGS += -DX11_RGBTXT=\"$(X11_RGBTXT)\"
xCFLAGS += -O2 -fno-strength-reduce
ifeq ($(BIGENDIAN), Y)
CFLAGS += -DCPU_BIG_ENDIAN=1
endif

# compiler flags
#avoid to use default CC which is cc
CC = $(TOOLSPREFIX)gcc
LN = ln -s
MV = mv
CP = cp -p
RM = rm -f
AR = ar

ifeq ($(VERBOSE), N)
.SILENT:
endif

OBJS = DestWind.o MapWindow.o NextEvent.o OpenDis.o ClDisplay.o\
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
	Resource.o GetGCVals.o Threads.o IM.o SetWMProto.o FontInfo.o \
	GetPntMap.o GetWMProps.o ChSaveSet.o TextToStr.o QueryBest.o

ifeq ($(INCLUDE_XRM), Y)
OBJS += Quarks.o Xrm.o ParseCmd.o
xOBJS += xrm/Xrm.o xrm/ParseCmd.o xrm/Misc.o xrm/Quarks.o xrm/lcWrap.o \
    xrm/lcInit.o xrm/lcGenConv.o xrm/SetLocale.o xrm/lcConv.o xrm/lcUTF8.o \
    xrm/lcDefConv.o xrm/lcPubWrap.o xrm/lcDynamic.o xrm/lcCharSet.o \
    xrm/lcDB.o xrm/lcGeneric.o xrm/lcUtil.o xrm/lcCT.o xrm/lcFile.o \
    xrm/lcPublic.o xrm/lcRM.o xrm/imInt.o
CFLAGS += -I.
endif

# comment out the next line to not compile in X extension dummy stubs into NXLIB
#OBJS += NXext.o

#ifeq ($(NX11),Y)
ifeq ($(SHAREDLIBS), Y)
CFLAGS += -fPIC
LIBS += lib$(LIBNAME).so.$(SOLIBREV)
LIBS += lib$(PLIBNAME).so.$(SOLIBREV)
else
LIBS = lib$(LIBNAME).a
LIBS += lib$(PLIBNAME).a
endif
#endif

#ifeq ($(NXEXT),Y)
#ifeq ($(SHAREDLIBS), Y)
##LIBS += lib$(ELIBNAME).so.$(SOLIBREV)
#else
##LIBS += lib$(ELIBNAME).a
#endif
#endif

all: $(LIBS)

ifneq ($(ARCH),MACOSX)
#avoid testprogs to be compiled before libs are done if parallel execution
	$(MAKE) testprogs
endif

# static NXLIB library
lib$(LIBNAME).a: keysymstr.h $(OBJS)
	echo "Linking lib$(LIBNAME).a"
	$(AR) r lib$(LIBNAME).a $(OBJS)
	$(CP) lib$(LIBNAME).a $(MWIN_LIB)
	
	
# shared NX11 library
#$@~ is the target ($@) with a ~ added as a suffix to indicate a temp file
lib$(LIBNAME).so.$(SOLIBREV): $(OBJS)
	$(RM) $@~
	@SONAME=`echo $@ | sed 's/\.[^\.]*$$//'`; set -x; \
	$(CC) -o ./$@~ -shared -Wl,-soname,$$SONAME $(OBJS) $(SOEXTRALIBS) -lc;
#	$(RM) $$SONAME; $(LN) $@ $$SONAME;
#	$(RM) $@
	$(MV) $@~ $@
#	$(RM) lib$(LIBNAME).so; $(LN) $@ lib$(LIBNAME).so
#	$(CP) $@ $(MW_DIR_SRC)/lib/lib$(LIBNAME).so
	$(MV) $@ lib$(LIBNAME).so
	$(CP) lib$(LIBNAME).so $(MWIN_LIB)
	-$(RM) lib$(LIBNAME).so.$(MAJVERSION)
	-ln -s lib$(LIBNAME).so lib$(LIBNAME).so.$(MAJVERSION)
	-$(CP)d lib$(LIBNAME).so.$(MAJVERSION) $(MWIN_LIB)
	-$(RM) lib$(LIBNAME).so.$(MAJVERSION)
	
ODIR = .
ARSCRIPT = arscript

# static PXLIB library
# FIXME we don't need more rules about how to link libraries
# use the automatic make mechanism already in place for linking. Also, "ar" scripts cannot be used.
# Makefile.rules is the place to put MINGW specific link instructions, not here.
lib$(PLIBNAME).a: $(OBJS)
	echo "Linking lib$(PLIBNAME).a"
	$(AR) r lib$(PLIBNAME).a $(OBJS) ../obj/engine/*.o ../obj/fonts/*.o ../obj/drivers/*.o ../obj/nanox/*.o
	$(CP) lib$(PLIBNAME).a $(MWIN_LIB)

# shared PX11 library
# FIXME we don't need more rules about how to link libraries
# use the automatic make mechanism already in place for linking.
lib$(PLIBNAME).so.$(SOLIBREV): $(OBJS)
	$(RM) $@~
	@SONAME=`echo $@ | sed 's/\.[^\.]*$$//'`; set -x; \
	$(CC) -o ./$@~ -shared -Wl,-soname,$$SONAME $(OBJS) -Wl,-whole-archive ../lib/libnano-X.a -Wl,-no-whole-archive -lc;
	$(MV) $@~ $@
	$(MV) $@ lib$(PLIBNAME).so
	$(CP) lib$(PLIBNAME).so $(MWIN_LIB)
	-$(RM) lib$(PLIBNAME).so.$(MAJVERSION)
	-ln -s lib$(PLIBNAME).so lib$(PLIBNAME).so.$(MAJVERSION)
	-$(CP) -d lib$(PLIBNAME).so.$(MAJVERSION) $(MWIN_LIB)
	-$(RM) lib$(PLIBNAME).so.$(MAJVERSION)

#
# FIXME we don't need more rules about how to compile .c files, this should
# use the automatic make mechanism already in place for CC. Also, "ar" cannot be assumed.
lib$(ELIBNAME).a:
	echo "Building NXext library"
	$(CC) -s -c $(ELIBNAME).c
	ar -cvq lib$(ELIBNAME).a $(ELIBNAME).o
	-$(RM) $(ELIBNAME).o
	$(MV) lib$(ELIBNAME).a $(MWIN_LIB)

lib$(ELIBNAME).so.$(SOLIBREV):
	$(CC) -s -fPIC -c $(ELIBNAME).c
	$(CC) -shared -Wl,-soname,lib$(ELIBNAME).so -o lib$(ELIBNAME).so $(ELIBNAME).o
	ln -sf lib$(ELIBNAME).so lib$(ELIBNAME).so.$(MAJVERSION)
	-$(RM) $(ELIBNAME).o
	$(CP) lib$(ELIBNAME).so $(MWIN_LIB)
	$(CP) -d lib$(ELIBNAME).so.$(MAJVERSION) $(MWIN_LIB)
	-$(RM) lib$(ELIBNAME).so.$(MAJVERSION)

testprogs:
ifneq ($(ARCH), RTEMS)
ifneq ($(ARCH), ANDROID) 
	echo "Compiling programs in test"
	-$(MAKE) -C test
	echo "Compiling programs in ptest"
	-$(MAKE) -C ptest
#	-$(RM) lib$(LIBNAME).a
#	-$(RM) lib$(PLIBNAME).a
endif
endif

install: $(LIBS)
# No mv to INSTALL_DIR since NXLIB moved into Microwindows source tree
# All libs are copied into ../lib instead
#
#	$(RM) $(INSTALL_DIR)/lib$(LIBNAME).so; \
#	$(CP) lib$(LIBNAME).so $(INSTALL_DIR)
#
#	@MAJREV=`expr $(SOLIBREV) : '\(.*\)\.'`; set -x; \
#	$(RM) $(INSTALL_DIR)/lib$(LIBNAME).so.$$MAJREV; \
#	$(MV) lib$(LIBNAME).so.$$MAJREV $(INSTALL_DIR)
#	$(RM) $(INSTALL_DIR)/lib$(LIBNAME).so.$(SOLIBREV); \
#	$(MV) lib$(LIBNAME).so.$(SOLIBREV) $(INSTALL_DIR)
#	$(MV) lib$(LIBNAME).a $(INSTALL_DIR)

clean: cleanlibs
	-$(RM) *.o *~
	-$(RM) *.a	
	-$(RM) *.so
	-$(MAKE) -C test clean
	-$(MAKE) -C ptest clean

cleanlibs:
	-$(RM) lib$(LIBNAME).a
	-$(RM) lib$(LIBNAME).so
	-$(RM) lib$(LIBNAME).so.*
	@MAJREV=`expr $(SOLIBREV) : '\(.*\)\.'`; \
	set -x; $(RM) lib$(LIBNAME).so.$$MAJREV
	-$(RM) lib$(LIBNAME).so.$(SOLIBREV)
	-$(RM) lib$(PLIBNAME).so.*

distclean: clean
	rm -f keysymstr.h

keysymstr.h: 
	perl ./keymap.pl $(X11_INCLUDE)/X11 > ./keysymstr.h

tt: tt.o $(LIBS)
	cc -o tt tt.c -L. -lnx11 $(SOEXTRALIBS)

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	@echo "Compiling nxlib/$(patsubst $(OBJS)/%,%,$<) ..."
	#echo "$(CC) $(CFLAGS) -o $@ -c $<"
	$(CC) $(CFLAGS) -o $@ -c $<
