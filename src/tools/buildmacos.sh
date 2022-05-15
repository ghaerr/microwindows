#!/bin/bash
#
# Cross compile external libs for Microwindows on macOS
#

BUILD_HOME=$(pwd)
EXTLIBS_HOME=$(pwd)

export INSTALLED=${BUILD_HOME}/installed/macos
#export DJGPP=${BUILD_HOME}/djgpp

#X11HDRLOCATION=${BUILD_HOME}/microwindows/src/nx11/X11-local
X11HDRLOCATION=/opt/X11/include

# functions
if test "$(uname)" = "Darwin"; then sed="sed -i ''"; else sed="sed -i"; fi

function build_zlib()
{
    pushd ${EXTLIBS_HOME}/zlib-1.2.12
    make clean
    ./configure --prefix=${INSTALLED}
    make
    make install
    popd
}

function build_png()
{
    pushd ${EXTLIBS_HOME}/libpng-1.6.37
    make clean
    #cp scripts/makefile.dj2 .
    #$SED "s#\-I\.\./zlib#\-I${INSTALLED}/include \-DPNG_NO_CONSOLE_IO#g" makefile.dj2
    #$SED "s#\-L\.\./zlib/#\-L${INSTALLED}/lib#g" makefile.dj2
    #make -f makefile.dj2 libpng.a
    #cp -p -f *.a ${INSTALLED}/lib
    #cp -p -f *.h ${INSTALLED}/include
    ./configure --prefix=${INSTALLED}
    make
    make install
    popd
}

function build_jpeg()
{
    #pushd ${EXTLIBS_HOME}/jpeg-6b
    pushd ${EXTLIBS_HOME}/libjpeg-6b-master
    make clean
    #make -i -f makefile.dj clean
    #make -i -f makefile.dj
    ./configure --prefix=${INSTALLED}
    make
    cp -p -f *.a ${INSTALLED}/lib
    cp -p -f *.h ${INSTALLED}/include
    popd
}

function build_freetype()
{

    #PATH=${DJGPP}/bin:$PATH
    #pushd ${EXTLIBS_HOME}/freetype-2.12.1
    pushd ${EXTLIBS_HOME}/freetype-VER-2-10-4
    sh autogen.sh
    make clean
    ZLIB_CFLAGS="-I${INSTALLED}/include" \
    ZLIB_LIBS="-L${INSTALLED}/lib -lz" \
    LIBPNG_CFLAGS="-I${INSTALLED}/include" \
    LIBPNG_LIBS="-L${INSTALLED}/lib -lpng" \
    #CFLAGS="-Doff_t=long" \
    #./configure --prefix=${INSTALLED} --build=x86_64-linux-gnu --host=i586-pc-msdosdjgpp
    ./configure --without-bzip2 --without-harfbuzz --prefix=${INSTALLED}
    make
    make install
    popd
}

function build_microwindows()
{
    pushd microwindows/src
    make -f Makefile_nr ARCH=MACOS clean
    make -f Makefile_nr ARCH=MACOS \
	    INCLUDE_FLAGS="-I${INSTALLED}/include -I${INSTALLED}/include/freetype2 -I${X11HDRLOCATION}" \
	    EXTENGINELIBS="-lSDL2"
    cp -p -f lib/*.a ${INSTALLED}/lib
    cp -p -f include/nano-X.h ${INSTALLED}/include
    cp -p -f include/mwtypes.h ${INSTALLED}/include
    popd
}

function build_fltk()
{
    pushd fltk-1.3.8
    sh autogen.sh
    ./configure \
        --prefix=$INSTALLED \
        --enable-x11 \
        --disable-xft \
        --disable-xdbe \
        --disable-xinerama \
        --disable-xfixes \
        --disable-xrender \
        --disable-xcursor \
        --disable-gl \
        --disable-threads \
        --disable-largefile \
        --with-x \
        --x-includes=$X11HDRLOCATION \
        --x-libraries=$X11LIBLOCATION
    $sed -e "s#$X11LIBLOCATION#$INSTALLED/lib#" makeinclude
    $sed -e "s#$X11LIBLOCATION#$INSTALLED/lib#" fltk-config
    $sed -e "s#^DSOFLAGS.*=.*#DSOFLAGS  = -L. -L$INSTALLED/lib#" makeinclude
    $sed -e "s#^LDFLAGS.*=.*#LDFLAGS  = \$(OPTIM) -L$INSTALLED/lib#" makeinclude
    $sed -e "s/-lX11/-lNX11 -lnano-X -lfreetype -ljpeg -lpng -lz -lSDL2/" makeinclude
    $sed -e "s/-lX11/-lNX11 -lnano-X -lfreetype -ljpeg -lpng -lz -lSDL2/" fltk-config
    patch -p1 < $BUILD_HOME/microwindows/src/tools/patch-fltk-1.3.8
    make
    popd
}

#mkdir -p installed/dos
mkdir -p installed/macos
#source ${DJGPP}/setenv

build_zlib
build_png
build_jpeg
build_freetype
build_microwindows
build_fltk
