#
# CONVROM - PC Bios ROM based font extractor
#
# Copyright (c) 1999 Greg Haerr <greg@censoft.com>
#
CC=cl -c -Gs -Ols -AM -Ic:\tools\msc5\include
CFLAGS=-W1
LFLAGS=/NOI
LIBDIR=c:\tools\msc5

OBJS=convrom.obj ..\asmbios.obj
LIBS=

all: convrom

clean:
	erase *.obj
	erase *.map
	erase convrom.exe

.c.obj:
	$(CC) $(CFLAGS) -Fo$@ $<

.s.obj:
	masm -Dmem_S -D__MEDIUM__ -Mx $<.s;;;

..\asmbios.obj: ..\asmbios.s
	masm -Dmem_S -D__MEDIUM__ -Mx ..\asmbios.s;;;

convrom: $(OBJS)
	@set LIB=$(LIBDOS)
	link @<<
$(LFLAGS) /NOE+
$(OBJS: =+^
)
convrom
convrom/stack:16384/seg:256/f/packc:50000
$(LIBS) $(LIBDIR)\mlibce
nul
<<

