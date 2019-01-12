#ifndef TS_DEVICE
#define TS_DEVICE "UCB1X00"

#define TS_DEVICE_FILE "/dev/ucb1x00-ts"

#include <sys/time.h>

struct ts_event {
	unsigned short	pressure;
	unsigned short	x;
	unsigned short	y;
	unsigned short	pad;
	struct timeval	stamp;
};

#endif

