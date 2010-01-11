/*
 * TomTom touchscreen init
 *
 * Copyright (c) 2006, Romain Beauxis <toots@rastageeks.org>
 * With sources from ttmp3 player.
 * Licence: GNU GPLv2
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "device.h"
#include "ttmouse_init.h"

int ttmouse_init(void) {

        struct ts_calib tscal;
        struct ts_matrix tsmatrix;
        int fd;
        long flag;


        tsmatrix.An = -363;
        tsmatrix.Bn = 0;
        tsmatrix.Cn = 360416;
        tsmatrix.Dn = 0;
        tsmatrix.En = 258;
        tsmatrix.Fn = -12676;
        tsmatrix.Divider = 1000;
        if((fd = open("/mnt/flash/sysfile/cal", O_RDONLY)) < 0) {
                EPRINTF("could not open touchscreen calibration file\n");
                tsmatrix.xMin = 0;
                tsmatrix.xMax = 1023;
                tsmatrix.yMin = 0;
                tsmatrix.yMax = 1023;
        } else {
                read(fd, &tscal, sizeof(tscal));
                close(fd);

                if(getRotate() == 1) {
                        tsmatrix.xMin = tscal.min2;
                        tsmatrix.xMax = tscal.max2;
                        tsmatrix.yMin = tscal.min1;
                        tsmatrix.yMax = tscal.max1;
                } else {
                
                        tsmatrix.xMin = tscal.min1;
                        tsmatrix.xMax = tscal.max1;
                        tsmatrix.yMin = tscal.min2;
                        tsmatrix.yMax = tscal.max2;
                }
        }
        if((fd = open(TS_DEVICE_FILE, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
                EPRINTF("could not open touchscreen\n");
                fd = -1;
        }
        flag = fcntl(fd, F_GETFL, 0);
        flag |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flag);

        ioctl(fd, TS_SET_CAL, &tsmatrix);
        ioctl(fd, TS_SET_RAW_OFF, NULL);

	return 0;
}


int getRotate(void) {

        char str[32];
        int fd;
        int i;

        if((fd = open("/proc/barcelona/shortname", O_RDONLY)) < 0) {
                EPRINTF("could not open /proc/barcelona/shortname: %s", strerror(errno));
                return -1;
        }
        if((i = read(fd, str, sizeof(str) - 1)) < 0) {
                EPRINTF("could not read from /proc/barcelona/shortname");
                close(fd);
                return -1;
        }
        close(fd);
        str[i] = '\0';
        if((i > 0) && (str[i - 1] == '\n'))
                str[i - 1] = '\0';

        if(strcmp(str, "GO") == 0) {
                // GO Classic
                return 1;
        } else if(strcmp(str, "GO 300") == 0) {
                // GO 300
                return 1;
        } else if(strcmp(str, "GO 500") == 0) {
                // GO 500
                return 1;
        } else if(strcmp(str, "GO 700") == 0) {
                // GO 700
                return 1;
        } else if(strcmp(str, "ONE") == 0) {
                // ONE
                return 0;
        } else if(strcmp(str, "RAIDER") == 0) {
                // RAIDER
                return 0;
        } else {
                // unknown - handle like a ONE
                return 0;
        }

}
