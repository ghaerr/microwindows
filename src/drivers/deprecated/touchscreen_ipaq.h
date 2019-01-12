#ifndef TS_DEVICE
#define TS_DEVICE "Ipaq"

#define TS_DEVICE_FILE "/dev/h3600_tsraw"

struct ts_event {
	short pressure;
	short x;
	short y;
	short pad;
};

#endif
