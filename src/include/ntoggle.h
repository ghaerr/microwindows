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

#ifndef __NTOGGLE_H
#define __NTOGGLE_H

DEFINE_NOBJECT(toggle,widget)
   int pressed;
   int selected;
   void (* onchange_handler) (struct toggle_nobject *, int selected);
END_NOBJECT

DEFINE_NCLASS(toggle,widget)
   NSLOT(int,init);
   NSLOT(void,onchange);    /* Set an onchange handler */
   NSLOT(void,setstate);    /* Set state of the toggle */
   NSLOT(int,isselected);  /* Is this button selected? */
   NSLOT(void,paintstate);
END_NCLASS

#define n_toggle_init(__this__,__parent__) n_call(toggle,init,__this__,(__this__,__parent__))
#define n_toggle_onchange(__this__,__handler__) n_call(toggle,onchange,__this__,(__this__,__handler__))
#define n_toggle_setstate(__this__,__state__) n_call(toggle,setstate,__this__,(__this__,__state__))
#define n_toggle_isselected(__this__) n_call(toggle,isselected,__this__,(__this__))
#define n_toggle_paintstate(__this__,__state__) n_call(toggle,paintstate,__this__,(__this__,__state__))

/* Define a simpler name for the instance structure */
typedef struct toggle_nobject NTOGGLE;

void n_init_toggle_class(void);		/* Initialise toggle class */

#endif
