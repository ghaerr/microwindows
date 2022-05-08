#!/bin/bash
#
# Build DOS Microwindows/Nano-X using cross-DJGPP

BUILD_HOME=$(pwd)
INSTALLED=${BUILD_HOME}/../../installed/dos
DJGPP=${BUILD_HOME}/../../djgpp

make -f Makefile_nr \
    ARCH=CROSSDOS \
	TOOLSPREFIX=${DJGPP}/bin/i586-pc-msdosdjgpp- \
	INCLUDE_FLAGS="-I${INSTALLED}/include \
    -I${INSTALLED}/include/freetype2 \
    -I${BUILD_HOME}/nx11/X11-local" \
	LIBRARY_FLAGS="-L${INSTALLED}/lib" \
    EXTENGINELIBS=""
