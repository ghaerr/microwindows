/*
 * NanoWidgets v0.2
 * (C) 1999 Screen Media AS
 * 
 * Written by Vidar Hokstad
 * 
 * Contains code from The Nano Toolkit,
 * (C) 1999 by Alexander Peuchert.
 *
 */

#ifndef __NRENDER_H
#define __NRENDER_H

enum {
   RCOL_WIDGET_BACKGROUND,
   RCOL_WIDGET_TEXT,
   RCOL_WIDGET_TEXTBACKGROUND,
   RCOL_WIDGET_LIGHT,
   RCOL_WIDGET_MEDIUM,
   RCOL_WIDGET_DARK,
   RCOL_HIGHLIGHTED,
   RCOL_CURSOR,
   RCOL_MAXCOL
};

DEFINE_NOBJECT(render,object)
   MWCOLORVAL colors[RCOL_MAXCOL];
END_NOBJECT

DEFINE_NCLASS(render,object)
    NSLOT(int,init);
    NSLOT(void,border);               /* Draw a pressed or unpressed border, typically for buttons etc. */
    NSLOT(void,panel);                /* Draw a pressed or unpressed panel, with surrounding border */
    NSLOT(void,widgetbackground);     /* Draw a pressed or unpressed widget background. How the background is
				       * rendered is undefined. The background is assumed to be drawn before
				       * the border, and before any "inner parts" of the widget is drawn
				       */
    NSLOT(MWCOLORVAL,getcolor);
END_NCLASS

#define n_render_init(__this__) n_call(render,init,__this__,(__this__))
#define n_render_getcolor(__this__,__col__) n_call(render,getcolor,__this__,(__this__,__col__))
#define n_render_border(__this__,widget,x,y,w,h,pressed) n_call(render,border,__this__,(__this__,widget,x,y,w,h,pressed))
#define n_render_panel(__this__,widget,x,y,w,h,pressed) n_call(render,panel,__this__,(__this__,widget,x,y,w,h,pressed))

typedef struct render_nobject NRENDER;

void n_init_render_class(void);		/* Initialise render class */

#endif
