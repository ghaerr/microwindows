/*
 * Copyright (c) 2000 Alex Holden <alex@linuxhacker.org>
 *
 * This file implements the device independant timer functions.
 *
 * When a part of the server wishes to set a timer, it should call the
 * GdAddTimer() function with the timeout parameter set to the number of
 * milliseconds before the timer should activate, the callback argument
 * set to the function which should be called when the timer expires, and
 * the arg argument set to the (void * type) argument which should be supplied
 * to the timer handler function. The GdAddTimer() returns a pointer to the
 * timer structure * which was created (or NULL if the creation failed for
 * some reason). The prototype for the callback function should look like:
 * void callbackfn(void *arg);
 *
 * If a part of the server wishes to destroy a timer before it has expired
 * (it is not necessary to do so after the timer has expired, as the timer
 * structure is automatically destroyed after the callback function is called),
 * it should call the GdDestroyTimer() function with the address of the timer
 * structure (which was returned by GdAddTimer()).
 *
 * If a part of the server wishes to destroy a timer but does not know the
 * address of it's timer structure, it can call GdFindTimer() with the
 * callback argument as a parameter. The argument must be unique to that
 * timer (the address of a structure or function is probably a good choice).
 * This function returns the address of the first timer found with that
 * argument, or NULL if no matching timer was found.
 *
 * The main select() loop needs to be called with a timeout obtained using the
 * GdGetNextTimeout(). GdGetNextTimeout() is called with the event loop
 * timeout in ms, and fills in the specified timeout structure, which should
 * be used as the argument to the select() call. The timeout returned by the
 * GdGetNextTimeout() call is decided by looking through the timer list for
 * the timer with the shortest amount of time remaining, and also at the
 * maximum delay parameter. If there are no timers on the timer list and the
 * timeout argument is 0, it will return FALSE, otherwise it will return TRUE.
 *
 * When the main select() loop times out, the GdTimeout() function should be
 * called. This will go through the timer list and call the callback functions
 * of all timers which have expired, then remove them from the timer list. At
 * the same time, you should check the value of the maximum timeout parameter
 * to see if it has expired (in which case you can then return to the client
 * with a timeout event). This function returns TRUE if the timeout specified in
 * the last GdGetNextTimeout() call has expired, or FALSE otherwise.
 *
 * Note that no guarantees can be made as to when exactly the timer callback
 * will be called as it depends on how often the GdTimeout() function is
 * called and how long any other timeouts in the queue before you take to
 * complete. Especially in the case where the client is linked into the server,
 * the client must call into the server on a regular basis, otherwise the
 * timers may run late.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "device.h"

static MWTIMER *timerlist = NULL;
static struct timeval mainloop_timeout;
static struct timeval current_time;

static void calculate_timeval(struct timeval *tv, MWTIMEOUT to); 
static signed long time_to_expiry(struct timeval *t);

MWTIMER *GdAddTimer(MWTIMEOUT timeout, MWTIMERCB callback, void *arg)
{
	MWTIMER *newtimer;

	if(!(newtimer = malloc(sizeof(MWTIMER)))) return NULL;

	gettimeofday(&current_time, NULL);

	if(timerlist) timerlist->prev = newtimer;

	calculate_timeval(&newtimer->timeout, timeout);
	newtimer->callback = callback;
	newtimer->arg = arg;
	newtimer->next = timerlist;
	newtimer->prev = NULL;
        newtimer->type   = MWTIMER_ONESHOT;
        newtimer->period = timeout;
	timerlist = newtimer;

	return newtimer;
}

MWTIMER *GdAddPeriodicTimer(MWTIMEOUT timeout, MWTIMERCB callback, void *arg)
{
    MWTIMER *newtimer;

    if(!(newtimer = malloc(sizeof(MWTIMER)))) return NULL;

    gettimeofday (&current_time, NULL);
    
    if (timerlist) timerlist->prev = newtimer;
    
    calculate_timeval (&newtimer->timeout, timeout);
    newtimer->callback = callback;
    newtimer->arg      = arg;
    newtimer->next     = timerlist;
    newtimer->prev     = NULL;
    newtimer->type     = MWTIMER_PERIODIC;
    newtimer->period   = timeout;
    timerlist          = newtimer;
    
    return newtimer;
}

void GdDestroyTimer(MWTIMER *timer)
{
	if(timer->next) timer->next->prev = timer->prev;
	if(timer->prev) timer->prev->next = timer->next;
	if(timer == timerlist) {
		if(timer->next) timerlist = timer->next;
		else timerlist = timer->prev;
	}
	free(timer);
}

MWTIMER *GdFindTimer(void *arg)
{
	MWTIMER *t = timerlist;

	while(t) {
		if(t->arg == arg) break;
		t = t->next;
	}

	return t;
}

MWBOOL GdGetNextTimeout(struct timeval *tv, MWTIMEOUT timeout)
{
	signed long i, lowest_timeout;
	MWTIMER *t = timerlist;

	if(!timeout && !timerlist) return FALSE;

	gettimeofday(&current_time, NULL);

	if(timeout) {
		calculate_timeval(&mainloop_timeout, timeout);
		lowest_timeout = time_to_expiry(&mainloop_timeout);
	} else {
		lowest_timeout = time_to_expiry(&t->timeout);
		mainloop_timeout.tv_sec = -1;
		t = t->next;
	}

	while(t) {
		i = time_to_expiry(&t->timeout);
		if(i < lowest_timeout) lowest_timeout = i;
		t = t->next;
	}

	if(lowest_timeout <= 0) {
		tv->tv_sec = 0;
		tv->tv_usec = 0;
	} else {
		tv->tv_sec = lowest_timeout / 1000;
		tv->tv_usec = (lowest_timeout % 1000) * 1000;
	}

	return TRUE;
}

MWBOOL GdTimeout(void)
{
	MWTIMER *n, *t = timerlist;

	gettimeofday(&current_time, NULL);

	while(t) {
		n = t->next;
		if(time_to_expiry(&t->timeout) <= 0) {
			t->callback(t->arg);
                        if (t->type == MWTIMER_ONESHOT)
                        {
                            /* One shot timer, is finished delete it now */
                            GdDestroyTimer(t);
                        }
                        else
                        {
                            /* Periodic timer needs to be reset */
                            calculate_timeval (&t->timeout, t->period);
                        }
		}
		t = n;
	}

	if(mainloop_timeout.tv_sec > 0 || mainloop_timeout.tv_usec > 0)
		if(time_to_expiry(&mainloop_timeout) <= 0)
			return TRUE;

	return FALSE;
}

static void calculate_timeval(struct timeval *tv, MWTIMEOUT to)
{
	tv->tv_sec = current_time.tv_sec + (to / 1000);
	tv->tv_usec = current_time.tv_usec + ((to % 1000) * 1000);
	if(tv->tv_usec > 1000000) {
		tv->tv_sec++;
		tv->tv_usec -= 1000000;
	}
}

static signed long time_to_expiry(struct timeval *t)
{
	MWTIMEOUT ret = (((t->tv_sec - current_time.tv_sec) * 1000) +
			((t->tv_usec - current_time.tv_usec) / 1000));

	return ret;
}
