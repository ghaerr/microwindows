/*
 *  Copyright (c) 1999 ConnectTel, Inc. All Rights Reserved.
 *  
 *  RTEMS Init Task for a Microwindows application that 
 *        may or may not use network functionality.
 *
 *  by: Rosimildo da Silva:
 *      rdasilva@connecttel.com
 *      http://www.connecttel.com
 *
 * MODIFICATION/HISTORY:
 *    Microwindows pre8 with patches
 *
 */

#include <stdio.h>
#include <bsp.h>              /* includes <rtems.h> */
#include <rtems/shell.h>
#include <rtems/untar.h>

/*
 *  The tarfile image is built automatically externally.
 */
#include "FilesystemImage.h"

/*
 * TBD: This should not be on i386.  We need to find a more general
 *      way to know if the BSP has a framebuffer and/or mouse driver.
 */
#if defined(__i386__)
  #include <bsp/tty_drv.h>
  #include <rtems/ps2_drv.h>
  #include <rtems/serial_mouse.h>
  #include <rtems/framebuffer.h>
#endif


/* TBD: Find better way than this to deal with BSPs which do not have
 *      these driver entries.  This is a hacky cover up.
 */
#if 0

#ifndef PAUX_DRIVER_TABLE_ENTRY
  #define PAUX_DRIVER_TABLE_ENTRY NULL_DRIVER_TABLE_ENTRY
#endif
#ifndef TTY2_DRIVER_TABLE_ENTRY
  #define TTY2_DRIVER_TABLE_ENTRY NULL_DRIVER_TABLE_ENTRY
#endif
#ifndef SERIAL_MOUSE_DRIVER_TABLE_ENTRY
  #define SERIAL_MOUSE_DRIVER_TABLE_ENTRY NULL_DRIVER_TABLE_ENTRY
#endif

#endif

/* TBD: For now assume there is a network configuration.  The default is
 *      loopback.  RTEMS has to have networking enabled but we don't have
 *      to use it.  And the POSIX_Init here doesn't initialize the TCP/IP
 *      stack anyway. :)
 */
#include "net_cfg.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "device.h"  /* DPRINTF() macro */

/* this is the command line options to be passed to the main routine */
char *cc_argv[] = 
{
	"cc_main",    /* arg[ 0 ] -- always the name of the program */
};
int cc_argc = sizeof( cc_argv ) / sizeof( cc_argv[ 0 ]  );


extern int rtems_main(int argc, char **argv);
void *POSIX_Init( void *argument );

/*
 * DESCRIPTION: Init task for any Microwindows/RTEMS application.
 */
void *POSIX_Init( void *argument )
{
  #define ARGV_LIMIT 32
  #define LINE_LIMIT 128
  int     sc;
  int     mainArgc;
  char   *mainArgv[ARGV_LIMIT];
  char    Line[LINE_LIMIT];

  DPRINTF( "\nStarting RTEMS init task...\n" );

  DPRINTF( "" );
  DPRINTF("Loading filesystem image\n");
  (void) Untar_FromMemory( (char *)FilesystemImage, FilesystemImage_size );

  #if !defined(NONETWORK)
    /* Make all network initialization */
    rtems_bsdnet_initialize_network();
    DPRINTF( "Network Initialization is complete.\n\n" );
  #endif

  setenv( "HOME", "/", 1 );
  setenv( "T1LIB_CONFIG", "/fonts/t1lib/t1lib.config", 1 );
  
  /*
   *  Clear argv pointer array
   */
  for ( mainArgc=0 ; mainArgc<ARGV_LIMIT ; mainArgc++ )
    mainArgv[mainArgc] = NULL;

  strcpy( Line, "RTEMS " );

  #if 0 /* defined(WITH_ARGS) */
    DPRINTF("With arguments\n" );
    {
      char   *p;

      DPRINTF("Enter arguments> " );
      p = fgets( &Line[6], LINE_LIMIT - 6, stdin );
      if ( !p ) {
        DPRINTF("error reading arguments\n" );
        exit(0);
      }
    }
  #else
    DPRINTF("Without arguments\n" );
  #endif

  /*
   *  Break into arguments
   */
  sc = rtems_shell_make_args( Line, &mainArgc, mainArgv, ARGV_LIMIT - 1 );
  if ( sc ) {
    DPRINTF("Error parsing arguments\n" );
    exit(0);
  }

  rtems_main( mainArgc, mainArgv );

  DPRINTF( "*** Done ***\n\n\n" );
  pthread_exit( NULL );
  return NULL; /* just so the compiler thinks we returned something */
}


/*
 *  This is significantly lower than original port.  It is likely still
 *  too high.
 */
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES              20
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES               40
#define CONFIGURE_MAXIMUM_POSIX_THREADS               10
#define CONFIGURE_MAXIMUM_SEMAPHORES                  40
#define CONFIGURE_MAXIMUM_TASKS                       10
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS      20
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM
#define CONFIGURE_INIT_TASK_ATTRIBUTES    RTEMS_FLOATING_POINT

#define CONFIGURE_POSIX_INIT_THREAD_STACK_SIZE        (128*1024)
#define CONFIGURE_POSIX_INIT_THREAD_TABLE

#define CONFIGURE_MICROSECONDS_PER_TICK	              1000

#define PS2_MOUSE 1
#if PS2_MOUSE
  #define MOUSE_DRIVER PAUX_DRIVER_TABLE_ENTRY
#else
  /*
   * Make sure that you have selected the COM port and the
   * mouse type in ( c/src/lib/libbsp/i386/pc386/console/serial_mouse.h ).
   */
  #define MOUSE_DRIVER SERIAL_MOUSE_DRIVER_TABLE_ENTRY
#endif

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
/*
 *  All BSPs which have frame buffer support define BSP_HAS_FRAME_BUFFER.
 *  So if we don't have support for it statically, then it is the responsibility
 *  of the application to dynamically detect and install one.  Otherwise,
 *  Nano-X should fail very early.
 */
#if (BSP_HAS_FRAME_BUFFER == 1)
  #define CONFIGURE_APPLICATION_NEEDS_FRAME_BUFFER_DRIVER
#endif

#define CONFIGURE_APPLICATION_EXTRA_DRIVERS \
          MOUSE_DRIVER
//        TTY2_DRIVER_TABLE_ENTRY,

#define  CONFIGURE_INIT
#include <rtems/confdefs.h>
