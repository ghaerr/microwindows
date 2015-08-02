##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := nxlib/ptest/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)
##############################################################################

$(info ) #blank line
$(info Compiling nxlib/ptest)

INCLUDE=-I..

ifeq ($(SHAREDLIBS), Y)
LIBS =$(LDFLAGS) $(CCNANOXCLIENTLIBS) ../../lib/libPX11.so 
else
LIBS =$(LDFLAGS) $(NANOXCLIENTLIBS) ../../lib/libPX11.a 
endif

all:
	@echo "Compiling nxlib/ptest/williams.c ..."
	$(CC) $(CFLAGS) -o williams $(INCLUDE) ../test/williams.c $(LIBS)
	@echo "Compiling nxlib/ptest/queryfont.c ..."
	$(CC) $(CFLAGS) -o queryfont $(INCLUDE) ../test/queryfont.c $(LIBS)
	@echo "Compiling nxlib/ptest/loadfont.c ..."
	$(CC) $(CFLAGS) -o loadfont $(INCLUDE) ../test/loadfont.c $(LIBS)
	@echo "Compiling nxlib/ptest/xhello.c ..."
	$(CC) $(CFLAGS) -g -o xhello $(INCLUDE) ../test/xhello.c $(LIBS)
	@echo "Compiling nxlib/ptest/xmouse.c ..."
	$(CC) $(CFLAGS) -g -o xmouse $(INCLUDE) ../test/xmouse.c $(LIBS)		
	@echo "Compiling nxlib/ptest/xgreen.c ..."
	$(CC) $(CFLAGS) -g -o xgreen $(INCLUDE) ../test/xgreen.c $(LIBS)		
	@echo " "
	
clean:
	-rm williams
	-rm queryfont
	-rm loadfont
	-rm xhello
	-rm xmouse
	-rm xgreen