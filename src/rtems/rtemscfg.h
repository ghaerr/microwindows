/***************************************************************************
 *
 * $Header: /usr/cvs/microwin/src/rtems/rtemscfg.h,v 1.1.1.1 2001/06/21 06:32:42 greg Exp $
 *
 * Copyright (c) 1999 ConnectTel, Inc. All Rights Reserved.
 *  
 * MODULE DESCRIPTION: RTEMS configuration file. This file should be
 *                     udpated to match your own configuration.
 *
 * by: Rosimildo da Silva:
 *     rdasilva@connecttel.com
 *     http://www.connecttel.com
 *
 * MODIFICATION/HISTORY:
 *
 * $Log: rtemscfg.h,v $
 * Revision 1.1.1.1  2001/06/21 06:32:42  greg
 * Microwindows pre8 with patches
 *
 * Revision 1.1.1.1  2001/06/05 03:44:03  root
 * First import of 5/5/2001 Microwindows to CVS
 *
 ****************************************************************************/


#include <bsp.h>              /* includes <rtems.h> */
#include <tty_drv.h>
#include <rtems/ps2_drv.h>
#include <rtems/fb_vga.h>
#include <rtems/serial_mouse.h>

/* functions */
#ifdef __cplusplus
extern "C" {
#endif

/* configuration information */
#define CONFIGURE_MAXIMUM_DEVICES                     40
#define CONFIGURE_MAXIMUM_TASKS                       300
#define CONFIGURE_MAXIMUM_TIMERS                      32
#define CONFIGURE_MAXIMUM_SEMAPHORES                  100
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES              20
#define CONFIGURE_MAXIMUM_PARTITIONS                  10
#define CONFIGURE_MAXIMUM_REGIONS                     10

/* This seetings overwrite the ones defined in confdefs.h */
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES 		         32
#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES  	32
#define CONFIGURE_MAXIMUM_POSIX_KEYS         		   32
#define CONFIGURE_MAXIMUM_POSIX_QUEUED_SIGNALS 		   10
#define CONFIGURE_MAXIMUM_POSIX_THREADS      		   256
#define CONFIGURE_MAXIMUM_POSIX_TIMERS                10

/* used by the input device driver */
#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES        10

void *POSIX_Init( void *argument );
#define CONFIGURE_INIT_TASK_STACK_SIZE	               (512*1024)
#define CONFIGURE_POSIX_INIT_THREAD_STACK_SIZE        (512*1024)
#define CONFIGURE_POSIX_INIT_THREAD_TABLE


#define CONFIGURE_HAS_OWN_DEVICE_DRIVER_TABLE
#define CONFIGURE_MICROSECONDS_PER_TICK	        1000


/* List of device drivers loaded by RTEMS at boot time */
rtems_driver_address_table Device_drivers[] = 
{
  CONSOLE_DRIVER_TABLE_ENTRY,
  CLOCK_DRIVER_TABLE_ENTRY,

/* this can be a bit tricky: If you are using a serial mouse,
   make sure that you do not install and serial driver that
   uses the same port !!!!
*/
  TTY2_DRIVER_TABLE_ENTRY,

#if PS2_MOUSE
  PAUX_DRIVER_TABLE_ENTRY,
#else

  /*
   * Make sure that you have selected the COM port and the
   * mouse type in ( c/src/lib/libbsp/i386/pc386/console/serial_mouse.h ).
   */
  SERIAL_MOUSE_DRIVER_TABLE_ENTRY,
#endif

  /* Standard VGA driver */
  FBVGA_DRIVER_TABLE_ENTRY,

  { NULL,NULL, NULL,NULL,NULL, NULL }
};

#include <confdefs.h>

#ifdef __cplusplus
}
#endif
/* end of include file */
