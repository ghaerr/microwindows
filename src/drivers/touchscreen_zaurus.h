#ifndef TS_DEVICE
#define TS_DEVICE "Zaurus"

#define TS_DEVICE_FILE "/dev/sharp_ts"

struct ts_event {
	long x;
	long y;
	long pressure;
	long long timestamp;
};

#endif

