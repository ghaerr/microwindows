/*
 * NanoWidgets v0.1
 * (C) 1999 Screen Media AS
 * 
 * Written by Vidar Hokstad
 * 
 * Contains code from The Nano Toolkit,
 * (C) 1999 by Alexander Peuchert.
 *
 */

#ifndef __NSLIDER_H
#define __NSLIDER_H

DEFINE_NOBJECT(slider,widget)
   int pressed;
   int freedom;     /* 1 = horizontal, 2 = vertical, 4 = both */

   /* Relative extent of the slider */
   long rel_maxw;  
   long rel_maxh;

   /* Relative dimensions and position */
   long rel_w;
   long rel_h;
   long rel_x;
   long rel_y;

   /* X position to start move from */
   long ox,oy;

   /* Callback used when the slider is being moved */
   void (* move_handler) (struct slider_nobject *, unsigned int);
END_NOBJECT

#define NSLIDER_FREEDOM_HORIZONTAL (1)
#define NSLIDER_FREEDOM_VERTICAL (2)

typedef struct slider_nobject NSLIDER;

DEFINE_NCLASS(slider,widget)
    NSLOT(int,init);
END_NCLASS

#define n_slider_init(__this__,__parent__) n_call(slider,init,__this__,(__this__,__parent__))

void n_init_slider_class(void);		/* Initialise slider class */

#endif
