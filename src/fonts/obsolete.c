/*
 * This file is now the entire contents of libmwfonts.
 * Do not use libmwfonts in new code - it is provided only
 * to keep backwards-compatibility with old programs.
 *
 * Where you previously used:
 *     -lnano-X -lmwengine -lmwdrivers -lmwfonts
 * you now only have to use:
 *     -lnano-X
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

char gr_warning_libmwfonts_is_obsolete[] =
    "WARNING: libmwfonts is obsolete and should not be used any more.\n"
    "WARNING: The code is now built into libnano-X and libmwin.\n"
    "WARNING: Just use libnano-X or libmwin, not mwengine, mwdrivers, ...\n"
    "WARNING: ... mwfonts, or mwinlib.\n";


