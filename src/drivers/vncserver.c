/*
 *  Remote framebuffer (VNC) server support for nano-X. 
 *  Copyright (C) 2002 Giampiero Giancipoli <gianci@freemail.it> 
 *
 *  Derived from libVNCserver example
 *  Copyright (C) 2001 Johannes E. Schindelin <Johannes.Schindelin@gmx.de>
 */

#ifdef WIN32
#define sleep Sleep
#else
#include <unistd.h>
#endif

#ifdef __IRIX__
#include <netdb.h>
#endif

#include "nano-X.h"
#include "device.h"
#include "genmem.h"

void GsHandleMouseStatus(GR_COORD, GR_COORD, int newbuttons);
void GsDeliverKeyboardEvent(GR_WINDOW_ID, GR_EVENT_TYPE, GR_KEY, GR_KEYMOD, GR_SCANCODE);
void GsTerminate(void);

#if VNCSERVER_PTHREADED
#include <pthread.h>

pthread_mutex_t eventMutex; /* mutex for events allocation */

#include <unistd.h>

/* pipe pair */                
static int     fd[2];  

/* vnc_thread_fd is fd[0]. It is added to the GsSelect fdset in srvmain.c.
   When a VNC event arrives, we put something into the pipe, waking up the select()  */

int     vnc_thread_fd; 
pthread_t       httpd_thread; /* httpd server process thread */

#endif
#include <rfb/rfb.h>
#include <rfb/keysym.h>
#include <assert.h>

extern MWPALENTRY gr_palette[256];    /* current palette*/

rfbScreenInfoPtr rfbScreen; /* the RFB screen structure */

static int     clients_connected = 0 ; /* # of clients connected */
static PSD     actualpsd; /* The actual PSD */
/* These are the true low level functions, which are called by their matching VNC stub */
static void 	 (*_DrawPixel)(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c);

static void 	 (*_DrawHorzLine)(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y,
                MWPIXELVAL c);
static void	 (*_DrawVertLine)(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2,
                MWPIXELVAL c);
static void	 (*_FillRect)(PSD psd,MWCOORD x1,MWCOORD y1,MWCOORD x2,
                MWCOORD y2,MWPIXELVAL c);
static void	 (*_Blit)(PSD destpsd, MWCOORD destx, MWCOORD desty, MWCOORD w,
                MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);

static void UndrawCursor(void)
{
        //FIXME if ( clients_connected && rfbScreen->cursorIsDrawn ) 
                //FIXME rfbUndrawCursor(rfbScreen);
}       

static void MarkRect( int x1, int y1, int x2, int y2 )
{
        if ( clients_connected )
        {
                /* Enlarge  the updated rectangle by a pixel, needed for 1 pixel wide rects */
                if ( x2 < (rfbScreen->width - 1)) 
                        x2++;
                else if ( x1 > 0 ) x1--;
                        
                if ( y2 < (rfbScreen->height - 1)) 
                        y2++;
                else if ( y1 > 0 ) y1--;

                rfbMarkRectAsModified(rfbScreen, x1, y1, x2, y2);
        }
}

static void stubDrawPixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
        UndrawCursor();
        _DrawPixel(psd, x, y, c);
        MarkRect(x, y, x, y);
}


#if 0
static void stubDrawArea(PSD psd, driver_gc_t *gc);
{
        UndrawCursor();
        _DrawArea(psd,gc);
        MarkRect(gc->dstx, gc->dsty, gc->dstx + gc->dstw, gc->dstx + gc->dstw ); 
}
#endif

static void stubFillRect(PSD psd, MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
        UndrawCursor();
        _FillRect(psd, x1, y1, x2, y2, c);
        MarkRect(x1, y1, x2, y2);   
}

static void stubDrawHorzLine(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y,
                MWPIXELVAL c)
{
        UndrawCursor();
        _DrawHorzLine(psd, x1, x2, y, c);
        MarkRect( x1, y, x2, y);
}

static void stubDrawVertLine(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2,
                MWPIXELVAL c)
{
        UndrawCursor();
        _DrawVertLine(psd,x, y1, y2, c);
        MarkRect( x, y1, x, y2 );
}

static void stubBlit( PSD destpsd, MWCOORD destx, MWCOORD desty, MWCOORD w,
                MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op )
{
        UndrawCursor();
        _Blit( destpsd, destx, desty, w,  h, srcpsd, srcx, srcy, op );
        if ( destpsd == actualpsd )
                MarkRect( destx, desty, destx + w, desty + h );

}

#if 0
static void stubStretchBlitEx(PSD dstpsd, PSD srcpsd, MWCOORD dest_x_start, int dest_y_start,
	MWCOORD width, int height, int x_denominator, int y_denominator,
	int src_x_fraction, int src_y_fraction,
	int x_step_fraction, int y_step_fraction, long op)
{
        UndrawCursor();
        _StretchBlitEx( dstpsd, srcpsd, dest_x_start, dest_y_start, width, height,
			x_denominator, y_denominator, src_x_fraction, src_y_fraction,
			x_step_fraction, y_step_fraction, op);
        if ( dstpsd == actualpsd )
                MarkRect( dest_x_start, dest_y_start, dest_x_start + width, dest_y_start + height );
}
#endif

static void clientgone(rfbClientPtr cl)
{
        clients_connected--;
}

//static enum rfbNewClientAction newclient(rfbClientPtr cl)
static void newclient( rfbClientPtr cl )
{
        clients_connected++;
        cl->clientGoneHook = clientgone;

        UndrawCursor();
#if 0
        MyStretchBlit( vncpsd, 0, 0, rfbScreen->width, rfbScreen->height, actualpsd, 0, 0, 
                       XRES, YRES, PSDOP_COPY );
#endif
        MarkRect( 0, 0, rfbScreen->width, rfbScreen->height );
//	return RFB_CLIENT_ACCEPT;

}


#if VNCSERVER_PTHREADED

static inline void awakeGsSelect()
{
        int dummy = 0xF0CACC1A;
        /* The focaccia is, beside pesto, another delicious food from Genova ;) */ 
        /*  write something to pipe, awaking GsSelect() */
        write(fd[1], &dummy, sizeof(int));

}

/*  httpd server thread */
void * httpd_proc( void * _ )
{

    int nfds;
    fd_set fds;


    if (!rfbScreen->httpDir)
        return 0;

    do  {
            FD_ZERO(&fds);
            FD_SET(rfbScreen->httpListenSock, &fds);
            if (rfbScreen->httpSock >= 0) {
                FD_SET(rfbScreen->httpSock, &fds);
            }
            nfds = select( max(rfbScreen->httpSock,rfbScreen->httpListenSock) + 1, &fds, NULL, NULL, NULL );
        
            rfbHttpCheckFds( rfbScreen );
    } while ( nfds > 0 );
    return 0;
}

#else
#define awakeGsSelect()
#endif

/* VNC Pointer events */
static void handle_pointer(int buttonMask,int x,int y, struct _rfbClientRec* cl)
{

        UndrawCursor();

        if ( x >= 0 && y >= 0 && x < rfbScreen->width && y < rfbScreen->height )
        {
               int newbuttons = 0;

               if ( buttonMask )
                      newbuttons = MWBUTTON_L;

               GsHandleMouseStatus( x, y, newbuttons );        

               awakeGsSelect();

        }
        rfbDefaultPtrAddEvent(buttonMask, x, y, cl);

}

/* VNC Keyboard events, almost entirely borrowed from kbd_x11.c X11_Read().  */
static void handle_keyboard( rfbBool down, rfbKeySym sym, rfbClientPtr cl)
{
        static MWKEYMOD key_modstate = 0; /* latest modifiers */
        MWKEY           mwkey;


        switch ( sym ) {
        case XK_Escape:
                mwkey = MWKEY_ESCAPE;
                break;

        case XK_Delete:
                mwkey = MWKEY_DELETE;
                break;
        case XK_Home:
                mwkey = MWKEY_HOME;
                break;
        case XK_Left:
                mwkey = MWKEY_LEFT;
                break;
        case XK_Up:
                mwkey = MWKEY_UP;
                break;
        case XK_Right:
                mwkey = MWKEY_RIGHT;
                break;
        case XK_Down:
                mwkey = MWKEY_DOWN;
                break;
        case XK_Page_Up:
                mwkey = MWKEY_PAGEUP;
                break;
        case XK_Page_Down:
                mwkey = MWKEY_PAGEDOWN;
                break;
        case XK_End:
                mwkey = MWKEY_END;
                break;
        case XK_Insert:
                mwkey = MWKEY_INSERT;
                break;
        case XK_Pause:
        case XK_Break:
                mwkey = MWKEY_QUIT;
                GsTerminate();
                break;
        case XK_Print:
        case XK_Sys_Req:
                mwkey = MWKEY_PRINT;
                break;
        case XK_Menu:
                mwkey = MWKEY_MENU;
                break;
        case XK_Cancel:
                mwkey = MWKEY_CANCEL;
                break;
        case XK_KP_Enter:
                mwkey = MWKEY_KP_ENTER;
                break;
        case XK_KP_Home:
                mwkey = MWKEY_KP7;
                break;
        case XK_KP_Left:
                mwkey = MWKEY_KP4;
                break;
        case XK_KP_Up:
                mwkey = MWKEY_KP8;
                break;
        case XK_KP_Right:
                mwkey = MWKEY_KP6;
                break;
        case XK_KP_Down:
                mwkey = MWKEY_KP2;
                break;
        case XK_KP_Page_Up:
                mwkey = MWKEY_KP9;
                break;
        case XK_KP_Page_Down:
                mwkey = MWKEY_KP3;
                break;
        case XK_KP_End:
                mwkey = MWKEY_KP1;
                break;
        case XK_KP_Insert:
                mwkey = MWKEY_KP0;
                break;
        case XK_KP_Delete:
                mwkey = MWKEY_KP_PERIOD;
                break;
        case XK_KP_Equal:
                mwkey = MWKEY_KP_EQUALS;
                break;
        case XK_KP_Multiply:
                mwkey = MWKEY_KP_MULTIPLY;
                break;
        case XK_KP_Add:
                mwkey = MWKEY_KP_PLUS;
                break;
        case XK_KP_Subtract:
                mwkey = MWKEY_KP_MINUS;
                break;
        case XK_KP_Decimal:
                mwkey = MWKEY_KP_PERIOD;
                break;
        case XK_KP_Divide:
                mwkey = MWKEY_KP_DIVIDE;
                break;
        case XK_KP_5:
        case XK_KP_Begin:
                mwkey = MWKEY_KP5;
                break;
        case XK_F1:
                mwkey = MWKEY_F1;
                break;
        case XK_F2:
                mwkey = MWKEY_F2;
                break;
        case XK_F3:
                mwkey = MWKEY_F3;
                break;
        case XK_F4:
                mwkey = MWKEY_F4;
                break;
        case XK_F5:
                mwkey = MWKEY_F5;
                break;
        case XK_F6:
                mwkey = MWKEY_F6;
                break;
        case XK_F7:
                mwkey = MWKEY_F7;
                break;
        case XK_F8:
                mwkey = MWKEY_F8;
                break;
        case XK_F9:
                mwkey = MWKEY_F9;
                break;
        case XK_F10:
                mwkey = MWKEY_F10;
                break;
        case XK_F11:
                mwkey = MWKEY_F11;
                break;
        case XK_F12:
                mwkey = MWKEY_F12;
                break;

                /* state modifiers*/
        case XK_Num_Lock:
                /* not sent, used only for state*/
                if ( down )
                        key_modstate ^= MWKMOD_NUM;
                return ;
        case XK_Shift_Lock:
        case XK_Caps_Lock:
                /* not sent, used only for state*/
                if ( down )
                        key_modstate ^= MWKMOD_CAPS;
                return ;
        case XK_Scroll_Lock:
                /* not sent, used only for state*/
                if ( down )
                        key_modstate ^= MWKMOD_SCR;
                return ;
                break;
        case XK_Shift_L:
                mwkey = MWKEY_LSHIFT;
                break;
        case XK_Shift_R:
                mwkey = MWKEY_RSHIFT;
                break;
        case XK_Control_L:
                mwkey = MWKEY_LCTRL;

        case XK_Control_R:
                mwkey = MWKEY_RCTRL;

                break;
        case XK_Alt_L:
                mwkey = MWKEY_LALT;

        case XK_Alt_R:
                mwkey = MWKEY_RALT;
                break;
        
        case XK_Meta_L:
        case XK_Super_L:
        case XK_Hyper_L:
                mwkey = MWKEY_LMETA;
                break;
        
        case XK_Meta_R:
        case XK_Super_R:
        case XK_Hyper_R:
                mwkey = MWKEY_RMETA;
        default:
                switch ( sym ) {
                case XK_BackSpace:
                case XK_Tab:
                case XK_Return:
                        break;
                default:
                        if ( sym & 0xFF00 )
                                EPRINTF("Unhandled VNC keysym: %04x\n", (int)sym);
                }

                if ( key_modstate & MWKMOD_CTRL )
                        mwkey = sym & 0x1f;     /* Control code */
                else
                        mwkey = sym & 0xff;     /* ASCII*/

                break;

        }

        switch ( mwkey ) {
        case MWKEY_LCTRL:
        case MWKEY_RCTRL:
                if ( down )
                        key_modstate ^= MWKMOD_CTRL;
                else
                        key_modstate &= ~MWKMOD_CTRL;
                break;
        case MWKEY_RALT:
        case MWKEY_LALT:
                if ( down )
                        key_modstate ^= MWKMOD_ALT;
                else
                        key_modstate &= ~MWKMOD_ALT;
                break;
        case MWKEY_RMETA:
        case MWKEY_LMETA:
                if ( down )
                        key_modstate ^= MWKMOD_META;
                else
                        key_modstate &= ~MWKMOD_META;
                break;
        case MWKEY_RSHIFT:
        case MWKEY_LSHIFT:
                if ( down )
                        key_modstate ^= MWKMOD_SHIFT;
                else
                        key_modstate &= ~MWKMOD_SHIFT;
                break;
        default:
                break;
        
        }

        if ( key_modstate & MWKMOD_NUM ) {
                switch ( mwkey ) {
                case MWKEY_KP0:
                case MWKEY_KP1:
                case MWKEY_KP2:
                case MWKEY_KP3:
                case MWKEY_KP4:
                case MWKEY_KP5:
                case MWKEY_KP6:
                case MWKEY_KP7:
                case MWKEY_KP8:
                case MWKEY_KP9:
                        mwkey = mwkey - MWKEY_KP0 + '0';
                        break;
                case MWKEY_KP_PERIOD:
                        mwkey = '.';
                        break;
                case MWKEY_KP_DIVIDE:
                        mwkey = '/';
                        break;
                case MWKEY_KP_MULTIPLY:
                        mwkey = '*';
                        break;
                case MWKEY_KP_MINUS:
                        mwkey = '-';
                        break;
                case MWKEY_KP_PLUS:
                        mwkey = '+';
                        break;
                case MWKEY_KP_ENTER:
                        mwkey = MWKEY_ENTER;
                        break;
                case MWKEY_KP_EQUALS:
                        mwkey = '-';
                        break;
                default:
                        break;
                }
        }

        GsDeliverKeyboardEvent(0,(down? GR_EVENT_TYPE_KEY_DOWN: GR_EVENT_TYPE_KEY_UP),
			mwkey, key_modstate, 0 );

        awakeGsSelect();


        DPRINTF( "Key %d (0x%x) %s %s %s %s \"%c\" %s\n", sym, sym,
                 key_modstate & MWKMOD_ALT? "ALT": "",
                 key_modstate & MWKMOD_CTRL? "CTRL": "",
                 key_modstate & MWKMOD_SHIFT? "SHIFT": "",
                 key_modstate & MWKMOD_META? "META": "",
                sym & 0xff, down? "pressed": "released" );


}

/* Initialization */


int GdOpenVNC( PSD psd, int argc, char *argv[] )
{
  rfbColourMap* cmap;
  int i;

   /* Save the actual FB screen driver drawing functions */
        
   actualpsd = psd;

   _DrawPixel = psd->DrawPixel;
   _DrawHorzLine = psd->DrawHorzLine;
   _DrawVertLine = psd->DrawVertLine;
   _FillRect = psd->FillRect;
//   _Blit = psd->Blit;
//   _DrawArea = psd->DrawArea;
//   _StretchBlitEx = psd->StretchBlitEx;
   

   /* Set the screen driver drawing functions to vnc stubs */
        
   psd->DrawPixel = stubDrawPixel;
   psd->DrawHorzLine = stubDrawHorzLine;
   psd->DrawVertLine = stubDrawVertLine;
   psd->FillRect = stubFillRect;
//   psd->Blit = stubBlit;
//   psd->DrawArea = stubDrawArea;
//   psd->StretchBlit = stubStretchBlit;
                        
   /* Don't set bits x sample & samples x pixel, we'll do it later  */
   rfbScreen = rfbGetScreen(&argc,argv,psd->xres,psd->yres,-1,-1,-1);
   if ( rfbScreen == NULL ) {
           EPRINTF( "Could not allocate RFB Screen!\n" );
           exit(-1);
   }

   /* NOW set bits x sample & samples x pixel */
        
   switch(psd->pixtype) {

   case  MWPF_TRUECOLOR332: /* 8 bpp */
           rfbScreen->serverFormat.redMax = 7;
           rfbScreen->serverFormat.greenMax = 7;
           rfbScreen->serverFormat.blueMax = 3;

           rfbScreen->serverFormat.redShift = 5;
           rfbScreen->serverFormat.greenShift = 2;
           rfbScreen->serverFormat.blueShift = 0;
           break;

   case  MWPF_TRUECOLOR565: 
           rfbScreen->serverFormat.greenMax = 63;
           rfbScreen->serverFormat.redShift = 11;
           goto skip555;

   case  MWPF_TRUECOLOR555: /* 16 bpp */
           rfbScreen->serverFormat.greenMax = 31;
           rfbScreen->serverFormat.redShift = 10;
                
           skip555:
           rfbScreen->serverFormat.redMax = 31;
           rfbScreen->serverFormat.blueMax = 31;
           
           rfbScreen->serverFormat.greenShift = 5;
           rfbScreen->serverFormat.blueShift = 0;

           break;

   case MWPF_TRUECOLOR1555:
           rfbScreen->serverFormat.greenMax = 31;
           rfbScreen->serverFormat.redMax = 31;
           rfbScreen->serverFormat.blueMax = 31;

           rfbScreen->serverFormat.redShift = 0;
           rfbScreen->serverFormat.greenShift = 5;
           rfbScreen->serverFormat.blueShift = 10;

           break;

   case MWPF_PALETTE:
     cmap = &(rfbScreen->colourMap);
     rfbScreen->serverFormat.trueColour = FALSE;
     cmap->count = psd->ncolors;
     cmap->is16 = FALSE;

     if (cmap->data.bytes == 0) {
       cmap->data.bytes=malloc(cmap->count*3);
     }

     for (i=0; i<cmap->count; i++) {
       cmap->data.bytes[3*i+0] = gr_palette[i].r;
       cmap->data.bytes[3*i+1] = gr_palette[i].g;
       cmap->data.bytes[3*i+2] = gr_palette[i].b;
     }
     /* Fall through to MWPF_TRUECOLOR888 */

   case MWPF_TRUECOLOR8888: /* 24/32 bpp */
   case MWPF_TRUECOLOR888:
           rfbScreen->serverFormat.redMax = 255;
           rfbScreen->serverFormat.greenMax = 255;
           rfbScreen->serverFormat.blueMax = 255;

           rfbScreen->serverFormat.redShift = 16;
           rfbScreen->serverFormat.greenShift = 8;
           rfbScreen->serverFormat.blueShift = 0;
                
           break;
        
   default:
     break;
   }

   /* Set bpp. If VNC does not support nano-X bpp, it will refuse connections, 
      but nano-X will continue to run  */

   rfbScreen->serverFormat.bitsPerPixel = 
   rfbScreen->serverFormat.depth = 
   rfbScreen->bitsPerPixel = rfbScreen->depth = psd->bpp;


   rfbScreen->desktopName = "nano-X";
   rfbScreen->frameBuffer = psd->addr;
   rfbScreen->alwaysShared = TRUE;
   rfbScreen->ptrAddEvent = handle_pointer;
   rfbScreen->kbdAddEvent = handle_keyboard;
   rfbScreen->newClientHook = newclient;
   // FIXME rfbScreen->dontSendFramebufferUpdate = FALSE;
   rfbScreen->cursor = NULL;

   rfbScreen->httpPort = 5800;
   rfbScreen->httpDir = "/var/lib/httpd/";
   rfbScreen->authPasswdData = "/etc/vncpasswd";

   //rfbScreen->paddedWidthInBytes = psd->linelen * (psd->bpp >> 3) ; 
   rfbScreen->paddedWidthInBytes = psd->pitch;
        
   /* initialize the server */
   rfbInitServer(rfbScreen);

#if VNCSERVER_PTHREADED
   pipe(fd);
   vnc_thread_fd = fd[0];
   pthread_mutex_init( &eventMutex, NULL );
   rfbScreen->deferUpdateTime = 25;
//   rfbScreen->backgroundLoop = TRUE;
   rfbRunEventLoop( rfbScreen, 0, TRUE);
   pthread_create( &httpd_thread, 0, httpd_proc, 0 );

#endif
   return 1;
}

void GdCloseVNC( void )
{

   rfbScreenCleanup( rfbScreen );
 #if VNCSERVER_PTHREADED
   close(fd[0]);
   close(fd[1]);
   pthread_mutex_destroy( &eventMutex );
#endif
   clients_connected = 0;
}
