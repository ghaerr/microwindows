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

#ifndef __NTEXTFIELD_H
#define __NTEXTFIELD_H

DEFINE_NOBJECT(textfield,widget)
    char * textbuf;

    long maxsize;    /* Maximum number of characters */
    int curpos;      /* Current cursor position */
    int firstpos;    /* First visible character */
    long overwrite;  /* Overwrite if 1, insert if 0 */

    int esc;         /* 1 if currently processing an escape sequence */

    /* Handler to be called to verify input - Can be used to restrict input
     * to for instance integers, or hexadecimal, or whatever format you want
     */
    int (* verify_handler)(struct textfield_nobject *,char *);
END_NOBJECT

DEFINE_NCLASS(textfield,widget)
    NSLOT(int,init);
    NSLOT(void,settext);
    NSLOT(const char * ,gettext);
END_NCLASS

#define n_textfield_init(__this__,__parent__,__text__) n_call(textfield,init,__this__,(__this__,__parent__,__text__))

typedef struct textfield_nobject NTEXTFIELD;

void n_init_textfield_class(void);	/* Initialise textfield class */

#endif
