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

#ifndef __NLISTVIEW_H
#define __NLISTVIEW_H

DEFINE_NOBJECT(listview,widget)
    const char ** entries;

    int numentries;
    int maxentries;  /* Maximum number of entries in listbox. If this is exceeded, we allocated a new array. */
    int topentry;    /* Entry shown at the top of the listbox. */
    int selected;    /* Number of selected list entry. -1 if none */
END_NOBJECT

DEFINE_NCLASS(listview,widget)
    NSLOT(void,init);
    NSLOT(void,setselected);
    NSLOT(int,addentries);
    NSLOT(int,addentry);
    NSLOT(const char * ,getselected);
END_NCLASS

#define n_listview_init(__this__,__parent__,__text__) n_call(listview,init,__this__,(__this__,__parent__,__text__))

typedef struct listview_nobject NLISTVIEW;

void n_init_listview_class(void);	/* Initialise listview class */

#endif
