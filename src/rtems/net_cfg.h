/***************************************************************************
 *
 * $Header: /usr/cvs/microwin/src/rtems/net_cfg.h,v 1.1.1.1 2001/06/21 06:32:42 greg Exp $
 *
 * MODULE DESCRIPTION: This module specializes the RTEMS Network configuration 
 *                     for the omniORB examples. It could be used as a starting
 *                     point of an application using omniORB and RTEMS.
 *
 * This file was based on "networkconfig.h" that comes with the netdemos
 * examples that ships with the RTEMS distribution.
 *
 * NOTE: This file must be modified to match your environment.
 *
 * by: Rosimildo da Silva:
 *     rdasilva@connecttel.com
 *     http://www.connecttel.com
 *
 * MODIFICATION/HISTORY:
 *
 * $Log: net_cfg.h,v $
 * Revision 1.1.1.1  2001/06/21 06:32:42  greg
 * Microwindows pre8 with patches
 *
 * Revision 1.1.1.1  2001/06/05 03:44:03  root
 * First import of 5/5/2001 Microwindows to CVS
 *
 ****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*
#define RTEMS_USE_BOOTP
*/

#include <stdio.h>
#include <rtems/rtems_bsdnet.h>

/*
 * Define RTEMS_SET_ETHERNET_ADDRESS if you want to specify the
 * Ethernet address here.  If RTEMS_SET_ETHERNET_ADDRESS is not
 * defined the driver will choose an address.
 */
#define RTEMS_SET_ETHERNET_ADDRESS
#if (defined (RTEMS_SET_ETHERNET_ADDRESS))
static char ethernet_address[6] = { 0x20, 0x00, 0x27, 0xAF, 0x03, 0x51 };
#endif

#undef  RTEMS_BSP_NETWORK_DRIVER_NAME
#define RTEMS_BSP_NETWORK_DRIVER_NAME  "ep0"

extern int rtems_3c509_driver_attach( struct rtems_bsdnet_ifconfig *config );
#undef  RTEMS_BSP_NETWORK_DRIVER_ATTACH
#define RTEMS_BSP_NETWORK_DRIVER_ATTACH rtems_3c509_driver_attach

/*
 * Default network interface
 */
static struct rtems_bsdnet_ifconfig netdriver_config = 
{
   RTEMS_BSP_NETWORK_DRIVER_NAME,      /* name */
	RTEMS_BSP_NETWORK_DRIVER_ATTACH,    /* attach function */

	NULL,				/* link to next interface */

#if (defined (RTEMS_USE_BOOTP))
	NULL,				/* BOOTP supplies IP address */
	NULL,				/* BOOTP supplies IP net mask */
#else
	"192.168.0.11",		/* IP address */
	"255.255.255.0",		/* IP net mask */
#endif /* !RTEMS_USE_BOOTP */

#if (defined (RTEMS_SET_ETHERNET_ADDRESS))
	ethernet_address,               /* Ethernet hardware address */
#else
	NULL,           /* Driver supplies hardware address */
#endif
	0				/* Use default driver parameters */
};

/*
 * Network configuration
 */
struct rtems_bsdnet_config rtems_bsdnet_config = {
	&netdriver_config,

#if (defined (RTEMS_USE_BOOTP))
	rtems_bsdnet_do_bootp,
#else
	NULL,
#endif

	0,			/* Default network task priority */
	0,			/* Default mbuf capacity */
	0,			/* Default mbuf cluster capacity */

#if (!defined (RTEMS_USE_BOOTP))
	"lucila",			/* Host name */
	"rps.com",			/* Domain name */
	"192.168.0.1",	/* Gateway */
	"192.168.0.1",	/* Log host */
	{"192.168.0.1" },	/* Name server(s) */
#endif /* !RTEMS_USE_BOOTP */

};

/*
 * For TFTP test application
 */
#if (!defined (RTEMS_USE_BOOTP))
#define RTEMS_TFTP_TEST_HOST_NAME "192.168.0.2"
#define RTEMS_TFTP_TEST_FILE_NAME "root/boot.bt"
#endif


#ifdef __cplusplus
}
#endif
 
/* end of include file */
