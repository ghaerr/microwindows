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

#ifndef __NBUTTON_H
#define __NBUTTON_H

DEFINE_NOBJECT(button,widget)
   int pressed;
   char * text;
   void (*onclick_handler) (struct button_nobject *, unsigned int);
END_NOBJECT

DEFINE_NCLASS(button,widget)
   NSLOT(int,init);
   NSLOT(void,onclick);    /* Set an onclick handler */
END_NCLASS

#define n_button_init(__this__,__parent__,__text__) n_call(button,init,__this__,(__this__,__parent__,__text__))
#define n_button_onclick(__this__,__handler__) n_call(button,onclick,__this__,(__this__,__handler__))

/* Define a simpler name for the instance structure */
typedef struct button_nobject NBUTTON;

void n_init_button_class(void);		/* Initialise the button class */

#endif
