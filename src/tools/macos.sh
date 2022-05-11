#!/bin/bash
#
# Build macOS Microwindows/Nano-X from macOS for testing DJGPP Makefile_nr

BUILD_HOME=$(pwd)
export INSTALLED=${BUILD_HOME}/../../installed/macos

#X11HDRLOCATION=${BUILD_HOME}/microwindows/src/nx11/X11-local
X11HDRLOCATION=/opt/X11/include

make -f Makefile_nr \
    ARCH=MACOS \
    INCLUDE_FLAGS="-I${INSTALLED}/include -I${INSTALLED}/include/freetype2 -I${X11HDRLOCATION}" \
    EXTENGINELIBS="-lSDL2"
