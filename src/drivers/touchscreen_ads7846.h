#ifndef TS_DEVICE
#define TS_DEVICE "ADS7846"

#define TS_DEVICE_FILE "/dev/innovator_ts"

struct ts_event {
  short pressure;
  short x;
  short y;
  short pad;
  struct timeval stamp;
};

#endif
