dnl Process this file with autoconf to produce a configure script.
AC_INIT(README.b)

dnl Setup for automake
AM_INIT_AUTOMAKE(sdldoom, 1.10)

dnl Check for tools

AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_INSTALL

dnl Check for SDL
SDL_VERSION=1.0.1
AM_PATH_SDL($SDL_VERSION,
            :,
            AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

# Finally create all the generated files
AC_OUTPUT([
Makefile
])
