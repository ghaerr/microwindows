/*
 * This file is now the entire contents of libmwinlib.
 * Do not use libmwinlib in new code - it is provided only
 * to keep backwards-compatibility with old programs.
 *
 * Where you previously used:
 *     -lmwin -lmwinlib -lmwengine -lmwdrivers -lmwfonts
 * you now only have to use:
 *     -lmwin
 *
 ************************************************************************
 * These obsolete libraries will be removed in a future release of      *
 * Microwindows.  This affects libmwengine, libmwdrivers, libmwfonts    *
 * and libmwinlib.                                                      *
 ************************************************************************
 *
 *
 * Copyright (C) 2003 Jon Foster <jon@jon-foster.co.uk>
 */

char gr_warning_libmwinlib_is_obsolete[] =
    "WARNING: libmwinlib is obsolete and should not be used any more.\n"
    "WARNING: The code is now built into libmwin.\n"
    "WARNING: Just use libmwin, not mwengine, mwdrivers, mwfonts, or mwinlib.\n";


