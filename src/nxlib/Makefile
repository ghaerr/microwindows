# 9 Dec 2010
#
# NXLIB library makefile - X11 Conversion Library for Nano-X
# Greg Haerr <greg@censoft.com>
#
# Note: if build fails, rebuild keysymstr.h by "make distclean", then "make"
#

#get shared flag, BIGENDIAN
include $(CONFIG)

# set to Y for big endian architecture (should be automatic)
#
#BIGENDIAN=N

#OPTFLAGS may contain flags for gprof and gdb too - set in Makefile.rules
ifndef OPTFLAGS
CFLAGS += -O3
else
CFLAGS += $(OPTFLAGS)
endif

ifneq ($(DEBUG), Y)
	CFLAGS += -DDEBUG=0
else
	CFLAGS += -DDEBUG=1
endif
# Compile-time nano-X include and lib directories. (relative paths ok)
#
ifneq ($(DOS_DJGPP), Y)
MWIN_INCLUDE=../include
MWIN_LIB=../lib
else
MWIN_INCLUDE=/djgpp/include/mwin -I/myprogs/nxlib
MWIN_LIB=../lib
endif

# Compile-time X11 include directory
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
X11_INCLUDE=.
#X11_INCLUDE=/usr/include/X11

# Run-time font directory and rgb.txt file location:
# For testing, relative paths are ok, default is local rgb.txt and fonts
# Modify FONT_DIR_LIST in fontlist.c to add font search directories
#
# For release, full paths are required matching target system locations
#	X11_RGBTXT=/usr/share/X11/rgb.txt
#
ifeq ($(DOS_DJGPP), Y)
X11_RGBTXT=fonts/rgb.txt
else
X11_RGBTXT=/usr/share/X11/rgb.txt
endif

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
xINSTALL_DIR=.
ifneq ($(DOS_DJGPP), Y)
INSTALL_DIR=/usr/local/lib
else
INSTALL_DIR=/djgpp/lib
endif

# set to Y to make shared libNX11.so library, shared lib dependencies e.g. shared nano-X
ifeq ($(SHAREDLIBS), Y)
SHAREDLIB=Y
else
# cannot make shared libNX11.so library if static nano-X lib
SHAREDLIB=N
endif

SOLIBREV=0.47
MAJVERSION=0
SOEXTRALIBS = -L$(MWIN_LIB) -lnano-X

# set to Y to include (unmodifed X11) Xrm routines
ifeq ($(DOS_DJGPP), Y)
INCLUDE_XRM=N
else
 ifeq ($(MINGW), Y)
  INCLUDE_XRM=N
 else #e.g Linux
  INCLUDE_XRM=Y
 endif
endif

# compiler flags
#avoid to use default CC which is cc
CC = $(TOOLSPREFIX)gcc
LN = ln -s
MV = mv
CP = cp -p
RM = rm -f

ifneq ($(DEBUG), Y)
	CFLAGS += -s -Wno-deprecated-declarations
else
	CFLAGS += -Wall
endif

CFLAGS += -I$(MWIN_INCLUDE) -I$(X11_INCLUDE)
CFLAGS += -DX11_FONT_DIR1=\"$(X11_FONT_DIR1)\"
CFLAGS += -DX11_FONT_DIR2=\"$(X11_FONT_DIR2)\"
CFLAGS += -DX11_FONT_DIR3=\"$(X11_FONT_DIR3)\"
CFLAGS += -DX11_RGBTXT=\"$(X11_RGBTXT)\"
xCFLAGS += -O2 -fno-strength-reduce

#$(info    CFLAGS is $(CFLAGS))

# If you dont want to see every executed command ...
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

EOBJS = NXext.o

LIBS = lib$(LIBNAME).a

ifeq ($(BIGENDIAN), Y)
CFLAGS += -DCPU_BIG_ENDIAN=1
endif

ifeq ($(SHAREDLIB), Y)
CFLAGS += -fPIC
LIBS += lib$(LIBNAME).so.$(SOLIBREV)
endif

LIBS += lib$(PLIBNAME).a
ifeq ($(SHAREDLIB), Y)
LIBS += lib$(PLIBNAME).so.$(SOLIBREV)
endif

ODIR = .
ARSCRIPT = arscript

ifeq ($(WITH_NX11),N)
$(info    )
$(info WITH_NX11 flag set to N - will not compile NX11!)
$(info    )
all:
else
all: $(LIBS) testprogs nxext
endif

# static NXLIB library
lib$(LIBNAME).a: keysymstr.h $(OBJS)
	echo "Linking lib$(LIBNAME).a"
	ar r lib$(LIBNAME).a ../lib/libnano-X.a $(OBJS)
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
	
# static PXLIB library
lib$(PLIBNAME).a: $(OBJS)
	echo "Linking lib$(PLIBNAME).a"
#	ar crsT $@ ../lib/libNX11.a ../lib/libnano-X.a
#create an AR script and execute that with ar -M to combine libNX11.a and libnano-X.a
	$(SILENT)echo "CREATE $@" > $(ODIR)/$(ARSCRIPT)
#	$(SILENT)echo "ADDLIB ../lib/libnano-X.a" >> $(ODIR)/$(ARSCRIPT)	
#	$(SILENT)echo "ADDLIB ../lib/libNX11.a" >> $(ODIR)/$(ARSCRIPT)		
ifeq ($(MINGW), Y)
	$(SILENT)echo "ADDLIB ../lib/libnano-X.a" >> $(ODIR)/$(ARSCRIPT)	
	$(SILENT)echo "ADDLIB ../lib/lib$(LIBNAME).a" >> $(ODIR)/$(ARSCRIPT)		
else    	
	$(SILENT)echo "ADDLIB $(MW_DIR_SRC)/lib/libnano-X.a" >> $(ODIR)/$(ARSCRIPT)	
	$(SILENT)echo "ADDLIB $(MW_DIR_SRC)/lib/lib$(LIBNAME).a" >> $(ODIR)/$(ARSCRIPT)		
endif	
	$(SILENT)echo "SAVE" >> $(ODIR)/$(ARSCRIPT)
	$(SILENT)echo "END" >> $(ODIR)/$(ARSCRIPT)
	-$(SILENT)$(AR) -M < $(ODIR)/$(ARSCRIPT)
	-$(CP) $@ $(MWIN_LIB)
	-$(RM) $(ODIR)/$(ARSCRIPT)

# shared PX11 library
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

ifeq ($(ARCH), RTEMS)
testprogs:
	echo "Test programs compile skipped for RTEMS"
else ifeq ($(ARCH), ANDROID) 
testprogs:
	echo "Test programs compile skipped for ANDROID"
else
testprogs:
	echo "Compiling programs in test"
	-$(MAKE) -C test
	echo "Compiling programs in ptest"
	-$(MAKE) -C ptest
	-$(RM) lib$(LIBNAME).a
	-$(RM) lib$(PLIBNAME).a
endif

#LIBS += lib$(ELIBNAME).a
ifeq ($(SHAREDLIB), Y)
#LIBS += lib$(ELIBNAME).so.$(SOLIBREV)
endif


.PHONY: nxext
nxext:
	echo "Building NXext library"
	$(CC) -s -c $(ELIBNAME).c
	ar -cvq lib$(ELIBNAME).a $(ELIBNAME).o
	-$(RM) $(ELIBNAME).o
	$(MV) lib$(ELIBNAME).a $(MWIN_LIB)
ifeq ($(SHAREDLIB), Y)
	$(CC) -s -fPIC -c $(ELIBNAME).c
	$(CC) -shared -Wl,-soname,lib$(ELIBNAME).so -o lib$(ELIBNAME).so $(ELIBNAME).o
	ln -sf lib$(ELIBNAME).so lib$(ELIBNAME).so.$(MAJVERSION)
	-$(RM) $(ELIBNAME).o
	$(CP) lib$(ELIBNAME).so $(MWIN_LIB)
	$(CP) -d lib$(ELIBNAME).so.$(MAJVERSION) $(MWIN_LIB)
	-$(RM) lib$(ELIBNAME).so.$(MAJVERSION)

endif

install: $(LIBS)
#nxlib.a and nxlib.so are copied into src/lib above, so these are installed
#by Makefile.rules already together with the other libs in /usr/lib
#same for NXext.a
#	$(RM) $(INSTALL_DIR)/lib$(LIBNAME).so; \
#	$(CP) lib$(LIBNAME).so $(INSTALL_DIR)

#	@MAJREV=`expr $(SOLIBREV) : '\(.*\)\.'`; set -x; \
#	$(RM) $(INSTALL_DIR)/lib$(LIBNAME).so.$$MAJREV; \
	$(MV) lib$(LIBNAME).so.$$MAJREV $(INSTALL_DIR)
#	$(RM) $(INSTALL_DIR)/lib$(LIBNAME).so.$(SOLIBREV); \
	$(MV) lib$(LIBNAME).so.$(SOLIBREV) $(INSTALL_DIR)
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
