/* 
 *  hre.h:            API for Handwriting Recognition Engine on Unix
 *  Author:           James Kempf
 *  Created On:       Wed Oct 28 11:30:43 1992
 *  Last Modified By: James Kempf
 *  Last Modified On: Fri Sep 23 13:49:26 1994
 *  Update Count:     74
 *  Copyright (c) 1994 by Sun Microsystems Computer Company
 *  All rights reserved.
 *  
 *  Use and copying of this software and preparation of 
 *  derivative works based upon this software are permitted.
 *  Any distribution of this software or derivative works
 *  must comply with all applicable United States export control
 *  laws.
 *
 *  This software is made available as is, and Sun Microsystems
 *  Computer Company makes no warranty about the software, its
 *  performance, or its conformity to any specification
 */

#ifndef _HRE_H_

#define _HRE_H_

#include <sys/types.h>
#include <stdlib.h>
/*#include <libintl.h>*/

#ifdef ELX
typedef unsigned int wchar_t;
#endif

/* Scalar Type Definitions */

/*For better readibility.*/

#ifndef true

typedef u_char bool;

#define true 1
#define false 0

#endif

/*For pointers to extra functions on recognizer.*/

typedef void (*rec_fn)();

/*
 *rec_confidence is an integer between 0-100 giving the confidence of the
 * recognizer in a particular result.
*/

typedef u_char rec_confidence;

/*Time value. This is the same as in X.h, so we conditionally define.*/
/* ari -- no it's not.  *SIGH*  there's an ifdef in X.h specifically */
/* for osf.  */
#ifndef X_H

#ifndef __osf__
typedef unsigned long Time;
#else
typedef unsigned int Time;
#endif
/* (mips) typedef unsigned long Time; */
/* (osf) typedef unsigned int Time; */

#endif

/**************** RECOGNIZER CONFIGURATION INFORMATION *******************/

/*
 * Recognizer information. Gives the locale, category of the character
 * set returned by the recognizer, and any subsets to which the
 * recognition can be limited. The locale and category should be
 * suitable for the setlocale(3). Those recognizers which don't do text
 * can simply report a blank locale and category, and report the
 * graphics types they recognize in the subset. 
*/

typedef struct {
    char* ri_locale;        /*The locale of the character set.*/
    char* ri_name;          /*Complete pathname to the recognizer.*/
    char** ri_subset;       /*Null terminated list of subsets supported*/
} rec_info;

/*These define a set of common character subset names.*/

#define GESTURE		"GESTURE"	/* gestures only */
#define MATHSET		"MATHSET"	/* %^*()_+={}<>,/. */
#define MONEYSET	"MONEYSET"	/* $, maybe cent, pound, and yen */
#define WHITESPACE	"WHITESPACE"	/* gaps are recognized as space */
#define KANJI_JIS1	"KANJI_JIS1"	/* the JIS1 kanji only */
#define KANJI_JIS1_PLUS	"KANJI_JIS1_PLUS" /* JIS1 plus some JIS2 */
#define KANJI_JIS2	"KANJI_JIS2"	/* the JIS1 + JIS2 kanji */
#define HIRIGANA	"HIRIGANA"	/* the hirigana */
#define KATAKANA	"KATAKANA"	/* the katakana */
#define UPPERCASE	"UPPERCASE"	/* upper case alphabetics, no digits */
#define LOWERCASE	"LOWERCASE"	/* lower case alphabetics, no digits */
#define DIGITS		"DIGITS"	/* digits 0-9 only */
#define PUNCTUATION	"PUNCTUATION"	/* \!-;'"?()&., */
#define NONALPHABETIC	"NONALPHABETIC" /* all nonalphabetics, no digits */
#define ASCII		"ASCII"		/* the ASCII character set */
#define ISO_LATIN12	"ISO_LATIN12"	/* The ISO Latin 12 characters */


/********************  RECOGNITION INPUT STRUCTURES ***********************/

/*
 * WINDOW SYSTEM INTERFACE
*/

/*Basic point. Note that it is identical to XTimeCoord, for easy conversion*/

typedef struct {
    Time time;
    short x, y;
} pen_point;

/*Bounding box. Structurally identical to XRectangle.*/

typedef struct {
    short x,y;                    /*Upper left corner.*/
    short width,height;           /*Width and height.*/
} pen_rect;    

/* Button flags - pen's button configuration. */

#define TABLET_TIP     0x1	/*tip switch*/
#define TABLET_BUTTON1 0x2	/*one barrel switch*/
#define TABLET_BUTTON2 0x4	/*two barrel switches*/
#define TABLET_BUTTON3 0x8	/*three barrel switches*/

/* Pen flags - additional state information that can be reported by the pen.*/

#define TABLET_PROXIMITY  0x1  /*can report position when pen not in contact*/
#define TABLET_RELATIVE   0x2	/*can report relative coords, like mouse*/
#define TABLET_ABSOLUTE	  0x4   /*can report absolute co-ordinates*/
#define TABLET_RANGE	  0x8   /*can report when pen goes out of range*/
#define TABLET_INVERT     0x10	/*can report when pen is inverted*/
#define TABLET_TOUCH	  0x20	/*finger can be used as pen*/

/* Angle flags - reporting of information about the pen angle. */

#define TABLET_ANGLEX  0x1	/*can report angle with the x axis*/
#define TABLET_ANGLEY  0x2	/*can report angle with the y axis*/
#define TABLET_ROTATE  0x4	/*can report barrel rotation*/

/* 
 * Sensor flags - configuration and reporting capabilities
*  of the tablet's sensor panel.
*/

#define TABLET_INTEGRATED     0x1     /*sensor panel is integrated with display*/
#define TABLET_PRESSURE       0x2     /*sensor panel can report pressure*/
#define TABLET_HEIGHT         0x4     /*sensor panel can report height*/

/* Units flags - in what units x and y coordinate data reported.*/

#define TABLET_DIMENSIONLESS 0x1	/*no units*/
#define TABLET_ENGLISH	     0x2	/*thousandths of an inch*/
#define TABLET_METRIC	     0x4	/*tenths of a millimeter*/

/* Origin flags - where the tablet's origin is located.*/

#define TABLET_ULEFT	0x1		/*upper left corner*/
#define TABLET_URIGHT	0x2		/*upper right corner*/
#define TABLET_LLEFT    0x4		/*lower left corner*/
#define TABLET_LRIGHT	0x8		/*lower right corner*/
#define TABLET_CENTER	0x10		/*center of tablet*/

/*
 * Tablet  capabilities structure. Defines basic information about tablet
 * configuration. 
*/

typedef struct {
	char tc_id[20];			   /*tablet identifier, null terminated*/
	u_short tc_button;      	   /*button capabilities*/
	u_short tc_pen;		       	   /*pen capabilities*/
	u_short tc_angle;         	   /*pen angle reporting*/
	u_int tc_sensor : 8;		   /*sensor capabilities*/
	u_int tc_units : 8;	   	   /*units for xy reporting*/
	u_int tc_default_units : 8; 	   /*default units*/
	u_int tc_origin : 8;	   	   /*where origin located*/
	short tc_x[2];			   /*minimum/maximum x*/
	short tc_y[2];			   /*minimum/maximum y*/
	short tc_pressure[2];		   /*minimum/maximum pressure/height*/
	u_int tc_sample_rate;		   /*rate of event reporting*/
	u_int tc_sample_distance;	   /*xy coords per sample*/

} tablet_cap;

/*
 * PEN STROKE DATA
*/

/*
 * Pen state parameters. "Basic" state is pen up/down, barrel buttons 
 * (if any), and in/out of range. Others may be reported by particular pens.
*/

typedef struct {
    u_short pt_button;      /*button state - same as tc_button*/
    u_short pt_pen;         /*other state - same as tc_pen*/
    short pt_pressure;      /*Pressure. + against tablet, - above tablet.*/
    double pt_anglex;       /*angle of tilt in the x direction, in radians.*/
    double pt_angley;       /*angle of tilt in the y direction, in radians.*/
    double pt_barrelrotate; /*angle of barrel rotation, in radians.*/
} pen_state;

/*
 * Stroke structure. 
*/

typedef struct {
    u_int ps_npts;                        /*Number of pen_point in array.*/
    pen_point* ps_pts;                    /*Array of points.*/
    u_int ps_nstate;			  /*Number of pen_state in array.*/
    u_int* ps_trans;			  /*State transition point indicies.*/
    pen_state* ps_state;                  /*Array of state.*/
} pen_stroke;

/*
 * RECOGNITION CONTEXT
*/

/* Structure for reporting writing area geometric constraints. */

typedef struct {
	pen_rect pr_area;
	short pr_row, pr_col;
	double pr_rowpitch, pr_colpitch;
} pen_frame; 

/*User preferences*/

#define REC_RIGHTH 0x0		/*Right-handed writer.*/
#define REC_LEFTH  0x1          /*Left-handed writer.*/

/*
 * Writing direction. There will generally be a preferred and a 
 * secondary direction (example: English is left to right, then
 * top to bottom). High byte has preferred, low byte secondary.
 * The recognizer can ignore this and key off of locale.
*/

#define REC_DEFAULT           0x0         /*Use default direction.*/
#define REC_BOTTOM_TOP        0x1         /*Bottom to top.*/
#define REC_LEFT_RIGHT        0x2         /*Left to right.*/
#define REC_RIGHT_LEFT        0x3         /*Right to left.*/
#define REC_TOP_BOTTOM        0x4         /*Top to bottom.*/

/* 
 * Structure for describing a set of letters to constrain recognition. 
 * ls_type is the same as the re_type field for rec_element below.
*/

typedef struct _letterset {
        char ls_type;
        union _ls_set {
                char* aval;
                wchar_t* wval;
        } ls_set;
} letterset;

/*
 * Recognition context. Describes the context in which the pen stroke
 * data was obtained and in which recognition should proceed. 
*/

typedef struct {
    u_short rc_upref;           /*User preference. */
    bool rc_gesture;            /*Look for gesture if true.*/
    u_short rc_direction;       /*Primary and secondary writing direction.*/
    rec_confidence rc_cutoff;	/*Cut off recognition below this confidence*/
    tablet_cap* rc_tinfo;       /*Tablet capabilities.*/
    char** rc_subset;           /*Confine recognition to these subsets.*/
    pen_frame* rc_frame;        /*If nonNULL, writing area geometry.*/
    wordset rc_wordset;         /*If nonNULL, dictionary.*/
    letterset rc_letterset;     /*If nonNULL, constrain to these chars.*/
    void* rc_context;           /*For recognizer-specific context.*/
} rc;

/**************************  GESTURES  **************************/

/*
 * Gestures. The toolkit initializes the recognizer with a
 * set of gestures having appropriate callbacks. 
 * When a gesture is recognized, it is returned as part of a
 * recognition element. The recognizer fills in the bounding
 * box and hotspots. The toolkit fills in any additional values,
 * such as the current window, and calls the callback.
*/

typedef struct Gesture {
    char* g_name;                      /*The gesture's name.*/
    u_int g_nhs;                       /*Number of hotspots.*/
    pen_point* g_hspots;               /*The hotspots.*/
    pen_rect g_bbox;                   /*The bounding box.*/
    void (*g_action)(struct Gesture*);       /*Pointer to execution function.*/
   void* g_wsinfo;	    	     /*For toolkit to fill in.*/
} gesture;

typedef void (*xgesture)(gesture*);

/*These provide some common gesture names.*/

#define COPY	"COPY"		/*Copy target to clipboard*/
#define CUT	"CUT"		/*Copy target and delete*/
#define PASTE	"PASTE"		/*Paste clipboard into target*/
#define UNDO	"UNDO"		/*Undo the previous gesture action*/
#define CLEAR	"CLEAR"		/*Clear clipboard*/
#define EXTEND	"EXTEND"	/*Extend selection to target location*/
#define RETURN	"RETURN"	/*Insert newline/carriage return at target*/
#define SPACE	"SPACE"		/*Insert space at target*/
#define TAB	"TAB"		/*Insert tab at target*/
#define KKCONVERT  "KKCONVERT"	/*Perform kana-kanji conversion on target*/

/********************* RECOGNITION RETURN VALUES *************************/


/*Different types in union. "Other" indicates a cast is needed.*/

#define REC_NONE    0x0             /*No return value*/
#define REC_GESTURE 0x1             /*Gesture.*/
#define REC_ASCII   0x2             /*Array of 8 bit ASCII*/
#define REC_VAR     0x4             /*Array of variable width characters. */
#define REC_WCHAR   0x8             /*Array of Unicode (wide) characters. */
#define REC_OTHER   0x10            /*Undefined type.*/
#define REC_CORR    0x20	    /*rec_correlation struct*/

/*
 * Recognition elements. A recognition element is a structure having a 
 * confidence level member, and a union, along with a flag indicating 
 * the union type. The union contains a pointer to the result. This
 * is the basic recognition return value, corresponding to one
 * recognized word, letter, or group of letters.
*/

/*Ruse to make types woik*/

#define rec_correlation void

typedef struct {
    char re_type;                  /*Union type flag.*/
    union {
	gesture* gval;             /*Gesture.*/
	char* aval;                /*ASCII and variable width.*/
	wchar_t* wval;             /*Unicode.*/
	rec_correlation* rcval;    /*rec_correlation*/
    } re_result;                   
    rec_confidence re_conf;        /*Confidence (0-100).*/
} rec_element;

/*
 * Recognition alternative. The recognition alternative gives
 * a translated element for a particular segmentation, and
 * a pointer to an array of alternatives for the next position
 * in the segmentation thread.
*/

struct _Rec_alternative {
	rec_element ra_elem; 		/*the translated element*/
	u_int ra_nalter;		/*number of next alternatives*/
	struct _Rec_alternative* ra_next; /*the array of next alternatives*/
};

typedef struct _Rec_alternative rec_alternative;

/*
 * Recognition correlation. A recognition correlation is a recognition
 * of the stroke input along with a correlation between the stroke
 * input and the recognized text. The rec_correlation struct contains
 * a pointer to an arrray of pointers to strokes, and
   two arrays of integers, giving the starting point and
 * stopping point of each corresponding recogition element returned
 * in the strokes. 
*/

#undef rec_correlation

typedef struct {
    rec_element ro_elem;          /*The recognized alternative.*/
    u_int ro_nstrokes;            /*Number of strokes.*/
    pen_stroke* ro_strokes;       /*Array of strokes.*/
    u_int* ro_start;              /*Starting index of points.*/
    u_int* ro_stop;               /*Stopping index of points.*/
} rec_correlation;

#endif
