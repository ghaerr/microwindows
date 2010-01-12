/*
 * Critical section locking definitions for Microwindows
 * Copyright (c) 2002, 2003 by Greg Haerr <greg@censoft.com>
 *
 * The current implementation uses pthreads included in libc
 *
 * It's currently required that any locking mechanism
 * allow multiple locks on the same thread (ie. recursive calls)
 * This is necessary since routines nest calls on
 * LOCK(&nxGlobalLock). (nanox/client.c and nanox/nxproto.c)
 */
#if THREADSAFE
#define THREADSAFE_LINUX	1	/* use linux threadsafe routines*/
#endif

/*
 * Linux critical section locking definitions
 */
#if THREADSAFE_LINUX
#define __USE_GNU		/* define _NP routines*/
#include <pthread.h>
typedef pthread_mutex_t	MWMUTEX;

#if !defined(__CYGWIN__) && !RTEMS
/*
 * This definition doesn't require explicit initialization and -lpthread
 *
 * It uses a common (but non-standard) pthreads extension.
 */
#define LOCK_DECLARE(name)	MWMUTEX name = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#define LOCK_INIT(m)
#else
/* 
 * This definition requires adding -lpthreads to link all Nano-X applications
 * which isn't required if LOCK_DECLARE is used as above:  The pthread entry
 * points pthread_mutex_lock/unlock are included in the standard C library, but
 * pthread_mutex_init is not.  If this is not the case with your library,
 * include these routines, and add -lpthreads to your applications link line.
 */
#define LOCK_DECLARE(name)	MWMUTEX name
#if 1
/*
 * Use portable version.
 *
 * Note: Older libraries may not have these UNIX98 functions.  You may need
 * to use the old non-portable function name (see below).
 */
#define LOCK_INIT(m)	\
	{ \
	pthread_mutexattr_t attr; \
	pthread_mutexattr_init(&attr); \
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
	pthread_mutex_init((m), &attr); \
	}
#else
/* Use old non-portable function name */
#define LOCK_INIT(m)	\
	{ \
	pthread_mutexattr_t attr; \
	pthread_mutexattr_init(&attr); \
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_RECURSIVE_NP); \
	pthread_mutex_init((m), &attr); \
	}
#endif
#endif

#define LOCK_EXTERN(name)	extern MWMUTEX name
#define LOCK_FREE(m)		pthread_mutex_destroy(m)
#define LOCK(m)			pthread_mutex_lock(m)
#define UNLOCK(m)		pthread_mutex_unlock(m)
#endif /* THREADSAFE_LINUX*/

/* no locking support - dummy macros*/
#if !THREADSAFE
typedef int		MWMUTEX;

#define LOCK_DECLARE(name)	MWMUTEX name
#define LOCK_EXTERN(name)	extern MWMUTEX name
#define LOCK_INIT(m)
#define LOCK_FREE(m)
#define LOCK(m)
#define UNLOCK(m)
#endif /* !THREADSAFE*/
