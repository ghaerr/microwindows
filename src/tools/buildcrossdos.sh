#!/bin/bash -x
#
# Cross compile external libs for Microwindows using cross DJGPP on Linux
#

BUILD_HOME=$(pwd)
EXTLIBS_HOME=$(pwd)

export INSTALLED=${BUILD_HOME}/installed/dos
export DJGPP=${BUILD_HOME}/djgpp

X11HDRLOCATION=${BUILD_HOME}/microwindows/src/nx11/X11-local
#X11HDRLOCATION=/usr/X11/include

# functions
if test "$(uname)" = "Darwin"; then sed="sed -i ''"; else sed="sed -i"; fi

function build_zlib()
{
    (
        source ${DJGPP}/setenv

        pushd ${EXTLIBS_HOME}/zlib-1.2.12
        make clean
        ./configure --prefix=${INSTALLED}
        make
        make install
        popd
    )
}

function build_png()
{
    (
        source ${DJGPP}/setenv

        pushd ${EXTLIBS_HOME}/libpng-1.6.37
        make clean
        cp scripts/makefile.dj2 .
        $sed "s#\-I\.\./zlib#\-I${INSTALLED}/include \-DPNG_NO_CONSOLE_IO#g" makefile.dj2
        $sed "s#\-L\.\./zlib/#\-L${INSTALLED}/lib#g" makefile.dj2
        make -f makefile.dj2 libpng.a
        cp -p -f *.a ${INSTALLED}/lib
        cp -p -f *.h ${INSTALLED}/include
        #make
        #make install
        popd
    )
}

function build_jpeg()
{
    (
        source ${DJGPP}/setenv

        pushd ${EXTLIBS_HOME}/jpeg-6b
        make clean
        make -i -f makefile.dj clean
        make -i -f makefile.dj
        cp -p -f *.a ${INSTALLED}/lib
        cp -p -f *.h ${INSTALLED}/include
        popd
    )
}

function build_freetype()
{
    (
        source ${DJGPP}/setenv

        PATH=${DJGPP}/bin:$PATH
        pushd ${EXTLIBS_HOME}/freetype-2.12.1
        #pushd ${EXTLIBS_HOME}/freetype-VER-2-10-4
        if [[ ! -e builds/exports.mk.original ]]; then
            mv builds/exports.mk builds/exports.mk.original
            echo "exports:" > builds/exports.mk
            echo -e "\t\$(info Skipping exports.)" >> builds/exports.mk
            touch objs/ftexport.sym
        fi
        sh autogen.sh
        make clean
        ZLIB_CFLAGS="-I${INSTALLED}/include" \
        ZLIB_LIBS="-L${INSTALLED}/lib -lz" \
        LIBPNG_CFLAGS="-I${INSTALLED}/include" \
        LIBPNG_LIBS="-L${INSTALLED}/lib -lpng" \
        CFLAGS="-Doff_t=long" \
        ./configure --prefix=${INSTALLED} --build=x86_64-linux-gnu --host=i586-pc-msdosdjgpp
        #./configure --without-bzip2 --without-harfbuzz --prefix=${INSTALLED}
        make
        make install
        popd
    )
}

function build_microwindows()
{
    pushd microwindows/src
    ln -s tools/cleandos.sh .
    ln -s tools/crossdos.sh .
    ./cleandos.sh
    ./crossdos.sh
    cp -p -f lib/*.a ${INSTALLED}/lib
    cp -p -f include/nano-X.h ${INSTALLED}/include
    cp -p -f include/mwtypes.h ${INSTALLED}/include
    popd
}

function build_fltk()
{
    (
        source ${DJGPP}/setenv

        cp -f /usr/include/X11/Xmd.h $X11HDRLOCATION/X11/.
        cp -f /usr/include/X11/cursorfont.h $X11HDRLOCATION/X11/.
        pushd fltk-1.3.8
        sh autogen.sh
        make clean
        CFLAGS="-I${INSTALLED}/include" \
        CPPFLAGS="-I${INSTALLED}/include" \
        LDFLAGS="-L${INSTALLED}/lib -lz -lpng -ljpeg" \
        ./configure \
            --prefix=$INSTALLED \
            --build=x86_64-linux-gnu \
            --host=i586-pc-msdosdjgpp \
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
            --disable-localjpeg \
            --disable-localzlib \
            --disable-localpng \
            --with-x \
            --x-includes=$X11HDRLOCATION \
            --x-libraries=$X11LIBLOCATION
        cp -f "$(which fluid)" fluid/fluid.exe
        $sed "s/fluid test documentation/test documentation/g" Makefile
        $sed "s/sudoku.exe/sudoku.ignore/g" test/Makefile
        $sed "s/sudoku:/sudoku\$(EXEEXT):/g" test/Makefile
        $sed -e "s#$X11LIBLOCATION#$INSTALLED/lib#" makeinclude
        $sed -e "s#$X11LIBLOCATION#$INSTALLED/lib#" fltk-config
        $sed -e "s#^DSOFLAGS.*=.*#DSOFLAGS  = -L. -L$INSTALLED/lib#" makeinclude
        $sed -e "s#^LDFLAGS.*=.*#LDFLAGS  = \$(OPTIM) -L$INSTALLED/lib#" makeinclude
        $sed -e "s/-lX11/-lNX11 -lnano-X -lfreetype -ljpeg -lpng -lz/" makeinclude
        $sed -e "s/-lX11/-lNX11 -lnano-X -lfreetype -ljpeg -lpng -lz/" fltk-config
        [[ ! -e patched ]] && patch -p1 < $BUILD_HOME/microwindows/src/tools/patch-fltk-1.3.8 && touch patched
        make
        popd
    )
}

mkdir -p installed/dos
#build_zlib
#build_png
#build_jpeg
#build_freetype
#build_microwindows
build_fltk
