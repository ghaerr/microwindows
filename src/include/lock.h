/*
 * Critical section locking definitions for Microwindows
 * Copyright (c) 2002 by Greg Haerr <greg@censoft.com>
 *
 * The current implementation uses pthreads
 *
 * It's currently required that any locking mechanism
 * allow multiple locks on the same thread (ie. recursive calls)
 * This is necessary since some routines nest calls to
 * the LOCK(&nxGlobalLock).
 */
#if THREADSAFE
#include <pthread.h>
typedef pthread_mutex_t	MWMUTEX;

#define LOCK_INIT(m)	\
	{ \
	pthread_mutexattr_t attr; \
	pthread_mutexattr_init(&attr); \
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_RECURSIVE_NP); \
	pthread_mutex_init((m), &attr); \
	}
#define LOCK_FREE(m)	pthread_mutex_destroy(m)
#define LOCK(m)		pthread_mutex_lock(m)
#define UNLOCK(m)	pthread_mutex_unlock(m)

#else
typedef int		MWMUTEX;

#define LOCK_INIT(m)
#define LOCK_FREE(m)
#define LOCK(m)
#define UNLOCK(m)
#endif /* THREADSAFE*/
