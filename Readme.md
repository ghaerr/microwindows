
# Microwindows or the Nano-X Window System

Microwindows or Nano-X is a small, Xlib compatible library. It is
Open Source and licenced under the the Mozilla Public License.
The main GUI toolkit that can be used with Microwindows is FLTK.

This is an overview of the system:

<img src="https://user-images.githubusercontent.com/10487642/40860798-38ddb54a-65d6-11e8-8ab8-9c1f1f329f47.png" width="90%"></img> 

The supported platforms are listed at the bottom on the left side. Next to
that there are the available screen drivers, mouse drivers and keyboard
drivers. The Nano-X / Microwindows engine is the core code which calls the 
various drivers and provides a platform for the two available APIs. These
are the Nano-X API and the MWin API which can be used to write applications.
To provide close X11 compatibility there are the NX11 and PX11 libraries
which are based on the Nano-X API. Based on these compatibility libraries,
which provide a limited X11 compatibility, X11 applications can be used. 
The FLTK GUI toolkit will run based on NX11 or PX11.

## Areas where Microwindows can be used are:

- provide an excellent GUI for small embedded devices with FLTK
- port the FLTK GUI toolkit to new platforms
- linking an application with Microwindows can provide it with a GUI 
  on console based Linux distros
- provide Xlib support for platforms without Xlib

It features two APIs: Nano-X/NX11 which provides an 
X11 compatible API and MWIN which is compatible to the Win32 API.

## Platforms

Microwindows is very portable, is written in C and can be used on a wide range 
of platforms. 

Currently it is available for **Linux, Windows, Mac OS X, Android, DOS, 
Raspberry Pi, uclinux, RTEMS, ELKS, PSP** and more. 

It can be compiled for **x86, x86_64, ARM, PowerPC and MIPS** processors.

**Microwindows has been ported to the Android platform. This way you can
use Microwindows and FLTK on Android smartphones.**

Download  this port from the Microwindows-Android-bin repo:
<https://github.com/georgp24/microwindows-android-bin>


## Library design

The Nano-X Window System has a layered design. At the lowest level there are 
drivers for screen output as well as mouse and keyboard input. Touch input 
devices are also supported. Drivers are available for **X11, frame buffer, 
SDL, the Allegro graphics library, VESA or the SVGA library**. Additional 
drivers can be added to port Microwindows to more platforms.

At the mid level, a portable graphics engine is implemented, providing 
support for line draws, area fills, polygons, clipping and color models. 

At the upper level, the Nano-X/NX11 and MWIN API's are implemented 
providing access to the graphics applications programmer. 

The Nano-X Window System can be compiled either as a separate server for 
several clients or linked together to a single, standalone library.

## Web site

The main Nano-X web site is at <http://www.microwindows.org>

## Install

To build Microwindows, see the files /src/INSTALL and /src/CONTENTS 
plus the faq files in the doc directory. For Android there is an
application note in the doc directory.

An HTML based FAQ and Architecture document are available from the web site.

## Links

The chief maintainer of the project is Greg Haerr <greg@censoft.com>

Microwindows and Nano-X are discussed on the NanoGUI mailing list. 
Mailing list archives are available at: 
<http://www.linuxhacker.org/ezmlm-browse/index.cgi?list=nanogui>

