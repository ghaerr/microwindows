#ifndef TS_DEVICE
#define TS_DEVICE "ADS"

#define TS_DEVICE_FILE "/dev/ts"

struct ts_event {
	short x;
	short y;
	short pressure;
};

#endif
