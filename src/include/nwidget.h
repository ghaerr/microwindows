/*
 * NanoWidgets v0.1
 * (C) 1999 Screen Media AS
 * 
 * Written by Vidar Hokstad
 * 
 * Contains code from The Nano Toolkit,
 * (C) 1999 by Alexander Peuchert.
 * 
 * In theory, only the widget class should depend on the underlying windowing
 * system. In practice, bitmap formats etc. may also end up being system
 * specific, though.
 */

#ifndef __NWIDGET_H
#define __NWIDGET_H

/* Define the instance structure */

DEFINE_NOBJECT(widget,object)
   struct widget_nobject * parent;
   struct widget_nobject * sibling;
   struct widget_nobject * children;     /* All widgets can be containers, but not all are well suited */

   int x,y,w,h;
   int shown;
   int infocus;        /* Is this widget in focus? That is, is it the last one that has been pressed?
			* The widget set keeps track of this via the static variable "infocusob"
			*/

   int id;             /* Nano X window id */
   GR_GC_ID gc;        /* Graphics contexts. Defaults to a shared GC */

   NOBJECT * renderob; /* This object (of class "renderer" is used for most rendering, and
			* make widgets "themeable". Widgets by default inherit the renderer of their parent
			*/
   NOBJECT * layout;    /* This object is called to do layout (by attach, resize etc.). FIXME: Currently not used. */
END_NOBJECT

/* Define class wide data and method slots */

DEFINE_NCLASS(widget,object)
   NSLOT(int,init);         // My init function.

   NSLOT(void,attach);      /* Attach the widget argument as a child of this widget */
   NSLOT(void,show);        /* Set this widget drawable. Will repaint widget */
   NSLOT(void,hide);        /* Set this widget hidden. Will repaint parent. */
   NSLOT(void,showall);     /* show() this wiget, and showAll() its children. */
   NSLOT(void,hideall);     /* hide() this widget, and hideAll() its children. */
   NSLOT(void,repaint);     /* Called to let widget (re)paint itself on the screen */
   NSLOT(void,resize);      /* Set width and height */
   NSLOT(void,move);        /* Move to a specific position. */
   NSLOT(int,ishit);        /* Returns NWTRUE if widget is "hit" by the */
                            /* (x,y) pair given as args, NWFALSE else. */
   NSLOT(void,mousemove);   /* Mouse moved while this widget was active. */
   NSLOT(void,buttondown);  /* Change in mouse button status */
   NSLOT(void,buttonup);    /* Button was released outside widget it was pressed in */
   NSLOT(void,clicked);     /* Button was pressed, *AND* released over the target area. */

   NSLOT(void,keypress);    /* Key press event. */

   NSLOT(struct render_nobject *,getrenderob);  /* Get rendering object */
   NSLOT(MWCOLORVAL,getrendercol);  /* Get color to use to render something yourself */

   NSLOT(void,fillrect);    /* Fill a rectangle */
   NSLOT(void,rect);        /* Draw a rectangle */
   NSLOT(void,line);        /* Draw a line */
   NSLOT(void,setfg);       /* Change the foreground color of the current GC */
   NSLOT(void,setbg);       /* Change the background color of the current GC */
   NSLOT(void,setmode);     /* Change drawing mode */
   NSLOT(void,getgeometry); /* Fill in integer pointers with geometry */

   NSLOT(void,textextent);  /* Get the extent (width, height, baseline) of a string for this widgets GC/font */
   NSLOT(void,text);        /* Draw text */

   NSLOT(int,isinfocus);    /* Is this widget currently in focus? */
   NSLOT(void,setfocus);    /* Set focus to this widget */
   NSLOT(void,leavefocus);  /* This widget is not in focus anymore */
END_NCLASS

/* Define macros to simplify method calling */

#define n_widget_init(__this__,__parent__) n_call(widget,init,__this__,(__this__,__parent__))
#define n_widget_attach(__this__,__child__) n_call(widget,attach,__this__,(__this__,__child__))
#define n_widget_show(__this__) n_call(widget,show,__this__,(__this__))
#define n_widget_hide(__this__) n_call(widget,hide,__this__,(__this__))
#define n_widget_repaint(__this__) n_call(widget,repaint,__this__,(__this__))
#define n_widget_fillrect(__this__,x,y,w,h) n_call(widget,fillrect,__this__,(__this__,(x),(y),(w),(h)))
#define n_widget_rect(__this__,x,y,w,h) n_call(widget,rect,__this__,(__this__,(x),(y),(w),(h)))
#define n_widget_line(__this__,x1,y1,x2,y2) n_call(widget,line,__this__,(__this__,(x1),(y1),(x2),(y2)))
#define n_widget_setfg(__this__,c) n_call(widget,setfg,__this__,(__this__,(c)))
#define n_widget_setmode(__this__,c) n_call(widget,setmode,__this__,(__this__,(c)))
#define n_widget_setbg(__this__,c) n_call(widget,setbg,__this__,(__this__,(c)))
#define n_widget_move(__this__,x,y) n_call(widget,move,__this__,(__this__,(x),(y)))
#define n_widget_resize(__this__,w,h) n_call(widget,resize,__this__,(__this__,(w),(h)))
#define n_widget_getgeometry(__this__,x,y,w,h) n_call(widget,getgeometry,__this__,(__this__,(x),(y),(w),(h)))
#define n_widget_buttondown(__this__,__x__,__y__,__button__) n_call(widget,buttondown,__this__,(__this__,__x__,__y__,__button__))
#define n_widget_buttonup(__this__,__x__,__y__,__button__) n_call(widget,buttonup,__this__,(__this__,__x__,__y__,__button__))
#define n_widget_keypress(__this__,__key__,__mod__,__b__) n_call(widget,keypress,__this__,(__this__,__key__,__mod__,__b__)) 
#define n_widget_mousemove(__this__,__x__,__y__,__button__) n_call(widget,mousemove,__this__,(__this__,__x__,__y__,__button__))
#define n_widget_clicked(__this__,__x__,__y__,__button__) n_call(widget,clicked,__this__,(__this__,__x__,__y__,__button__))
#define n_widget_text(__this__,__x__,__y__,__text__,__count__) n_call(widget,text,__this__,(__this__,__x__,__y__,__text__,__count__))
#define n_widget_textextent(__this__,__text__,__count__,__retw__,__reth__,__retb__) n_call(widget,textextent,__this__,(__this__,__text__,__count__,__retw__,__reth__,__retb__))

#define n_widget_isinfocus(__this__) n_call(widget,isinfocus,__this__,(__this__))
#define n_widget_setfocus(__this__) n_call(widget,setfocus,__this__,(__this__))
#define n_widget_leavefocus(__this__) n_call(widget,leavefocus,__this__,(__this__))
#define n_widget_getrenderob(__this__) n_call(widget,getrenderob,__this__,(__this__))
#define n_widget_getrendercol(__this__,__c__) n_call(widget,getrendercol,__this__,(__this__,__c__))
#define n_main() {while(1)n_handle_event();}

/* Define a simpler name for the instance structure */
typedef struct widget_nobject NWIDGET;

void n_handle_event(void);		/* Handle a single event */
void n_init_widget_class(void);		/* Initialise the widget class */

#endif
