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

#ifndef __NRADIO_H
#define __NRADIO_H

DEFINE_NOBJECT(radio,toggle)
   struct radio_nobject * next_radio;
   struct radio_nobject * last_radio;
END_NOBJECT

DEFINE_NCLASS(radio,toggle)
   /* setstate modified to change state of entire ring (rest.selected == !this.selected */
   NSLOT(int,init);
   NSLOT(void,connect);    /* Connect another radio button to this radio button ring */
END_NCLASS

#define n_radio_init(__this__,__parent__) n_call(radio,init,__this__,(__this__,__parent__))
#define n_radio_connect(__this__,__radio__) n_call(radio,connect,__this__,(__this__,__radio__))

typedef struct radio_nobject NRADIO;

void n_init_radio_class(void);		/* Initialise radio class */

#endif
