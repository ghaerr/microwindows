/* 
 *  hre_internal.h:   Internal Interface for Recognizer.
 *  Author:           James Kempf
 *  Created On:       Thu Nov  5 10:54:18 1992
 *  Last Modified By: James Kempf
 *  Last Modified On: Fri Sep 23 13:51:15 1994
 *  Update Count:     99
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

#ifndef _HRE_INTERNAL_H_

#define _HRE_INTERNAL_H_

/*Avoids forward reference problem.*/

#define wordset void*

#include <hre.h>

#undef wordset

#define recognizer void*

/*
 * Internal view of wordset. The recognition engine uses this view to
 * maintain information about which recognizer object this wordset
 * belongs to, which file (in case it needs to be saved), and internal
 * data structures.
*/

typedef struct _wordset {
	char* ws_pathname;		/*Path name to word set file.*/
	recognizer ws_recognizer;	/*To whom it belongs.*/
	void* ws_internal;		/*Internal data structures.*/
} *wordset;

#undef recognizer

/*
 * Internal view of the recognizer struct. This view is only available
 * to OEM clients who implement a recognizer shared library. Clients
 * of the recognizer itself see it as an opaque data type. The struct
 * contains a function pointer for each function in the client API.
*/

struct _Recognizer {
	u_int recognizer_magic;
/* ari */
/* 	const char* recognizer_version;  */
        char* recognizer_version; 

	rec_info* recognizer_info;
	void* recognizer_specific;
	int
		(*recognizer_load_state)(
			struct _Recognizer*,
			char*, char*);
/*			char* dir,
			char* name);
*/
	int
		(*recognizer_save_state)(
			struct _Recognizer*,
			char*, char*);
/*			char* dir,
			char* name); 
*/
	char*
		(*recognizer_error)(
			struct _Recognizer*);
	wordset
		(*recognizer_load_dictionary)(
/*			struct _Recognizer* rec,
			char* directory,
			char* name);
*/
			struct _Recognizer*,
                        char*, char*);
	int
		(*recognizer_save_dictionary)(
/*			struct _Recognizer* rec,
			char* directory,
			char* name,
			wordset dict);
*/
			struct _Recognizer*,
                        char*, char*, wordset);

	int
	  	(*recognizer_free_dictionary)(
/*			struct _Recognizer* rec,
			wordset dict);
*/
			struct _Recognizer*,
                        wordset);
	int
	  	(*recognizer_add_to_dictionary)(
/*			struct _Recognizer* rec,
			letterset* word,
			wordset dict);
*/
			struct _Recognizer*,
                        letterset*, wordset);
	int
	  	(*recognizer_delete_from_dictionary)(
/*			struct _Recognizer* rec,
			letterset* word,
			wordset dict);
*/
			struct _Recognizer*,
                        letterset*, wordset);
	int
	 (*recognizer_set_context)(
/*		struct _Recognizer* rec,
		rc* rec_xt);
*/
			struct _Recognizer*,rc*);
	rc*
	 (*recognizer_get_context)(
/*		struct _Recognizer* rec);
*/
			struct _Recognizer*);
				   
	int
	 (*recognizer_clear)(
/*		struct _Recognizer* rec,
		bool delete_ponts_p);
*/
			struct _Recognizer*, bool);
	int
	 (*recognizer_get_buffer)(
/*		struct _Recognizer* rec,
		u_int* nstrokes,
		pen_stroke** strokes);
*/
			struct _Recognizer*, u_int*, pen_stroke**);

	int
	 (*recognizer_set_buffer)(
/*		struct _Recognizer* rec,
		u_int nstrokes,
		pen_stroke* strokes);
*/
			struct _Recognizer*, u_int, pen_stroke*);
	int
	 (*recognizer_translate)(
/*		struct _Recognizer* rec,
		u_int nstrokes,
		pen_stroke* strokes,
		bool correlate_p,
		int* nret,
		rec_alternative** ret);
*/
			struct _Recognizer*, u_int, pen_stroke*,
			bool, int*, rec_alternative**);
	rec_fn*
	 (*recognizer_get_extension_functions)(
		struct _Recognizer*);
	char**
		(*recognizer_get_gesture_names)(
			struct _Recognizer*);
	xgesture
		(*recognizer_set_gesture_action)(
			struct _Recognizer*,
/*			char* name,
			xgesture fn,
			void* wsinfo);
*/
                        char*, xgesture, void*);
	u_int recognizer_end_magic; 
};

typedef struct _Recognizer* recognizer;

/*
 * recognizer_internal_initialize - Allocate and initialize the recognizer 
 * object. The recognition shared library has the responsibility for filling
 * in all the function pointers for the recognition functions. This
 * function must be defined as a global function within the shared
 * library, so it can be accessed using dlsym() when the recognizer
 * shared library is loaded. It returns NULL if an error occured and
 * sets errno to indicate what.
*/

typedef recognizer (*recognizer_internal_initialize)(rec_info* ri);

/*Function header definition for recognizer internal initializer.*/

/* ari -- This is used in cmu_recognizer.c. */

#define RECOGNIZER_INITIALIZE(_a) \
        recognizer __recognizer_internal_initialize(rec_info* _a)

/*
 * recognizer_internal_finalize - Deallocate and deinitialize the recognizer
 * object. If the recognizer has allocated any additional storage, it should
 * be deallocated as well. Returns 0 if successful, -1 if the argument
 * wasn't a recognizer or wasn't a recognizer handled by this library.
*/

typedef int (*recognizer_internal_finalize)(recognizer r);

#define RECOGNIZER_FINALIZE(_a) \
       int __recognizer_internal_finalize(recognizer _a)


/*
 * The following are for creating HRE structures.
 */

recognizer 
make_recognizer(rec_info* ri);
void 
delete_recognizer(recognizer rec);

rec_alternative* 
make_rec_alternative_array(u_int size);
rec_correlation* 
make_rec_correlation(char type,
		     u_int size,
		     void* trans,
		     rec_confidence conf,
		     u_int ps_size);

rec_fn* 
make_rec_fn_array(u_int size);
void 
delete_rec_fn_array(rec_fn* rf);

gesture* 
initialize_gesture(gesture* g,
		   char* name,
		   u_int nhs,
		   pen_point* hspots,
		   pen_rect bbox,
		   xgesture cback,
		   void* wsinfo);
gesture* 
make_gesture_array(u_int size);
void 
delete_gesture_array(u_int size,gesture* ga,bool delete_points_p);

pen_stroke*
concatenate_pen_strokes(int nstrokes1,
			pen_stroke* strokes1,
			int nstrokes2,
			pen_stroke* strokes2,
			int* nstrokes3,
			pen_stroke** strokes3);

rec_alternative*
initialize_rec_alternative(rec_alternative* ra,u_int nelem);
rec_element*
initialize_rec_element(rec_element* re,
		       char type,
		       u_int size,
		       void* trans,
		       rec_confidence conf);
/*
 * Pathnames, etc.
*/

/* these going to be handled in the makefile, for portability */
/* #define REC_DEFAULT_HOME_DIR   "/udir/rapkin/vb/hre.ultrix/lib/recognizers" */
/* #define REC_DEFAULT_USER_DIR	".recognizers" */

#define REC_DEFAULT_LOCALE  	"C"
#define RECHOME			"RECHOME"
#define LANG			"LANG"

#include <hre_api_internal.h>

#endif

