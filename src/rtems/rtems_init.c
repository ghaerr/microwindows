/*
/////////////////////////////////////////////////////////////////////////////
// $Header: /usr/cvs/microwin/src/rtems/rtems_init.c,v 1.1.1.1 2001/06/21 06:32:42 greg Exp $
//
// Copyright (c) 1999 ConnectTel, Inc. All Rights Reserved.
//  
// MODULE DESCRIPTION:
//
//  RTEMS Init Task for a MicroWindows application that 
//        may or may not use network functionality.
//
//  by: Rosimildo da Silva:
//      rdasilva@connecttel.com
//      http://www.connecttel.com
//
// MODIFICATION/HISTORY:
// $Log: rtems_init.c,v $
// Revision 1.1.1.1  2001/06/21 06:32:42  greg
// Microwindows pre8 with patches
//
// Revision 1.1.1.1  2001/06/05 03:44:03  root
// First import of 5/5/2001 Microwindows to CVS
//
//
/////////////////////////////////////////////////////////////////////////////
*/

#define  CONFIGURE_INIT
#include <stdio.h>
#include "rtemscfg.h"

#ifndef  NONETWORK
#include "net_cfg.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include "device.h"  /* DPRINTF() macro */

/* Enable the following define if you want to debug this application */
/* #define USE_REMOTE_GDB__ */
#ifdef USE_REMOTE_GDB__
extern void init_remote_gdb( void );
#define BREAKPOINT() asm("   int $3");
#endif

/* this is the command line options to be passed to the main routine */
char *cc_argv[] = 
{
	"cc_main",    /* arg[ 0 ] -- always the name of the program */
};
int cc_argc = sizeof( cc_argv ) / sizeof( cc_argv[ 0 ]  );


extern int rtems_main(int argc, char **argv);

/*
 * DESCRIPTION: Init task for any MicroWindows/RTEMS application.
 */
void *POSIX_Init( void *argument )
{
  size_t st = 0;

#ifdef USE_REMOTE_GDB__
  init_remote_gdb();
/*  BREAKPOINT(); */
#endif

  DPRINTF( "\nStarting RTEMS init task...\n" );

#ifndef NONETWORK
  /* Make all network initialization */
  rtems_bsdnet_initialize_network();
  DPRINTF( "Netowrk Initialization is complete.\n\n" );
#endif

  st = _Thread_Executing->Start.Initial_stack.size;
  DPRINTF( "Init Task Stack Size is: %d\n", st );

  rtems_main( cc_argc, cc_argv );
  DPRINTF( "*** Done ***\n\n\n" );
  pthread_exit( NULL );
  return NULL; /* just so the compiler thinks we returned something */
}
