/*
 *  li_recognizer.h
 *
 *  Adapted from cmu_recognizer.h.
 *  Credit to Dean Rubine, Jim Kempf, and Ari Rapkin.
 */

#ifndef _LI_RECOGNIZER_H_

#define _LI_RECOGNIZER_H_

/*Extension function interfaces and indices.*/

#define LI_ISA_LI         0               /*Is this a li recognizer?.*/

typedef bool (*li_isa_li)(recognizer r);

#define LI_TRAIN	    1		   /*Train recognizer*/

typedef int (*li_recognizer_train)(recognizer r,
				    rc* rec_xt,
				    u_int nstrokes,
				    pen_stroke* strokes,
				    rec_element* re,
				    bool replace_p);

#define LI_CLEAR           2              /* ari's clear-state extension fn. */

typedef int (*li_recognizer_clearState)(recognizer r);

#define LI_GET_CLASSES     3              /* ari's get-classes extension fn. */

typedef int (*li_recognizer_getClasses)(recognizer r, char ***list, int *nc);

#define LI_NUM_EX_FNS     4               /*Number of extension functions*/

#endif

