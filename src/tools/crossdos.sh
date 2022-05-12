#!/bin/bash
#
# Build DOS Microwindows/Nano-X using cross-DJGPP

BUILD_HOME=$(pwd)
export INSTALLED=${BUILD_HOME}/../../installed/dos
DJGPP=${BUILD_HOME}/../../djgpp

# for macOS testing until possible to compile external libraries
#export NOEXTLIBS=Y

make -f Makefile_nr \
    ARCH=CROSSDOS \
	TOOLSPREFIX=${DJGPP}/bin/i586-pc-msdosdjgpp- \
	INCLUDE_FLAGS="-I${INSTALLED}/include \
    -I${INSTALLED}/include/freetype2 \
    -I${BUILD_HOME}/nx11/X11-local" \
	LIBRARY_FLAGS="-L${INSTALLED}/lib" \
    EXTENGINELIBS=""
