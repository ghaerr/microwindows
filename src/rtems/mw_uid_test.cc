/*
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <rtems/mw_uid.h>
#include <rtems/kd.h>

static const char *Q_NAME       = "MWQ";
#define            Q_MAX_MSGS   128


/*
 * "Select() routine called by the MicroWindows framework to receive events 
 * from the input devices.
 */
void GsSelect(void)
{
  struct MW_UID_MESSAGE m;
  int rc;

  /* let's make sure that the type is invalid */
  m.type = MV_UID_INVALID;

  /* wait up to 100 milisecons for events */
  rc = uid_read_message( &m, 100 );

  /* return if timed-out or something went wrong */
  if( rc < 0 )
  {
     if( errno != ETIMEDOUT )
        printf( " rc= %d, errno=%d\n", rc, errno );
     return;
  }
  /* let's pass the event up to microwindows */
  switch( m.type )
  {
    /* Mouse or Touch Screen event */
    case MV_UID_REL_POS:
    case MV_UID_ABS_POS:
        printf( "Mouse: btns=%X, dx=%d, dy=%d, dz=%d\n",  
                  m.m.pos.btns, m.m.pos.x, m.m.pos.y, m.m.pos.z );
        break;


    /* KBD event */
    case MV_UID_KBD:
        printf( "Kbd: code=%X, modifiers=%X, mode=%d\n",  
                 (unsigned char )m.m.kbd.code, m.m.kbd.modifiers, m.m.kbd.mode );
        break;

    /* micro-windows does nothing with those.. */
    case MV_UID_TIMER:
    case MV_UID_INVALID:
    default:
       ;
  }
}

extern "C" int close( int );
extern "C" int rtems_main(int argc, char **argv)
{
  int status;
  printf( "Starting untar file.\n" ); 
  struct MW_UID_MESSAGE m;

  int rc;
  /* if this is the first time around, create the message queue */
  rc = uid_open_queue( Q_NAME, O_CREAT | O_RDWR, Q_MAX_MSGS );

  printf( "Open QUEUE=%X\n", rc );

  int mouse_fd = open( "/dev/mouse", O_NONBLOCK );
  uid_register_device( mouse_fd, Q_NAME );

  /* kbd it is already opened */
  int kbd_fd = fileno( stdin );
  uid_register_device( kbd_fd, Q_NAME );

  int old_mode = 0;
  /* set keyboard to scanmode */
  rc = uid_set_kbd_mode( kbd_fd, MV_KEY_MODE_SCANCODE, &old_mode );
  printf( "Kbd OldMode=%X, rc=%d\n", old_mode, rc );
  while( TRUE )
  {
     GsSelect();
  }
  uid_unregister_device( kbd_fd  );
  uid_unregister_device( mouse_fd );
  rc = uid_close_queue();
  printf( "Close QUEUE=%X\n", rc );

  close( mouse_fd );

  fprintf(stdout, "About to leave main()\n" ); 
  return 0;
}
