/* 
 *  hre_api_internal.h: API functions, used by internal clients also.
 *  Author:           James Kempf
 *  Created On:       Tue Jan 12 12:52:27 1993
 *  Last Modified By: James Kempf
 *  Last Modified On: Fri Sep 23 13:50:48 1994
 *  Update Count:     33
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

#ifndef _HRE_API_INTERNAL_H_

#define _HRE_API_INTERNAL_H_

/*Need structs for return types.*/

#include <hre.h>

/*
 * ADMINISTRATION
*/

/*
 * recognizer_load - If directory is not NULL, then use it as a pathname
 * to find the recognizer. Otherwise, use the default naming conventions
 * to find the recognizer having file name name. The subset argument
 * contains a null-terminated array of names for character subsets which
 * the recognizer should translate.
*/

recognizer 
recognizer_load(char* directory,char* name,char** subset);

/*
 * recognizer_unload - Unload the recognizer.
*/

int
recognizer_unload(recognizer rec);

/*
 * recognizer_get_info-Get a pointer to a rec_info 
 * giving the locale and subsets supported by the recognizer, and shared
 * library pathname.
*/

const rec_info* 
recognizer_get_info(recognizer rec);


/*
 * recognizer_manager_version-Return the version number string of the
 * recognition manager.
*/

const char* recognizer_manager_version(recognizer rec);

/*
 * recognizer_load_state-Get any recognizer state associated with name
 * in dir. Note that name may not be simple file name, since
 * there may be more than one file involved. Return 0 if successful,
 * -1 if not.
*/

int 
recognizer_load_state(recognizer rec,char* dir,char* name);

/*
 * recognizer_save_state-Save any recognizer state to name
 * in dir. Note that name may not be a simple file name, since
 * there may be more than one file involved. Return 0 if successful,
 * -1 if not.
*/

int 
recognizer_save_state(recognizer rec,char* dir,char* name);

/*
 * recognizer_error-Return the last error message, or NULL if none.
*/

char* 
recognizer_error(recognizer rec);

/*
 * DICTIONARIES
*/

/* recognizer_load_dictionary-Load a dictionary from the directory
 * dir and file name. Return the dictionary pointer if successful,
 * otherwise NULL.
*/

wordset 
recognizer_load_dictionary(recognizer rec,char* directory,char* name);

/* recoginzer_save_dictionary-Save the dictionary to the file. Return 0
 * successful, -1 if error occurs.
*/

int 
recognizer_save_dictionary(recognizer rec,char* dir,char* name,wordset dict);

/*
 * recognizer_free_dictionary-Free the dictionary. Return 0 if successful,
 * -1 if error occurs.
*/

int
recognizer_free_dictionary(recognizer rec,wordset dict);

/*
 * recognizer_add_to_dictionary-Add the word to the dictionary. Return 0
 * if successful, -1 if error occurs.
*/

int
recognizer_add_to_dictionary(recognizer rec,letterset* word,wordset dict);

/*
 * recognizer_delete_from_dictionary-Delete the word from the dictionary.
 * Return 0 if successful, -1 if error occurs.
*/

int
recognizer_delete_from_dictionary(recognizer rec,letterset* word,wordset dict);

/*
 * TRANSLATION
*/

/* recognizer_set/get_context - Set/get the recognition context for 
 * subsequent buffering and translation. recognizer_set_context() 
 * returns -1 if an error occurs, otherwise 0. recognizer_get_context() 
 * returns NULL if no context has been set. The context is copied to avoid 
 * potential memory deallocation problems.
*/

int 
recognizer_set_context(recognizer rec,rc* rec_xt);
rc* 
recognizer_get_context(recognizer rec);

/* recognizer_clear - Set stroke buffer to NULL and clear the context. 
 * Returns -1 if an error occurred, otherwise 0. Both the context and the 
 * stroke buffer are deallocated. If delete_points_p is true, delete the
 * points also.
*/

int 
recognizer_clear(recognizer rec,bool delete_points_p);

/* recognizer_get/set_buffer - Get/set the stroke buffer. The stroke buffer 
 * is copied to avoid potential memory allocation problems. Returns -1 if 
 * an error occurs, otherwise 0.
*/

int 
 recognizer_get_buffer(recognizer rec, u_int* nstrokes,pen_stroke** strokes);

int 
recognizer_set_buffer(recognizer rec,u_int nstrokes,pen_stroke* strokes);

/* recognizer_translate - Copy the strokes argument into the stroke buffer and
 * translate the buffer. If correlate_p is true, then provide stroke 
 * correlations as well. If either nstrokes is 0 or strokes is NULL, then 
 * just translate the stroke buffer and return the translation. Return an 
 * array of alternative translation segmentations in the ret pointer and the 
 * number of alternatives in nret, or NULL and 0 if there is no translation. 
 * The direction of segmentation is as specified by the rc_direction field in 
 * the buffered recognition context. Returns -1 if an error occurred, 
 * otherwise 0. 
*/

int 
recognizer_translate(recognizer rec,
		     u_int nstrokes,
		     pen_stroke* strokes,
		     bool correlate_p,
		     int* nret,
		     rec_alternative** ret);

/*
 * recognizer_get_extension_functions-Return a null terminated array
 * of functions providing extended functionality. Their interfaces
 * will change depending on the recognizer.
*/

rec_fn* 
recognizer_get_extension_functions(recognizer rec);


/*
 * GESTURE SUPPORT
*/

/*
 * recognizer_get_gesture_names - Return a null terminated array of
 * character strings containing the gesture names.
*/

char** 
recognizer_get_gesture_names(recognizer rec);

/*
 * recognizer_set_gesture_action-Set the action function associated with the 
 *  name.
*/

xgesture 
recognizer_set_gesture_action(recognizer rec,
			      char* name,
			      xgesture fn,
			      void* wsinof);

/*
 * The following functions are for deleting data structures returned
 *   by the API functions.
*/


void 
delete_rec_alternative_array(u_int nalter,
			     rec_alternative* ra,
			     bool delete_points_p);

void 
delete_rec_correlation(rec_correlation* corr,
		       bool delete_points_p);

/*
 * These are used by clients to create arrays for passing to API
 *  functions.
*/

pen_stroke* 
make_pen_stroke_array(u_int size);
pen_stroke* 
initialize_pen_stroke(pen_stroke* ps,
		      u_int npts,
		      pen_point* pts,
		      u_int nstate,
		      u_int* trans,
		      pen_state* state);
void 
delete_pen_stroke_array(u_int size,pen_stroke* ps,bool delete_points_p);

pen_point* 
make_pen_point_array(u_int size);
void 
delete_pen_point_array(pen_point* pp);

pen_stroke*
copy_pen_stroke_array(u_int nstrokes,pen_stroke* strokes);
pen_state*
copy_pen_state_array(u_int nstate,pen_state* state);
u_int*
copy_state_trans_array(u_int ntrans,u_int* trans);

pen_state* 
make_pen_state_array(u_int size);
pen_state* 
initialize_pen_state(pen_state* ps,
		     u_short button,
		     u_short pen,
		     short pressure,
		     double anglex,
		     double angley,
		     double barrelrotate);
void 
delete_pen_state_array(pen_state* ps);

#endif

