
# Microwindows or the Nano-X Window System

Microwindows or Nano-X is a small graphical windowing system that implements
both Win32 and Nano-X (X11-like) APIs for clipped graphics drawing in windows
on Linux, OSX, EMSCRIPTEN, Android and other platforms. It is
Open Source and licenced under the the Mozilla Public License.
For creating GUIs, the Win32 builtin controls and TinyWidget's controls
based on Nano-X are included. FLTK can be used with the X11 compability library NX11.

This is an overview of the system:

<img src="https://user-images.githubusercontent.com/10487642/40860798-38ddb54a-65d6-11e8-8ab8-9c1f1f329f47.png" width="90%"></img> 

Sopme of the supported platforms are listed at the bottom on the left side. Next to
that there are available screen drivers, mouse drivers and keyboard
drivers. The Nano-X / Microwindows engine is the core code that implements
all drawing and clipping, with the Win32 and Nano-X graphical windowing APIs implemented in seperately
configurable layers on top of that. The engine is configured to use various
OS platforms and associated screen, mouse and keyboard drivers, or bare hardware.
The Nano-X API and the Win32 APIs are used to write applications.
To provide close X11 compatibility the NX11 library
can be built on top of the Nano-X API, which allows X11 applications to be linked
and run without recompilation.  The FLTK GUI toolkit runs based on NX11.

## Areas where Microwindows can be used are:

- run X11 or Win32 applications in a browser using EMSCRIPTEN
- provide an excellent GUI for small embedded devices with FLTK, TinyWidgets or Win32
- port the FLTK GUI toolkit to small platforms
- linking an application with Microwindows can provide it with a GUI 
  on console based Linux distros
- provide Xlib support for platforms without Xlib
- Portrait and Landscape modes and auto-flipping are supported for handheld devies

## Platforms

Microwindows is very portable, is written in C and can be used on a wide range 
of platforms. 

Currently it is available for **Linux, Mac OS X, Android, EMSCRIPTEN,
Raspberry Pi, DOS DJGPP, RTEMS, ECOS, Sony PSP, Nintendo DS** and more. 

Currently supported screen drivers include **Memory-mapped framebuffer, X11,
SDL 2, Allegro, and an X11 based framebuffer emulator.**

Support for cross-compiler toolchains on **x86, x86_64, ARM, PowerPC and MIPS**,
big- and little-endian, and other processors.

Microwindows has been ported to the Android platform. This way you can
use Microwindows and FLTK on Android smartphones.

Download the Android port from the Microwindows-Android-bin repo:
<https://github.com/georgp24/microwindows-android-bin>


## Library design

The Nano-X Window System has a layered design. At the lowest level there are 
drivers for screen output as well as mouse and keyboard input. Touch input 
devices are also supported. Drivers are available for **X11, frame buffer, 
SDL, the Allegro graphics library, VESA or the SVGA library**. Additional 
drivers can be added to port Microwindows to more platforms.

At the mid level, a portable graphics engine is implemented, providing 
support for line draws, area fills, polygons, clipping and 1, 2, 4, 8, 15, 16, 24 and 32 bit color models. 

At the upper level, the Nano-X/NX11 and Win32 API's are implemented 
providing access to the graphics applications programmer. 

The Nano-X Window System can be compiled either as a separate server for 
several clients or linked together to a single, standalone library.

## Web site

The main Nano-X web site is at <http://www.microwindows.org>

## Install

To build Microwindows, see microwindows/src/README
plus the faq files in the doc directory. For Android there is an
application note in the doc directory.

An HTML based FAQ and Architecture document are available from the web site.

## Links

The chief maintainer of the project is Greg Haerr <greg@censoft.com>

Microwindows and Nano-X are discussed on the NanoGUI mailing list. 
Mailing list archives are available at: 
<http://www.linuxhacker.org/ezmlm-browse/index.cgi?list=nanogui>

