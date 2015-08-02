##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := nxlib/test/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)
##############################################################################

$(info ) #blank line
$(info Compiling nxlib/test)

INCLUDE=-I..

ifeq ($(SHAREDLIBS), Y)
LIBS =$(LDFLAGS) $(CCNANOXCLIENTLIBS) ../../lib/libNX11.so 
else
LIBS =$(LDFLAGS) $(NANOXCLIENTLIBS) ../../lib/libNX11.a 
endif

all:
	@echo "Compiling nxlib/test/williams.c ..."
	$(CC) $(CFLAGS) -o williams $(INCLUDE) williams.c $(LIBS)
	@echo "Compiling nxlib/test/queryfont.c ..."
	$(CC) $(CFLAGS) -o queryfont $(INCLUDE) queryfont.c $(LIBS)
	@echo "Compiling nxlib/test/loadfont.c ..."
	$(CC) $(CFLAGS) -o loadfont $(INCLUDE) loadfont.c $(LIBS)
	@echo "Compiling nxlib/test/xhello.c ..."
	$(CC) $(CFLAGS) -g -o xhello $(INCLUDE) xhello.c $(LIBS)	
	@echo "Compiling nxlib/test/xmouse.c ..."
	$(CC) $(CFLAGS) -g -o xmouse $(INCLUDE) xmouse.c $(LIBS)	
	@echo "Compiling nxlib/test/xgreen.c ..."
	$(CC) $(CFLAGS) -g -o xgreen $(INCLUDE) xgreen.c $(LIBS)	
	
clean:
	-rm williams
	-rm queryfont
	-rm loadfont
	-rm xhello
	-rm xmouse
	-rm xgreen