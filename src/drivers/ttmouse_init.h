/*
 * TomTom touchscreen init
 *
 * Copyright (c) 2006, Romain Beauxis <toots@rastageeks.org>
 * With sources from ttmp3 player.
 * Licence: GNU GPLv2
 */

#include "touchscreen_tomtom.h"

#define TS_DRIVER_MAGIC 'f'
#define TS_SET_RAW_OFF  _IO(TS_DRIVER_MAGIC, 15)
#define TS_SET_CAL      _IOW(TS_DRIVER_MAGIC, 11, struct ts_matrix)


struct ts_calib {
        int min1;
        int max1;
        int min2;
        int max2;
};

struct ts_matrix {
        long An;
        long Bn;
        long Cn;
        long Dn;
        long En;
        long Fn;
        long Divider;
        int xMin;
        int xMax;
        int yMin;
        int yMax;
};

int ttmouse_init(void);
int getRotate(void);

