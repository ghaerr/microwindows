#
# Borland C++ IDE generated makefile
# Generated 2000/4/21 at PM 02:09:29 
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
CompLocalOptsAtDOS_mwdlib =  -ml -f-
LinkerLocalOptsAtDOS_mwdlib =  -c -Tde
ResLocalOptsAtDOS_mwdlib = 
BLocalOptsAtDOS_mwdlib = 
CompOptsAt_mwdlib = $(CompLocalOptsAtDOS_mwdlib)
CompInheritOptsAt_mwdlib = -I$(MW_Path)\SRC\INCLUDE;$(MW_Path)\SRC\DRIVERS;$(MW_Path)\SRC\ENGINE;$(MW_Path)\SRC\MWIN;$(MW_Path)\SRC\MWIN\WINLIB;$(BC_Path)\INCLUDE -DMSDOS=1;DOS_TURBOC=1
LinkerInheritOptsAt_mwdlib = -x
LinkerOptsAt_mwdlib = $(LinkerLocalOptsAtDOS_mwdlib)
ResOptsAt_mwdlib = $(ResLocalOptsAtDOS_mwdlib)
BOptsAt_mwdlib = $(BLocalOptsAtDOS_mwdlib)
CompLocalOptsAtDOS_demodexe =  -ml -f287
LinkerLocalOptsAtDOS_demodexe =  -c -Tde
ResLocalOptsAtDOS_demodexe = 
BLocalOptsAtDOS_demodexe = 
CompOptsAt_demodexe = $(CompLocalOptsAtDOS_demodexe)
CompInheritOptsAt_demodexe = -I$(MW_Path)\SRC\INCLUDE;$(BC_Path)\INCLUDE -DMSDOS=1;DOS_TURBOC=1
LinkerInheritOptsAt_demodexe = -x
LinkerOptsAt_demodexe = $(LinkerLocalOptsAtDOS_demodexe)
ResOptsAt_demodexe = $(ResLocalOptsAtDOS_demodexe)
BOptsAt_demodexe = $(BLocalOptsAtDOS_demodexe)
CompLocalOptsAtDOS_minedexe =  -ml -f287
LinkerLocalOptsAtDOS_minedexe =  -c -Tde
ResLocalOptsAtDOS_minedexe = 
BLocalOptsAtDOS_minedexe = 
CompOptsAt_minedexe = $(CompLocalOptsAtDOS_minedexe)
CompInheritOptsAt_minedexe = -I$(MW_Path)\SRC\INCLUDE;$(BC_Path)\INCLUDE;$(MW_Path)\SRC\DEMOS\MWIN 
LinkerInheritOptsAt_minedexe = -x
LinkerOptsAt_minedexe = $(LinkerLocalOptsAtDOS_minedexe)
ResOptsAt_minedexe = $(ResLocalOptsAtDOS_minedexe)
BOptsAt_minedexe = $(BLocalOptsAtDOS_minedexe)
CompLocalOptsAtDOS_demosbmwinbminedc = 
LinkerLocalOptsAtDOS_demosbmwinbminedc = 
ResLocalOptsAtDOS_demosbmwinbminedc = 
BLocalOptsAtDOS_demosbmwinbminedc = 
CompOptsAt_demosbmwinbminedc = $(CompOptsAt_minedexe) $(CompLocalOptsAtDOS_demosbmwinbminedc)
CompInheritOptsAt_demosbmwinbminedc = -I$(MW_Path)\SRC\INCLUDE;$(BC_Path)\INCLUDE;$(MW_Path)\SRC\DEMOS\MWIN 
LinkerInheritOptsAt_demosbmwinbminedc = -x
LinkerOptsAt_demosbmwinbminedc = $(LinkerOptsAt_minedexe) $(LinkerLocalOptsAtDOS_demosbmwinbminedc)
ResOptsAt_demosbmwinbminedc = $(ResOptsAt_minedexe) $(ResLocalOptsAtDOS_demosbmwinbminedc)
BOptsAt_demosbmwinbminedc = $(BOptsAt_minedexe) $(BLocalOptsAtDOS_demosbmwinbminedc)
CompLocalOptsAtDOS_malphadexe =  -ml -f-
LinkerLocalOptsAtDOS_malphadexe =  -c -Tde
ResLocalOptsAtDOS_malphadexe = 
BLocalOptsAtDOS_malphadexe = 
CompOptsAt_malphadexe = $(CompLocalOptsAtDOS_malphadexe)
CompInheritOptsAt_malphadexe = -I$(MW_Path)\SRC\INCLUDE;$(BC_Path)\INCLUDE -DMSDOS=1;DOS_TURBOC=1
LinkerInheritOptsAt_malphadexe = -x
LinkerOptsAt_malphadexe = $(LinkerLocalOptsAtDOS_malphadexe)
ResOptsAt_malphadexe = $(ResLocalOptsAtDOS_malphadexe)
BOptsAt_malphadexe = $(BLocalOptsAtDOS_malphadexe)
CompLocalOptsAtDOS_mtestdexe =  -ml -f-
LinkerLocalOptsAtDOS_mtestdexe =  -c -Tde
ResLocalOptsAtDOS_mtestdexe = 
BLocalOptsAtDOS_mtestdexe = 
CompOptsAt_mtestdexe = $(CompLocalOptsAtDOS_mtestdexe)
CompInheritOptsAt_mtestdexe = -I$(MW_Path)\SRC\INCLUDE;$(BC_Path)\INCLUDE -DMSDOS=1;DOS_TURBOC=1
LinkerInheritOptsAt_mtestdexe = -x
LinkerOptsAt_mtestdexe = $(LinkerLocalOptsAtDOS_mtestdexe)
ResOptsAt_mtestdexe = $(ResLocalOptsAtDOS_mtestdexe)
BOptsAt_mtestdexe = $(BLocalOptsAtDOS_mtestdexe)

#
# Dependency List
#
Dep_mw = \
   mw.lib\
   demo.exe\
   mine.exe\
   malpha.exe\
   mtest.exe

mw : BccDos.cfg $(Dep_mw)
  echo MakeNode

Dep_mwdlib = \
   egavga.obj\
   rom8x16.obj\
   winfreesansserif11x13.obj\
   winfreesystem14x16.obj\
   x6x13.obj\
   genfont.obj\
   kbd_tc.obj\
   mou_dos.obj\
   scr_tc.obj\
   devclip.obj\
   devdraw.obj\
   devkbd.obj\
   devlist.obj\
   devfont.obj\
   devmouse.obj\
   devpal4.obj\
   devpal2.obj\
   devpal1.obj\
   devrgn.obj\
   devrgn2.obj\
   devarc.obj\
   error.obj\
   cs1.obj\
   button.obj\
   edit.obj\
   progbar.obj\
   listbox.obj\
   draw3d.obj\
   fastfill.obj\
   graph3d.obj\
   insetr.obj\
   ptinsid.obj\
   mwuser.obj\
   winsbar.obj\
   winclip.obj\
   windefw.obj\
   winevent.obj\
   winexpos.obj\
   wingdi.obj\
   winmain.obj\
   winuser.obj

mw.lib : $(Dep_mwdlib)
  $(TLIB) $< $(IDE_BFLAGS) $(BOptsAt_mwdlib) @&&|
 -+egavga.obj &
-+rom8x16.obj &
-+winfreesansserif11x13.obj &
-+winfreesystem14x16.obj &
-+x6x13.obj &
-+genfont.obj &
-+kbd_tc.obj &
-+mou_dos.obj &
-+scr_tc.obj &
-+devclip.obj &
-+devdraw.obj &
-+devkbd.obj &
-+devlist.obj &
-+devfont.obj &
-+devmouse.obj &
-+devpal4.obj &
-+devpal2.obj &
-+devpal1.obj &
-+devrgn.obj &
-+devrgn2.obj &
-+devarc.obj &
-+error.obj &
-+cs1.obj &
-+button.obj &
-+edit.obj &
-+progbar.obj &
-+listbox.obj &
-+draw3d.obj &
-+fastfill.obj &
-+graph3d.obj &
-+insetr.obj &
-+ptinsid.obj &
-+mwuser.obj &
-+winsbar.obj &
-+winclip.obj &
-+windefw.obj &
-+winevent.obj &
-+winexpos.obj &
-+wingdi.obj &
-+winmain.obj &
-+winuser.obj
|

rom8x16.obj :  fonts\rom8x16.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ fonts\rom8x16.c
|

winfreesansserif11x13.obj :  fonts\winfreesansserif11x13.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ fonts\winfreesansserif11x13.c
|

winfreesystem14x16.obj :  fonts\winfreesystem14x16.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ fonts\winfreesystem14x16.c
|

x6x13.obj :  fonts\x6x13.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ fonts\x6x13.c
|

genfont.obj :  drivers\genfont.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ drivers\genfont.c
|

kbd_tc.obj :  drivers\kbd_tc.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ drivers\kbd_tc.c
|

mou_dos.obj :  drivers\mou_dos.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ drivers\mou_dos.c
|

scr_tc.obj :  drivers\scr_tc.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ drivers\scr_tc.c
|

devclip.obj :  engine\devclip.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devclip.c
|

devdraw.obj :  engine\devdraw.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devdraw.c
|

devkbd.obj :  engine\devkbd.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devkbd.c
|

devlist.obj :  engine\devlist.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devlist.c
|

devfont.obj :  engine\devfont.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devfont.c
|

devmouse.obj :  engine\devmouse.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devmouse.c
|

devpal4.obj :  engine\devpal4.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devpal4.c
|

devpal2.obj :  engine\devpal2.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devpal2.c
|

devpal1.obj :  engine\devpal1.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devpal1.c
|

devrgn.obj :  engine\devrgn.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devrgn.c
|

devrgn2.obj :  engine\devrgn2.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devrgn2.c
|

devarc.obj :  engine\devarc.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\devarc.c
|

error.obj :  engine\error.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ engine\error.c
|

cs1.obj :  mwin\bmp\cs1.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\bmp\cs1.c
|

button.obj :  mwin\winlib\button.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\button.c
|

edit.obj :  mwin\winlib\edit.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\edit.c
|

progbar.obj :  mwin\winlib\progbar.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\progbar.c
|

listbox.obj :  mwin\winlib\listbox.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\listbox.c
|

draw3d.obj :  mwin\winlib\draw3d.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\draw3d.c
|

fastfill.obj :  mwin\winlib\fastfill.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\fastfill.c
|

graph3d.obj :  mwin\winlib\graph3d.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\graph3d.c
|

insetr.obj :  mwin\winlib\insetr.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\insetr.c
|

ptinsid.obj :  mwin\winlib\ptinsid.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\ptinsid.c
|

mwuser.obj :  mwin\winlib\mwuser.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winlib\mwuser.c
|

winsbar.obj :  mwin\winsbar.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winsbar.c
|

winclip.obj :  mwin\winclip.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winclip.c
|

windefw.obj :  mwin\windefw.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\windefw.c
|

winevent.obj :  mwin\winevent.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winevent.c
|

winexpos.obj :  mwin\winexpos.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winexpos.c
|

wingdi.obj :  mwin\wingdi.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\wingdi.c
|

winmain.obj :  mwin\winmain.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winmain.c
|

winuser.obj :  mwin\winuser.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mwdlib) $(CompInheritOptsAt_mwdlib) -o$@ mwin\winuser.c
|

Dep_demodexe = \
   mw.lib\
   microwin.obj\
   penguin.obj\
   demo.obj

demo.exe : $(Dep_demodexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_demodexe) $(LinkerInheritOptsAt_demodexe) +
$(BC_Lib_Path)\c0l.obj+
microwin.obj+
penguin.obj+
demo.obj
$<,$*
mw.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
microwin.obj :  mwin\bmp\microwin.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demodexe) $(CompInheritOptsAt_demodexe) -o$@ mwin\bmp\microwin.c
|

penguin.obj :  mwin\bmp\penguin.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demodexe) $(CompInheritOptsAt_demodexe) -o$@ mwin\bmp\penguin.c
|

demo.obj :  demos\mwin\demo.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demodexe) $(CompInheritOptsAt_demodexe) -o$@ demos\mwin\demo.c
|

Dep_minedexe = \
   mw.lib\
   minebomb.obj\
   minedone.obj\
   mineface.obj\
   mineflag.obj\
   minehitf.obj\
   minelost.obj\
   mine.obj

mine.exe : $(Dep_minedexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_minedexe) $(LinkerInheritOptsAt_minedexe) +
$(BC_Lib_Path)\c0l.obj+
minebomb.obj+
minedone.obj+
mineface.obj+
mineflag.obj+
minehitf.obj+
minelost.obj+
mine.obj
$<,$*
mw.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
minebomb.obj :  demos\mwin\minebomb.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_minedexe) $(CompInheritOptsAt_minedexe) -o$@ demos\mwin\minebomb.c
|

minedone.obj :  demos\mwin\minedone.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_minedexe) $(CompInheritOptsAt_minedexe) -o$@ demos\mwin\minedone.c
|

mineface.obj :  demos\mwin\mineface.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_minedexe) $(CompInheritOptsAt_minedexe) -o$@ demos\mwin\mineface.c
|

mineflag.obj :  demos\mwin\mineflag.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_minedexe) $(CompInheritOptsAt_minedexe) -o$@ demos\mwin\mineflag.c
|

minehitf.obj :  demos\mwin\minehitf.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_minedexe) $(CompInheritOptsAt_minedexe) -o$@ demos\mwin\minehitf.c
|

minelost.obj :  demos\mwin\minelost.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_minedexe) $(CompInheritOptsAt_minedexe) -o$@ demos\mwin\minelost.c
|

mine.obj :  demos\mwin\mine.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_demosbmwinbminedc) $(CompInheritOptsAt_demosbmwinbminedc) -o$@ demos\mwin\mine.c
|

Dep_malphadexe = \
   mw.lib\
   car8.obj\
   malpha.obj

malpha.exe : $(Dep_malphadexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_malphadexe) $(LinkerInheritOptsAt_malphadexe) +
$(BC_Lib_Path)\c0l.obj+
car8.obj+
malpha.obj
$<,$*
mw.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
car8.obj :  mwin\bmp\car8.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_malphadexe) $(CompInheritOptsAt_malphadexe) -o$@ mwin\bmp\car8.c
|

malpha.obj :  demos\mwin\malpha.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_malphadexe) $(CompInheritOptsAt_malphadexe) -o$@ demos\mwin\malpha.c
|

Dep_mtestdexe = \
   mw.lib\
   mtest.obj

mtest.exe : $(Dep_mtestdexe)
  $(TLINK)   @&&|
 /v $(IDE_LinkFLAGSDOS) $(LinkerOptsAt_mtestdexe) $(LinkerInheritOptsAt_mtestdexe) +
$(BC_Lib_Path)\c0l.obj+
mtest.obj
$<,$*
mw.lib+
$(BC_Lib_Path)\graphics.lib+
$(BC_Lib_Path)\fp87.lib+
$(BC_Lib_Path)\mathl.lib+
$(BC_Lib_Path)\cl.lib



|
mtest.obj :  demos\mwin\mtest.c
  $(BCCDOS) -P- -c @&&|
 $(CompOptsAt_mtestdexe) $(CompInheritOptsAt_mtestdexe) -o$@ demos\mwin\mtest.c
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
-H=mw.csm
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

