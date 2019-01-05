#
# Microwindows Makefile for MSDOS using Microsoft C 5.10
#
# Copyright (c) 1999 Greg Haerr <greg@censoft.com>
#
CC=cl -c -Gs -Ols -AM -I. -Ic:\tools\msc5\include
xCFLAGS=-W1 -DUPDATEREGIONS=1 -DERASEMOVE=1
CFLAGS=-W1 -DERASEMOVE=1
LFLAGS=/NOI
LIBDIR=c:\tools\msc5

SERV=mwin\winmain.obj mwin\winuser.obj mwin\wingdi.obj mwin\winexpos.obj\
	mwin\winclip.obj mwin\winevent.obj mwin\windefw.obj mwin\list.obj\
	engine\devdraw.obj engine\devmouse.obj engine\devkbd.obj\
	engine\devclip.obj\
	engine\devpal1.obj engine\devpal2.obj engine\devpal4.obj\
	mwin\winlib\draw3d.obj mwin\winlib\fastfill.obj mwin\winlib\insetr.obj\
	mwin\winlib\ptinsid.obj mwin\winlib\graph3d.obj
VGA_NEW=drivers\vgaplan4.obj drivers\mempl4.obj
VGA_OLD=drivers\asmplan4.obj
VGA_HWINIT=drivers\vgainit.obj
DRIVERS=drivers\scr_bios.obj drivers\romfont.obj drivers\kbd_bios.obj\
	drivers\mou_dos.obj drivers\asmbios.obj $(VGA_NEW)
DEMO=demos\mwin\demo.obj mwin\winctl\button.obj mwin\bmp\cs1.obj
OBJS=$(SERV) $(DRIVERS) $(DEMO)
LIBS=

all: mwindemo

clean:
	erase *.map
	erase mwindemo.exe
	erase drivers\*.obj
	erase demos\mwin\*.obj
	erase engine\*.obj
	erase mwin\*.obj
	erase mwin\bmp\*.obj
	erase mwin\winctl\*.obj
	erase mwin\winlib\*.obj


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

mwindemo: $(OBJS)
	@set LIB=$(LIBDOS)
	link @<<
$(LFLAGS) /NOE+
$(OBJS: =+^
)
mwindemo
mwindemo/map/stack:3000/seg:256/f/packc:50000
$(LIBS) $(LIBDIR)\mlibce
nul
<<

