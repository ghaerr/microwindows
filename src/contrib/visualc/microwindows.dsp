# Microsoft Developer Studio Project File - Name="microwindows" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=microwindows - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "microwindows.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "microwindows.mak" CFG="microwindows - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "microwindows - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "microwindows - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "microwindows - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x404 /d "NDEBUG"
# ADD RSC /l 0x404 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "microwindows - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\src\include" /I "..\..\linuxCompat" /D "_CONSOLE" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "NOSTDPAL8" /D "NOSTDPAL4" /D "NOSTDPAL2" /D MWPIXEL_FORMAT=MWPF_TRUECOLOR565 /D THREADSAFE=0 /D SCREEN_HEIGHT=480 /D SCREEN_WIDTH=640 /D NONETWORK=1 /D MW_CPU_BIG_ENDIAN=0 /D HAVE_WARNING=1 /D "USE_EXPOSURE" /U "HAVE_FILEIO" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x404 /d "_DEBUG"
# ADD RSC /l 0x404 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\fltkMmi\libs\microwindows.lib"

!ENDIF 

# Begin Target

# Name "microwindows - Win32 Release"
# Name "microwindows - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\nxlib\AllocColor.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Atom.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Backgnd.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\BdrWidth.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Bell.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Border.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ChProperty.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ChWindow.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ClassHint.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ClDisplay.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Clear.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ClearArea.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Colormap.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Colorname.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Copy.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\CrBFData.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\CrCursor.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\CrGC.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\CrPFBData.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\CrPixmap.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\CrWindow.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\DefCursor.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\DestWind.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devarc.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devclip.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devdraw.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devfont.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devimage.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devkbd.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devlist.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devmouse.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devopen.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devpal1.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devpal2.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devpal4.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devpal8.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devpoly.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devrgn.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devrgn2.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devstipple.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\devtimer.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\DrArc.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\DrLine.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\DrLines.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\DrPoint.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\DrRect.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\error.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ErrorHandler.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fb.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fblin1.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fblin16.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fblin2.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fblin24.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fblin32.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fblin32alpha.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fblin4.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\fblin8.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\FillPolygon.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\FillRct.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Flush.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\font.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\font_dbcs.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\FontCursor.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Free.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\FreeGC.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\genfont.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\genmem.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\GetGeom.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Image.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\kbd_win32.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ListFonts.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ListPix.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\LoadFont.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\LowerWin.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\MapRaised.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\MapWindow.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Misc.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\mou_win32.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\MoveWin.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\NextEvent.c
# End Source File
# Begin Source File

SOURCE=..\src\nanox\nxdraw.c
# End Source File
# Begin Source File

SOURCE=..\src\nanox\nxtransform.c
# End Source File
# Begin Source File

SOURCE=..\src\nanox\nxutil.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\OpenDis.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\ParseColor.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\PmapBgnd.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Quarks.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\QueryColor.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\QueryFont.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\QueryPointer.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\QueryTree.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\RaiseWin.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Region.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\RepWindow.c
# End Source File
# Begin Source File

SOURCE=..\src\fonts\rom8x16.c
# End Source File
# Begin Source File

SOURCE=..\src\fonts\rom8x8.c
# End Source File
# Begin Source File

SOURCE=..\src\drivers\scr_win32.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Selection.c
# End Source File
# Begin Source File

SOURCE=..\src\engine\selfont.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\SelInput.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\SetAttributes.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\SetClip.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\SetFontPath.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\SetIFocus.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\SetWMProps.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Shape.c
# End Source File
# Begin Source File

SOURCE=..\src\nanox\srvclip.c
# End Source File
# Begin Source File

SOURCE=..\src\nanox\srvevent.c
# End Source File
# Begin Source File

SOURCE=..\src\nanox\srvfunc.c
# End Source File
# Begin Source File

SOURCE=..\src\nanox\srvmain.c
# End Source File
# Begin Source File

SOURCE=..\src\nanox\srvutil.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\StName.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\StrKeysym.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\StrToText.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\stub.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Sync.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Text.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Text16.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\TextExt.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\UndefCurs.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\UnloadFont.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\UnmapWin.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Visual.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\Window.c
# End Source File
# Begin Source File

SOURCE=..\nxlib\WindowProperty.c
# End Source File
# Begin Source File

SOURCE=..\src\fonts\winFreeSansSerif11x13.c
# End Source File
# Begin Source File

SOURCE=..\src\fonts\winFreeSystem14x16.c
# End Source File
# Begin Source File

SOURCE=..\src\fonts\X6x13.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\libw11\X11\Xlib.h
# End Source File
# End Group
# End Target
# End Project
