#ifndef TS_DEVICE
#define TS_DEVICE "TomTom"

#define TS_DEVICE_FILE "/dev/ts"

struct ts_event {
        short pressure;
        short x;
        short y;
        short pad;
};

#endif
