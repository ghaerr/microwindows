#
# Nano-X Makefile for MSDOS using Microsoft C 5.10
#
# Copyright (c) 1999 Greg Haerr <greg@censoft.com>
#
CC=cl -c -Gs -Ols -AM -I. -Ic:\tools\msc5\include
CFLAGS=-W1 -DNONETWORK=1
LFLAGS=/NOI
LIBDIR=c:\tools\msc5

SERV=nanox\srvmain.obj nanox\srvfunc.obj nanox\srvutil.obj nanox\srvevent.obj\
	engine\devdraw.obj engine\devmouse.obj engine\devkbd.obj\
	engine\devclip.obj\
	engine\devpal1.obj engine\devpal2.obj engine\devpal4.obj
VGA_NEW=drivers\vgaplan4.obj drivers\mempl4.obj
VGA_OLD=drivers\asmplan4.obj
VGA_HWINIT=drivers\vgainit.obj
DRIVERS=drivers\scr_bios.obj drivers\romfont.obj drivers\kbd_bios.obj\
	drivers\mou_dos.obj drivers\asmbios.obj $(VGA_NEW)
xDEMO=demos\nanox\demo.obj nanox\stubs.obj
DEMO=demos\nanox\landmine.obj nanox\stubs.obj
xDEMO=demos\nanox\world.obj nanox\stubs.obj
xDEMO=test.obj
OBJS=$(SERV) $(DRIVERS) $(DEMO)
LIBS=

all: nanox

clean:
	erase *.map
	erase nanox.exe
	erase nanox\*.obj
	erase engine\*.obj
	erase drivers\*.obj
	erase demos\nanox\*.obj

.c.obj:
	$(CC) $(CFLAGS) -Fo$@ $<

drivers\asmbios.obj: drivers\asmbios.s
	cd drivers
	masm -Dmem_S -D__MEDIUM__ -Mx asmbios.s;;;
	cd ..

drivers\asmplan4.obj: drivers\asmplan4.s
	cd drivers
	masm -Dmem_S -D__MEDIUM__ -Mx asmplan4.s;;;
	cd ..

nanox: $(OBJS)
	@set LIB=$(LIBDOS)
	link @<<
$(LFLAGS) /NOE+
$(OBJS: =+^
)
nanox
nanox/map/stack:16384/seg:256/f/packc:50000
$(LIBS) $(LIBDIR)\mlibce
nul
<<

