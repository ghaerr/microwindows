#ifndef __NTIMER_H__
#define __NTIMER_H__

struct GsTimer {
    struct GsTimer* next;
    long due;
    void* some_func;
    void* some_args;
};

struct GsTimerHolder {
    struct GsTimer* head;
    struct GsTimer* tail;
    int numof;
};



#endif
