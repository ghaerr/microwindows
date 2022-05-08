#!/bin/bash
#
# Build macOS Microwindows/Nano-X from macOS for testing DJGPP Makefile_nr

BUILD_HOME=$(pwd)
export INSTALLED=${BUILD_HOME}/../../installed/macos

make -f Makefile_nr \
    ARCH=MACOS \
    INCLUDE_FLAGS="-I${INSTALLED}/include -I${INSTALLED}/include/freetype2 -I${BUILD_HOME}/nx11/X11-local" \
    EXTENGINELIBS="-lSDL2"
