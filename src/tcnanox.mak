#
# Borland C++ IDE generated makefile
# Generated 2000/4/20 at PM 05:57:12 
#
.AUTODEPEND


#
# Borland C++ tools
#
IMPLIB  = Implib
BCCDOS  = Bcc +BccDos.cfg 
TLINK   = TLink
TLIB    = TLib
TASM    = Tasm
#
# IDE macros
#


#
# Options
#
BC_Path = C:\BC5
BC_Lib_Path = $(BC_Path)\LIB
BGI_Path = $(BC_Path)\BGI
MW_Path = D:\microwin
IDE_LinkFLAGSDOS =  -L$(BC_Lib_Path)
IDE_BFLAGS = 
CompLocalOptsAtDOS_nanoxdlib =  -ml -f-
LinkerLocalOptsAtDOS_nanoxdlib =  -c -Tde
ResLocalOptsAtDOS_nanoxdlib = 
BLocalOptsAtDOS_nanoxdlib = 
CompOptsAt_nanoxdlib = $(CompLocalOptsAtDOS_nanoxdlib)
CompInheritOptsAt_nanoxdlib = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE;$(MW_Path)\SRC\DRIVERS;$(MW_Path)\SRC\ENGINE;$(MW_Path)\SRC\NANOX -DMSDOS=1;DOS_TURBOC=1;NONETWORK=1
LinkerInheritOptsAt_nanoxdlib = -x
LinkerOptsAt_nanoxdlib = $(LinkerLocalOptsAtDOS_nanoxdlib)
ResOptsAt_nanoxdlib = $(ResLocalOptsAtDOS_nanoxdlib)
BOptsAt_nanoxdlib = $(BLocalOptsAtDOS_nanoxdlib)
CompLocalOptsAtDOS_demodexe =  -ml -f-
LinkerLocalOptsAtDOS_demodexe =  -c -Tde
ResLocalOptsAtDOS_demodexe = 
BLocalOptsAtDOS_demodexe = 
CompOptsAt_demodexe = $(CompLocalOptsAtDOS_demodexe)
CompInheritOptsAt_demodexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_demodexe = -x
LinkerOptsAt_demodexe = $(LinkerLocalOptsAtDOS_demodexe)
ResOptsAt_demodexe = $(ResLocalOptsAtDOS_demodexe)
BOptsAt_demodexe = $(BLocalOptsAtDOS_demodexe)
CompLocalOptsAtDOS_demo2dexe =  -ml -f-
LinkerLocalOptsAtDOS_demo2dexe =  -c -Tde
ResLocalOptsAtDOS_demo2dexe = 
BLocalOptsAtDOS_demo2dexe = 
CompOptsAt_demo2dexe = $(CompLocalOptsAtDOS_demo2dexe)
CompInheritOptsAt_demo2dexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_demo2dexe = -x
LinkerOptsAt_demo2dexe = $(LinkerLocalOptsAtDOS_demo2dexe)
ResOptsAt_demo2dexe = $(ResLocalOptsAtDOS_demo2dexe)
BOptsAt_demo2dexe = $(BLocalOptsAtDOS_demo2dexe)
CompLocalOptsAtDOS_demo3dexe =  -ml -f-
LinkerLocalOptsAtDOS_demo3dexe =  -c -Tde
ResLocalOptsAtDOS_demo3dexe = 
BLocalOptsAtDOS_demo3dexe = 
CompOptsAt_demo3dexe = $(CompLocalOptsAtDOS_demo3dexe)
CompInheritOptsAt_demo3dexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_demo3dexe = -x
LinkerOptsAt_demo3dexe = $(LinkerLocalOptsAtDOS_demo3dexe)
ResOptsAt_demo3dexe = $(ResLocalOptsAtDOS_demo3dexe)
BOptsAt_demo3dexe = $(BLocalOptsAtDOS_demo3dexe)
CompLocalOptsAtDOS_demo4dexe =  -ml -f-
LinkerLocalOptsAtDOS_demo4dexe =  -c -Tde
ResLocalOptsAtDOS_demo4dexe = 
BLocalOptsAtDOS_demo4dexe = 
CompOptsAt_demo4dexe = $(CompLocalOptsAtDOS_demo4dexe)
CompInheritOptsAt_demo4dexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_demo4dexe = -x
LinkerOptsAt_demo4dexe = $(LinkerLocalOptsAtDOS_demo4dexe)
ResOptsAt_demo4dexe = $(ResLocalOptsAtDOS_demo4dexe)
BOptsAt_demo4dexe = $(BLocalOptsAtDOS_demo4dexe)
CompLocalOptsAtDOS_demo5dexe =  -ml -f-
LinkerLocalOptsAtDOS_demo5dexe =  -c -Tde
ResLocalOptsAtDOS_demo5dexe = 
BLocalOptsAtDOS_demo5dexe = 
CompOptsAt_demo5dexe = $(CompLocalOptsAtDOS_demo5dexe)
CompInheritOptsAt_demo5dexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_demo5dexe = -x
LinkerOptsAt_demo5dexe = $(LinkerLocalOptsAtDOS_demo5dexe)
ResOptsAt_demo5dexe = $(ResLocalOptsAtDOS_demo5dexe)
BOptsAt_demo5dexe = $(BLocalOptsAtDOS_demo5dexe)
CompLocalOptsAtDOS_demo6dexe =  -ml -f-
LinkerLocalOptsAtDOS_demo6dexe =  -c -Tde
ResLocalOptsAtDOS_demo6dexe = 
BLocalOptsAtDOS_demo6dexe = 
CompOptsAt_demo6dexe = $(CompLocalOptsAtDOS_demo6dexe)
CompInheritOptsAt_demo6dexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_demo6dexe = -x
LinkerOptsAt_demo6dexe = $(LinkerLocalOptsAtDOS_demo6dexe)
ResOptsAt_demo6dexe = $(ResLocalOptsAtDOS_demo6dexe)
BOptsAt_demo6dexe = $(BLocalOptsAtDOS_demo6dexe)
CompLocalOptsAtDOS_ftdemodexe =  -ml -f-
LinkerLocalOptsAtDOS_ftdemodexe =  -c -Tde
ResLocalOptsAtDOS_ftdemodexe = 
BLocalOptsAtDOS_ftdemodexe = 
CompOptsAt_ftdemodexe = $(CompLocalOptsAtDOS_ftdemodexe)
CompInheritOptsAt_ftdemodexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_ftdemodexe = -x
LinkerOptsAt_ftdemodexe = $(LinkerLocalOptsAtDOS_ftdemodexe)
ResOptsAt_ftdemodexe = $(ResLocalOptsAtDOS_ftdemodexe)
BOptsAt_ftdemodexe = $(BLocalOptsAtDOS_ftdemodexe)
CompLocalOptsAtDOS_infodexe =  -ml -f-
LinkerLocalOptsAtDOS_infodexe =  -c -Tde
ResLocalOptsAtDOS_infodexe = 
BLocalOptsAtDOS_infodexe = 
CompOptsAt_infodexe = $(CompLocalOptsAtDOS_infodexe)
CompInheritOptsAt_infodexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_infodexe = -x
LinkerOptsAt_infodexe = $(LinkerLocalOptsAtDOS_infodexe)
ResOptsAt_infodexe = $(ResLocalOptsAtDOS_infodexe)
BOptsAt_infodexe = $(BLocalOptsAtDOS_infodexe)
CompLocalOptsAtDOS_landminedexe =  -ml -f-
LinkerLocalOptsAtDOS_landminedexe =  -c -Tde
ResLocalOptsAtDOS_landminedexe = 
BLocalOptsAtDOS_landminedexe = 
CompOptsAt_landminedexe = $(CompLocalOptsAtDOS_landminedexe)
CompInheritOptsAt_landminedexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_landminedexe = -x
LinkerOptsAt_landminedexe = $(LinkerLocalOptsAtDOS_landminedexe)
ResOptsAt_landminedexe = $(ResLocalOptsAtDOS_landminedexe)
BOptsAt_landminedexe = $(BLocalOptsAtDOS_landminedexe)
CompLocalOptsAtDOS_movedexe =  -ml -f-
LinkerLocalOptsAtDOS_movedexe =  -c -Tde
ResLocalOptsAtDOS_movedexe = 
BLocalOptsAtDOS_movedexe = 
CompOptsAt_movedexe = $(CompLocalOptsAtDOS_movedexe)
CompInheritOptsAt_movedexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_movedexe = -x
LinkerOptsAt_movedexe = $(LinkerLocalOptsAtDOS_movedexe)
ResOptsAt_movedexe = $(ResLocalOptsAtDOS_movedexe)
BOptsAt_movedexe = $(BLocalOptsAtDOS_movedexe)
CompLocalOptsAtDOS_sliderdexe =  -ml -f-
LinkerLocalOptsAtDOS_sliderdexe =  -c -Tde
ResLocalOptsAtDOS_sliderdexe = 
BLocalOptsAtDOS_sliderdexe = 
CompOptsAt_sliderdexe = $(CompLocalOptsAtDOS_sliderdexe)
CompInheritOptsAt_sliderdexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_sliderdexe = -x
LinkerOptsAt_sliderdexe = $(LinkerLocalOptsAtDOS_sliderdexe)
ResOptsAt_sliderdexe = $(ResLocalOptsAtDOS_sliderdexe)
BOptsAt_sliderdexe = $(BLocalOptsAtDOS_sliderdexe)
CompLocalOptsAtDOS_waitdexe =  -ml -f-
LinkerLocalOptsAtDOS_waitdexe =  -c -Tde
ResLocalOptsAtDOS_waitdexe = 
BLocalOptsAtDOS_waitdexe = 
CompOptsAt_waitdexe = $(CompLocalOptsAtDOS_waitdexe)
CompInheritOptsAt_waitdexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_waitdexe = -x
LinkerOptsAt_waitdexe = $(LinkerLocalOptsAtDOS_waitdexe)
ResOptsAt_waitdexe = $(ResLocalOptsAtDOS_waitdexe)
BOptsAt_waitdexe = $(BLocalOptsAtDOS_waitdexe)
CompLocalOptsAtDOS_worlddexe =  -ml -f-
LinkerLocalOptsAtDOS_worlddexe =  -c -Tde
ResLocalOptsAtDOS_worlddexe = 
BLocalOptsAtDOS_worlddexe = 
CompOptsAt_worlddexe = $(CompLocalOptsAtDOS_worlddexe)
CompInheritOptsAt_worlddexe = -I$(BC_Path)\INCLUDE;$(MW_Path)\SRC\INCLUDE 
LinkerInheritOptsAt_worlddexe = -x
LinkerOptsAt_worlddexe = $(LinkerLocalOptsAtDOS_worlddexe)
ResOptsAt_worlddexe = $(ResLocalOptsAtDOS_worlddexe)
BOptsAt_worlddexe = $(BLocalOptsAtDOS_worlddexe)

#
# Dependency List
#
Dep_nanox = \
   nanox.lib\
   demo.exe\
   demo2.exe\
   demo3.exe\
   demo4.exe\
   demo5.exe\
   demo6.exe\
   ftdemo.exe\
   info.exe\
   landmine.exe\
   move.exe\
   slider.exe\
   wait.exe\
   world.exe

nanox : BccDos.cfg $(Dep_nanox)
  echo MakeNode

Dep_nanoxdlib = \
   egavga.obj\
   x6x13.obj\
   rom8x16.obj\
   winfreesansserif11x13.obj\
   winfreesystem14x16.obj\
   genfont.obj\
   mou_dos.obj\
   kbd_tc.obj\
   scr_tc.obj\
   devclip.obj\
   devpal4.obj\
   devpal2.obj\
   devpal1.obj\
   devmouse.obj\
   devkbd.obj\
   devfont.obj\
   devdraw.obj\
   devrgn.obj\
   devrgn2.obj\
   devarc.obj\
   error.obj\
   srvclip.obj\
   srvutil.obj\
   srvmain.obj\
   srvfunc.obj\
   srvevent.obj\
   stubs.obj

nanox.lib : $(Dep_nanoxdlib)
  $(TLIB) $< $(IDE_BFLAGS) $(BOptsAt_nanoxdlib) @&&|
 -+egavga.obj &
-+x6x13.obj &
-+rom8x16.obj &
-+winfreesansserif11x13.obj &
-+winfreesystem14x16.obj &
-+genfont.obj &
-+mou_dos.obj &
-+kbd_tc.obj &
-+scr_tc.obj &
-+devclip.obj &
-+devpal4.obj &
-+devpal2.obj &
-+devpal1.obj &
-+devmouse.obj &
-+devkbd.obj &
-+devfont.obj &
-+devdraw.obj &
-+devrgn.obj &
-+devrgn2.obj &
-+devarc.obj &
-+error.obj &
-+srvclip.obj &
-+srvutil.obj &
-+srvmain.obj &
-+srvfunc.obj &
-+srvevent.obj &
-+stubs.obj
|

x6x13.obj :  fonts\x6x13.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ fonts\x6x13.c
|

rom8x16.obj :  fonts\rom8x16.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ fonts\rom8x16.c
|

winfreesansserif11x13.obj :  fonts\winfreesansserif11x13.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ fonts\winfreesansserif11x13.c
|

winfreesystem14x16.obj :  fonts\winfreesystem14x16.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ fonts\winfreesystem14x16.c
|

genfont.obj :  drivers\genfont.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ drivers\genfont.c
|

mou_dos.obj :  drivers\mou_dos.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ drivers\mou_dos.c
|

kbd_tc.obj :  drivers\kbd_tc.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ drivers\kbd_tc.c
|

scr_tc.obj :  drivers\scr_tc.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ drivers\scr_tc.c
|

devclip.obj :  engine\devclip.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devclip.c
|

devpal4.obj :  engine\devpal4.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devpal4.c
|

devpal2.obj :  engine\devpal2.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devpal2.c
|

devpal1.obj :  engine\devpal1.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devpal1.c
|

devmouse.obj :  engine\devmouse.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devmouse.c
|

devkbd.obj :  engine\devkbd.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devkbd.c
|

devfont.obj :  engine\devfont.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devfont.c
|

devdraw.obj :  engine\devdraw.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devdraw.c
|

devrgn.obj :  engine\devrgn.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devrgn.c
|

devrgn2.obj :  engine\devrgn2.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devrgn2.c
|

devarc.obj :  engine\devarc.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\devarc.c
|

error.obj :  engine\error.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ engine\error.c
|

srvclip.obj :  nanox\srvclip.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ nanox\srvclip.c
|

srvutil.obj :  nanox\srvutil.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ nanox\srvutil.c
|

srvmain.obj :  nanox\srvmain.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ nanox\srvmain.c
|

srvfunc.obj :  nanox\srvfunc.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ nanox\srvfunc.c
|

srvevent.obj :  nanox\srvevent.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ nanox\srvevent.c
|

stubs.obj :  nanox\stubs.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_nanoxdlib) $(CompInheritOptsAt_nanoxdlib) -o$@ nanox\stubs.c
|

Dep_demodexe = \
   nanox.lib\
   demo.obj

demo.exe : $(Dep_demodexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_demodexe) $(LinkerInheritOptsAt_demodexe) +
$(BC_Lib_Path)\c0l.obj+
demo.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
demo.obj :  demos\nanox\demo.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demodexe) $(CompInheritOptsAt_demodexe) -o$@ demos\nanox\demo.c
|

Dep_demo2dexe = \
   nanox.lib\
   demo2.obj

demo2.exe : $(Dep_demo2dexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_demo2dexe) $(LinkerInheritOptsAt_demo2dexe) +
$(BC_Lib_Path)\c0l.obj+
demo2.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
demo2.obj :  demos\nanox\demo2.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demo2dexe) $(CompInheritOptsAt_demo2dexe) -o$@ demos\nanox\demo2.c
|

Dep_demo3dexe = \
   nanox.lib\
   demo3.obj

demo3.exe : $(Dep_demo3dexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_demo3dexe) $(LinkerInheritOptsAt_demo3dexe) +
$(BC_Lib_Path)\c0l.obj+
demo3.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
demo3.obj :  demos\nanox\demo3.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demo3dexe) $(CompInheritOptsAt_demo3dexe) -o$@ demos\nanox\demo3.c
|

Dep_demo4dexe = \
   nanox.lib\
   demo4.obj

demo4.exe : $(Dep_demo4dexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_demo4dexe) $(LinkerInheritOptsAt_demo4dexe) +
$(BC_Lib_Path)\c0l.obj+
demo4.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
demo4.obj :  demos\nanox\demo4.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demo4dexe) $(CompInheritOptsAt_demo4dexe) -o$@ demos\nanox\demo4.c
|

Dep_demo5dexe = \
   nanox.lib\
   demo5.obj

demo5.exe : $(Dep_demo5dexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_demo5dexe) $(LinkerInheritOptsAt_demo5dexe) +
$(BC_Lib_Path)\c0l.obj+
demo5.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
demo5.obj :  demos\nanox\demo5.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demo5dexe) $(CompInheritOptsAt_demo5dexe) -o$@ demos\nanox\demo5.c
|

Dep_demo6dexe = \
   nanox.lib\
   demo6.obj

demo6.exe : $(Dep_demo6dexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_demo6dexe) $(LinkerInheritOptsAt_demo6dexe) +
$(BC_Lib_Path)\c0l.obj+
demo6.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
demo6.obj :  demos\nanox\demo6.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demo6dexe) $(CompInheritOptsAt_demo6dexe) -o$@ demos\nanox\demo6.c
|

Dep_ftdemodexe = \
   nanox.lib\
   ftdemo.obj

ftdemo.exe : $(Dep_ftdemodexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_ftdemodexe) $(LinkerInheritOptsAt_ftdemodexe) +
$(BC_Lib_Path)\c0l.obj+
ftdemo.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
ftdemo.obj :  demos\nanox\ftdemo.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_ftdemodexe) $(CompInheritOptsAt_ftdemodexe) -o$@ demos\nanox\ftdemo.c
|

Dep_infodexe = \
   nanox.lib\
   info.obj

info.exe : $(Dep_infodexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_infodexe) $(LinkerInheritOptsAt_infodexe) +
$(BC_Lib_Path)\c0l.obj+
info.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
info.obj :  demos\nanox\info.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_infodexe) $(CompInheritOptsAt_infodexe) -o$@ demos\nanox\info.c
|

Dep_landminedexe = \
   nanox.lib\
   landmine.obj

landmine.exe : $(Dep_landminedexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_landminedexe) $(LinkerInheritOptsAt_landminedexe) +
$(BC_Lib_Path)\c0l.obj+
landmine.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
landmine.obj :  demos\nanox\landmine.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_landminedexe) $(CompInheritOptsAt_landminedexe) -o$@ demos\nanox\landmine.c
|

Dep_movedexe = \
   nanox.lib\
   move.obj

move.exe : $(Dep_movedexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_movedexe) $(LinkerInheritOptsAt_movedexe) +
$(BC_Lib_Path)\c0l.obj+
move.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
move.obj :  demos\nanox\move.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_movedexe) $(CompInheritOptsAt_movedexe) -o$@ demos\nanox\move.c
|

Dep_sliderdexe = \
   nanox.lib\
   slider.obj

slider.exe : $(Dep_sliderdexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_sliderdexe) $(LinkerInheritOptsAt_sliderdexe) +
$(BC_Lib_Path)\c0l.obj+
slider.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
slider.obj :  demos\nanox\slider.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_sliderdexe) $(CompInheritOptsAt_sliderdexe) -o$@ demos\nanox\slider.c
|

Dep_waitdexe = \
   nanox.lib\
   wait.obj

wait.exe : $(Dep_waitdexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_waitdexe) $(LinkerInheritOptsAt_waitdexe) +
$(BC_Lib_Path)\c0l.obj+
wait.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
wait.obj :  demos\nanox\wait.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_waitdexe) $(CompInheritOptsAt_waitdexe) -o$@ demos\nanox\wait.c
|

Dep_worlddexe = \
   nanox.lib\
   world.obj

world.exe : $(Dep_worlddexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_worlddexe) $(LinkerInheritOptsAt_worlddexe) +
$(BC_Lib_Path)\c0l.obj+
world.obj
$<,$*
nanox.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
world.obj :  demos\nanox\world.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_worlddexe) $(CompInheritOptsAt_worlddexe) -o$@ demos\nanox\world.c
|

# Compiler configuration file
BccDos.cfg : 
   Copy &&|
-W-
-w
-R
-v
-vi
-H
-H=nanox.csm
| $@


clean :
  del *.obj
  del *.lib
  del *.exe
  del *.csm
  del BccDos.cfg

egavga.obj :
  copy $(BGI_Path)\egavga.bgi .
  $(BGI_Path)\bgiobj egavga
  del egavga.bgi

