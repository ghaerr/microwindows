/* 
 *  hre_api.c:        Implementation of HRE API
 *  Author:           James &
 *  Created On:       Wed Dec  9 13:49:14 1992
 *  Last Modified By: James Kempf
 *  Last Modified On: Fri Sep 23 13:49:04 1994
 *  Update Count:     137
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


#include <sys/types.h>
#ifdef ELX
#include <vxWorks.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
/*#include <libintl.h>*/
#include <hre_internal.h>   /* includes hre.h */

/* ari -- prototype for rii function */
recognizer __recognizer_internal_initialize(rec_info* ri);

/*Version number of API.*/

char* REC_VERSION = "2.0";

/*Domain name for internationalized text.*/

#define INTL_DOMAIN "recognition_manager"

/* XXX -- Intl Hack -- Jay & Ari */
#define	dgettext(domain, msg)	(msg)
#define	bindtextdomain(dirname,	domain)

/*
 * These magic numbers are used to ensure the integrity of the
 * recognizer structure.
*/


#define REC_MAGIC       0xfeed
#define REC_END_MAGIC   0xbeef

/*Check the recognizer for validity*/

#define RI_CHECK_MAGIC(rec) \
  ( (rec != NULL) && \
    (((recognizer)rec)->recognizer_magic == REC_MAGIC) && \
   (((recognizer)rec)->recognizer_end_magic == REC_END_MAGIC) &&\
   (((recognizer)rec)->recognizer_version == REC_VERSION) )

/*The name of the initialization & finalization functions.*/

/* static char rii_name[] = "__recognizer_internal_initialize";
static char rif_name[] = "__recognizer_internal_finalize";  */

/*User home directory for recognizer info.*/
/* ari -- changed USERRECHOME from ".recognizers" */
#define HOME "HOME"
#define USERRECHOME ".classifiers"

/*Local functions*/

#if 0
static char* shared_library_name(char* directory,char* locale,char* name);
#endif
static rec_info* make_rec_info(char* directory,char* name,char** subset);
static void delete_rec_info(rec_info* ri);
#if 0
static int check_for_user_home();
#endif
static void intl_initialize();

static void cleanup_rec_element(rec_element* re,bool delete_points_p);

/*The last error.*/

static char* the_last_error = NULL;

static char *safe_malloc (int nbytes)
{
  char *res = malloc(nbytes);
  if (res == NULL) {
    error("malloc failure");
    exit(2);
  }
  return (res);
}


/*
 * Implementation of API functions
*/

/*
 * recognizer_load - Load the recognizer matching the rec_info struct.
 * If name is not null, then load the recognizer having that name. Returns
 * the recognizer object, or null if it can't load the recognizer, and
 * sets errno to indicate why.
*/

recognizer 
recognizer_load(char* directory,char* name,char** subset)
{
    recognizer rec;                     /*the recognizer*/
#if 0
    recognizer_internal_initialize rii; /*the initialization function*/
#endif
    rec_info* rinf;                     /*rec_info for recognizer information*/
    static bool intl_init = false;	/*true if recog. manager initted.*/

    if( intl_init == false ) {
      intl_init = true;

      intl_initialize();
    }

    /*The name takes precedence.*/
    rinf = make_rec_info(directory,name,subset);
    if (rinf == NULL) {
	the_last_error = 
	  dgettext(INTL_DOMAIN,
		   "Ran out of memory during prelinking initialization.");
	return((recognizer)NULL);
    } 
    /*fprintf(stderr, "Got past make_rec_info.\n");*/

    /*Let recognition code create recognizer and initialize*/
    rec = __recognizer_internal_initialize(rinf);
    if (rec == NULL) {
	return((recognizer)NULL);
    }
    /*fprintf(stderr, "Did rii.\n");*/
    /*Check whether it's been correctly initialized*/

    if( rec->recognizer_load_state == NULL ||
        rec->recognizer_save_state == NULL ||
        rec->recognizer_load_dictionary == NULL ||
        rec->recognizer_save_dictionary == NULL ||
        rec->recognizer_free_dictionary == NULL ||
        rec->recognizer_add_to_dictionary == NULL ||
        rec->recognizer_delete_from_dictionary == NULL ||
        rec->recognizer_error == NULL ||
        rec->recognizer_set_context == NULL ||
        rec->recognizer_get_context == NULL ||
        rec->recognizer_clear == NULL ||
        rec->recognizer_get_buffer == NULL ||
        rec->recognizer_set_buffer == NULL ||
        rec->recognizer_translate == NULL ||
        rec->recognizer_get_extension_functions == NULL ||
        rec->recognizer_get_gesture_names == NULL ||
        rec->recognizer_set_gesture_action == NULL
       ) {

	recognizer_unload(rec);
fprintf(stderr, "Unloading b/c null function pointer.\n");
	the_last_error = 
	  dgettext(INTL_DOMAIN,
		   "One or more recognizer function pointers is NULL.");
	return((recognizer)NULL);
    }


    /*Set the rec_info structure.*/

    rec->recognizer_info = rinf;

    /*Check whether home directory is there for recognizer info.*/

/*
 *  ari -- don't bother.  We're not going to load from each user's
 *  home directory at this point.  Instead, we'll use a stupid
 *  little a-b-c file because it loads FAST.
 *
 *    if( check_for_user_home() < 0 ) {
 *	recognizer_unload(rec);
 *	return((recognizer)NULL);
 *   }
 */
    /*We got it!*/
    /*fprintf(stderr, "Done.\n");*/

    return(rec);
}

/*
 * recognizer_unload - Unload the recognizer.
*/

int
recognizer_unload(recognizer rec)
{
#if 0
    recognizer_internal_finalize rif;
#endif
    
    /*Make sure magic numbers right.*/
    
    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }
    
    __recognizer_internal_finalize(rec);

    return(0);
}

/*
 * recognizer_load_state-Get any recognizer state associated with name
 * in dir. Note that name may not be simple file name, since
 * there may be more than one file involved. Return 0 if successful,
 * -1 if not.
*/

int recognizer_load_state(recognizer rec,char* dir,char* name)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/
    return(rec->recognizer_load_state(rec,dir,name));
}

/*
 * recognizer_save_state-Save any recognizer state to name
 * in dir. Note that name may not be a simple file name, since
 * there may be more than one file involved. Return 0 if successful,
 * -1 if not.
*/

int recognizer_save_state(recognizer rec,char* dir,char* name)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_save_state(rec,dir,name));
}

/*
 * recognizer_load_dictionary-Load dictionary, return pointer
 * to it, or NULL if error.
*/

wordset recognizer_load_dictionary(recognizer rec,char* dir,char* name)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(NULL);
    }

    /*Do the function.*/

    return(rec->recognizer_load_dictionary(rec,dir,name));
}

/*
 * recognizer_save_dictionary-Save the  dictionary to the file, return 0 if
 * OK, -1 if error.
*/

int recognizer_save_dictionary(recognizer rec,char* dir,char* name,wordset dict)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_save_dictionary(rec,dir,name,dict));
}

/*
 * recognizer_free_dictionary-Free the dictionary, return 0 if
 * OK, -1 if error.
*/

int recognizer_free_dictionary(recognizer rec,wordset dict)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_free_dictionary(rec,dict));
}

/*
 * recognizer_add_to_dictionary-Add word to the dictionary,
 * return 0 if OK, -1 if error.
*/


int recognizer_add_to_dictionary(recognizer rec,letterset* word,wordset dict)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_add_to_dictionary(rec,word,dict));
}

/*
 * recognizer_delete_from_dictionary-Delete word from the dictionary,
 * return 0 if OK, -1 if error.
*/

int 
recognizer_delete_from_dictionary(recognizer rec,letterset* word,wordset dict)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_delete_from_dictionary(rec,word,dict));
}

/*
 * recognizer_get_info-Get a pointers to the rec_info
 * giving the locales and subsets supported by the recognizer
 * and the shared library pathname.
*/

const rec_info*
recognizer_get_info(recognizer rec)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return((rec_info*)NULL);
    }

    /*Return the rec_info object.*/

    return(rec->recognizer_info);
}

/*
 * recognizer_manager_version-Return the version number string of the
 * recognition manager.
*/

const char* recognizer_manager_version(recognizer rec)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(NULL);
    }

    return(rec->recognizer_version);
  
}
/*
 * recognizer_error-Return the last error message, or NULL if none.
*/

char* recognizer_error(recognizer rec)
{
    
    /*Make sure magic numbers right and function there.*/

    if( !RI_CHECK_MAGIC(rec) && the_last_error == NULL ) {
      return(dgettext(INTL_DOMAIN,"Bad recognizer object."));

    } else if( the_last_error != NULL ) {
      char* error = the_last_error;

      the_last_error = NULL;
      return(error);
    }

    /*Do the function.*/

    return(rec->recognizer_error(rec));
}

/*
 * recognizer_set_context-Set the recognition context for translation.
 * Return 0 if successful, -1 if error.
*/

int recognizer_set_context(recognizer rec,rc* rec_xt)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_set_context(rec,rec_xt));
}

/* 
 * recognzier_get_context-Get the recognition context for translation.
 * If none or error, return NULL.
*/

rc* recognizer_get_context(recognizer rec)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(NULL);
    }

    /*Do the function.*/

    return(recognizer_get_context(rec));
}

/*
 * recognizer_clear-Clear buffer and recognition context.
 * Return 0 if success, else -1.
*/

int recognizer_clear(recognizer rec,bool delete_points_p)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_clear(rec,delete_points_p));
}

/*recognizer_get_buffer-Get stroke buffer. Return 0 if success, else -1.*/


int recognizer_get_buffer(recognizer rec, u_int* nstrokes,pen_stroke** strokes)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_get_buffer(rec,nstrokes,strokes));

}

/*
 * recognizer_set_buffer-Set stroke buffer to arg. Return 0 if success, else 
 * return -1.
*/

int recognizer_set_buffer(recognizer rec,u_int nstrokes,pen_stroke* strokes)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_set_buffer(rec,nstrokes,strokes));

}

/*
 * recognizer_translate-Translate the strokes in the current context, including
 * buffered strokes. If nstrokes == 0 or strokes == NULL, return 
 * translation of stroke buffer.
*/

int recognizer_translate(recognizer rec,
			 u_int nstrokes,
			 pen_stroke* strokes,
			 bool correlate_p,
			 int* nret,
			 rec_alternative** ret)
{
    int retval;
    char msg[80];
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN, msg);
	return(-1);
    }

/* ari */
/*    {
 *      u_int i;
 *      pen_stroke ari_pstr;
 *      pen_point* ari_pts;
 *      int ari;
 *      for (i = 0; i < nstrokes; i++) {
 *	ari_pstr = strokes[i];
 *	ari_pts = ari_pstr.ps_pts;
 *	fprintf(stderr, "\nrecognizer_translate: ari_pts = %ld, sizeof(Time) = %d, sizeof(ari_pts[0] = %d, %d points are...\n", ari_pts, sizeof(Time), sizeof(ari_pts[0]), ari_pstr.ps_npts);
 *	for (ari = 0; ari < ari_pstr.ps_npts; ari++)
 *	   fprintf(stderr, "%ld -- (%d, %d)  ", ari_pts[ari], ari_pts[ari].x, ari_pts[ari].y);
 *      }
 *    }     
*/
    /*Do the function.*/
/* ari -- this is calling cmu_recognizer_translate */
    retval = rec->recognizer_translate(rec,
				     nstrokes,
				     strokes,
				     correlate_p,
				     nret,
				     ret);
    return (retval);
}


/*
 * recognizer_get_extension_functions-Return a null terminated array
 * of functions providing extended functionality. Their interfaces
 * will change depending on the recognizer.
*/

rec_fn* recognizer_get_extension_functions(recognizer rec)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return((rec_fn*)NULL);
    }

    /*Do the function.*/

    return(rec->recognizer_get_extension_functions(rec));
}


/*
 * recognizer_get_gesture_names - Return a null terminated array of
 * gesture name strings.
*/

char**
recognizer_get_gesture_names(recognizer rec)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(NULL);
    }

    /*Do the function.*/

    return(rec->recognizer_get_gesture_names(rec));
}

/*
 * recognizer_set_gesture_action-Set the action function for the gesture.
*/

xgesture 
recognizer_train_gestures(recognizer rec,char* name,xgesture fn,void* wsinfo)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return((xgesture)-1);
    }

    /*Do the function.*/

    return(rec->recognizer_set_gesture_action(rec,name,fn,wsinfo));
}

/*
 * Local functions.
*/

/*
 * shared_library_name-Get the full pathname to the shared library,
 *    based on the recognizer name and the environment.
*/


#if 0
static char* shared_library_name(char* directory,char* locale,char* name)
{
    char* ret = NULL;
    int len = strlen(name);

    /*If directory is there, it takes precedence.*/

    if( directory != NULL ) {
	ret = (char*)safe_malloc(strlen(directory) + len + 2);
	strcpy(ret,directory);
	strcat(ret,"/");
	strcat(ret,name);

    } 
    else {
	char* dir = NULL;

	/*First try the environment variable.*/

	if( (dir = getenv(RECHOME)) == NULL ) {
	    dir = "REC_DEFAULT_HOME_DIR";

	  }

	ret = (char*)safe_malloc(strlen(dir) + strlen(locale) + len + 3);
	/*Form the pathname.*/
	strcpy(ret,dir);
	strcat(ret,"/");
	strcat(ret,locale);
	strcat(ret,"/");
	strcat(ret,name);

    }

    return(ret);
}
#endif

/*
 * intl_initialize-Initialize the internationaliztion of messages for
 * the recognition manager.
*/

static void intl_initialize()
{
  char* dirname;

  /*Get recognizer home directory name from environment.*/

  if( (dirname = getenv(RECHOME)) == NULL ) {
    dirname = "REC_DEFAULT_HOME_DIR";
  }

  /*Bind the text domain.*/

  bindtextdomain(dirname,INTL_DOMAIN);
}


/*make_rec_info-Create a rec_info structure*/

static rec_info* make_rec_info(char* directory,char* name,char** subset)
{
    int i,len;
    rec_info* ri;
    char* locale;

    ri = (rec_info*)safe_malloc(sizeof(rec_info));
    ri->ri_locale = NULL;
    ri->ri_name = NULL;
    ri->ri_subset = NULL;

    /*Get locale*/

    if( (locale = getenv(LANG)) == NULL ) {
	locale = strdup(REC_DEFAULT_LOCALE);
    }

    if( (ri->ri_locale = strdup(locale)) == NULL ) {
	delete_rec_info(ri);
	return(NULL);
    }

    /*Get shared library pathname.*/

/*
 *    if( (ri->ri_name = shared_library_name(directory,locale,name)) == NULL ) {
 *	delete_rec_info(ri);
 *	return(NULL);
 *    }
 */

    /*Initialize the subset information.*/

    if( subset != NULL ) {
	
	/*Count the subset strings.*/

	for( len = 1; subset[len] != NULL; len++ ) ;
	
	/*Copy the subset strings.*/
	
	ri->ri_subset = (char**)safe_malloc((len +1)*sizeof(char*));
	
	for( i = 0; i < len; i++ ) {
	    if( subset[i] != NULL ) {
		if( (ri->ri_subset[i] = strdup(subset[i])) == NULL ) {
		    delete_rec_info(ri);
		    return(NULL);
		}
	    } else {
		ri->ri_subset[i] = subset[i];
	    }
	}

	ri->ri_subset[i] = NULL;

    } else {

	ri->ri_subset = NULL;
    }
    
    return(ri);
}

static void delete_rec_info(rec_info* ri)
{
    if( ri != NULL ) {
	if( ri->ri_locale != NULL ) {
	    free(ri->ri_locale);
	}
/*
 *	if( ri->ri_name != NULL ) {
 *	    free(ri->ri_name);
 *	}
 */
	if( ri->ri_subset != NULL ) {
	    int i;
	    for( i = 0; ri->ri_subset[i] != NULL; i++) {
		free(ri->ri_subset[i]);
	    }
	    free(ri->ri_subset);
	}
	free(ri);
    }
}

/*check_for_user_home-Check whether USERRECHOME has been created.*/

#if 0
static int check_for_user_home()
{
    char* homedir = getenv(HOME);
    char* rechome = NULL;

    if( homedir == NULL ) {
	the_last_error = "Home environment variable HOME not set.";
	return(-1);
    }

    rechome = (char*)safe_malloc(strlen(homedir) + strlen(USERRECHOME) + 2);

    /*Form name.*/

    strcpy(rechome,homedir);
    strcat(rechome,"/");
    strcat(rechome,USERRECHOME);

    /*Create directory.*/

    if( mkdir(rechome,S_IRWXU | S_IRWXG | S_IRWXO) < 0 ) {

	/*If errno is EEXIST, then OK.*/

	if( errno != EEXIST ) {
	    the_last_error = "Error during creation of USERRECHOME.";
	    free(rechome);
	    return(-1);
	}
    }

    free(rechome);

    return(0);
}
#endif

/*
 * Constructor functions for making structures.
 *
 *    The general philosophy here is that we control all memory
 *    in connected data structures, *except* for pen_point arrays.
 *    There are likely to be lots and lots of points, they are likely
 *    to come from the window system; so if we wanted to control them,
 *    we would have to copy which would be slow. We require the client
 *    to deal with them directly, or the client can give us permission
 *    to delete them.
*/

/*
 * recognizer
*/


recognizer make_recognizer(rec_info* rif)
{
    recognizer rec;
    
    /*Allocate it.*/

    rec = (recognizer)safe_malloc(sizeof(*rec));
    rec->recognizer_magic = REC_MAGIC;
    rec->recognizer_version = REC_VERSION;
    rec->recognizer_info = rif;
    rec->recognizer_specific = NULL;
    rec->recognizer_end_magic = REC_END_MAGIC;
    rec->recognizer_load_state = NULL;
    rec->recognizer_save_state = NULL;
    rec->recognizer_load_dictionary = NULL;
    rec->recognizer_save_dictionary = NULL;
    rec->recognizer_free_dictionary = NULL;
    rec->recognizer_add_to_dictionary = NULL;
    rec->recognizer_delete_from_dictionary = NULL;
    rec->recognizer_error = NULL;
    rec->recognizer_set_context = NULL;
    rec->recognizer_get_context = NULL;
    rec->recognizer_clear = NULL;
    rec->recognizer_get_buffer = NULL;
    rec->recognizer_set_buffer = NULL;
    rec->recognizer_translate = NULL;
    rec->recognizer_get_extension_functions = NULL;
    rec->recognizer_get_gesture_names = NULL;
    rec->recognizer_set_gesture_action = NULL;
    return(rec);
}

void delete_recognizer(recognizer rec)
{

    if( rec != NULL ) {
	if( rec->recognizer_info != NULL ) {
	    delete_rec_info(rec->recognizer_info);
	}
	free(rec);
    }
}

/*
 * rec_alternative
*/

rec_alternative* make_rec_alternative_array(u_int size)
{
    int i;
    rec_alternative* ri;

    ri = (rec_alternative*) safe_malloc(size * sizeof(rec_alternative));

    for( i = 0; i < size; i++ ) {
        ri[i].ra_elem.re_type = REC_NONE;
	ri[i].ra_elem.re_result.aval = NULL;
	ri[i].ra_elem.re_conf = 0;
	ri[i].ra_nalter = 0;
	ri[i].ra_next = NULL;
    }

    return(ri);    
}

rec_alternative*
  initialize_rec_alternative(rec_alternative* ra,
			     u_int nelem)
{
  if( ra != NULL ) {
    if( (ra->ra_next = make_rec_alternative_array(nelem)) == NULL ) {
      return(NULL);
    }

    ra->ra_nalter = nelem;
  }

  return(ra);
}

void delete_rec_alternative_array(u_int nalter,
				  rec_alternative* ra,
				  bool delete_points_p)
{
  int i;

    if( ra != NULL ) {

      for( i = 0; i < nalter; i++ ) {
	cleanup_rec_element(&ra[i].ra_elem,delete_points_p);
	
	/*Now do the next one down the line.*/
	
	if( ra[i].ra_nalter > 0 ) {
	  delete_rec_alternative_array(ra[i].ra_nalter,
				       ra[i].ra_next,
				       delete_points_p);
        }
      }

      free(ra);
    }
}


/*initialize_rec_element-Initialize a recognition element.*/

rec_element*
initialize_rec_element(rec_element* re,
		       char type,
		       u_int size,
		       void* trans,
		       rec_confidence conf)
{
    if( re != NULL ) {

	re->re_type = type;
	re->re_conf = conf;
	re->re_result.aval = NULL;
	
	switch (type) {
	    
	  case REC_GESTURE:
	    if( size > 0 && trans != NULL ) {
		re->re_result.gval = 
		     (gesture*)safe_malloc(sizeof(gesture));
		memcpy((void*)re->re_result.gval,trans,sizeof(gesture));
	    }
	    break;
	    
	  case REC_ASCII:
	  case REC_VAR:
	  case REC_OTHER:
	    if( size > 0 && trans != NULL ) {
		re->re_result.aval = 
		     (char*)safe_malloc((size+1)*sizeof(char));
		memcpy((void*)re->re_result.aval,trans,size*sizeof(char));
		re->re_result.aval[size] = '\000';
	    }
	    break;
	    
	  case REC_WCHAR:
	    if( size > 0 && trans != NULL ) {
		re->re_result.wval = 
		     (wchar_t*)safe_malloc((size+1)*sizeof(wchar_t));
		memcpy((void*)re->re_result.wval,trans,size*sizeof(wchar_t));
		re->re_result.wval[size] = '\000';
	    }
	    break;
	    
	  case REC_CORR:
	    if( size > 0 && trans != NULL ) {
	      re->re_result.rcval =
		   (rec_correlation*)safe_malloc(sizeof(rec_correlation));
	      memcpy((void*)re->re_result.rcval,
		     trans,
		     sizeof(rec_correlation));
	    }
	    break;

	  default:
	    return(NULL);
	}

    }

    return(re);
}

static void cleanup_rec_element(rec_element* re,bool delete_points_p)
{
  switch(re->re_type) {
    
  case REC_NONE:
    break;
    
  case REC_ASCII:
  case REC_VAR:
  case REC_WCHAR:
  case REC_OTHER:
    free(re->re_result.aval);
    break;
    
  case REC_GESTURE:
    delete_gesture_array(1,re->re_result.gval,true);
    break;

  case REC_CORR:
    delete_rec_correlation(re->re_result.rcval,
			   delete_points_p);
    break;
    
  }
  
}

/*
 * rec_correlation
*/


rec_correlation* 
make_rec_correlation(char type,
		     u_int size,
		     void* trans,
		     rec_confidence conf,
		     u_int ps_size)
{
  rec_correlation* rc;

    rc = (rec_correlation*)safe_malloc(sizeof(rec_correlation));

    rc->ro_nstrokes = ps_size;

    /*First initialize element.*/

    if( initialize_rec_element(&(rc->ro_elem),
			       type,
			       size,
			       trans,
			       conf) == NULL ) {
      return(NULL);
    }
    
    if( (rc->ro_strokes = make_pen_stroke_array(ps_size)) == NULL ) {
      return(NULL);
    }
    
    rc->ro_start = (u_int*)safe_malloc(ps_size * sizeof(int));
    rc->ro_stop = (u_int*)safe_malloc(ps_size * sizeof(int));
    return(rc);
}

void delete_rec_correlation(rec_correlation* rc,bool delete_points_p)
{
  if( rc != NULL ) {

    cleanup_rec_element(&rc->ro_elem,delete_points_p);

    delete_pen_stroke_array(rc->ro_nstrokes,rc->ro_strokes,delete_points_p);

    if( rc->ro_start != NULL ) {
      free(rc->ro_start);
    }

    if( rc->ro_stop != NULL ) {
      free(rc->ro_stop);
    }

    free(rc);
  }

}


/*
 * rec_fn
*/


rec_fn* make_rec_fn_array(u_int size)
{
    rec_fn* ri = (rec_fn*)safe_malloc((size + 1) * sizeof(rec_fn));
    int i;

    for( i = 0; i < size; i++ ) {
	ri[i] = NULL;
    }

    ri[i] = NULL;

    return(ri);
}

void delete_rec_fn_array(rec_fn* rf)
{
    if( rf != NULL ) {
	free(rf);
    }
}

/*
 * pen_stroke
*/


pen_stroke* make_pen_stroke_array(u_int size)
{
    int i;
    pen_stroke* ri;

    ri = (pen_stroke*) safe_malloc(size * sizeof(pen_stroke));
    for( i = 0; i < size; i++ ) {
	ri[i].ps_npts = 0;
	ri[i].ps_pts = NULL;
	ri[i].ps_nstate = 0;
	ri[i].ps_state = NULL;
    }

    return(ri);       
}

pen_stroke* initialize_pen_stroke(pen_stroke* ps,
				  u_int npts,
				  pen_point* pts,
				  u_int nstate,
				  u_int* trans,
				  pen_state* state)
{
  if( ps != NULL ) {
    ps->ps_npts = npts;
    ps->ps_pts = pts;
    ps->ps_nstate = nstate;
    ps->ps_trans = trans;
    ps->ps_state = state;
  }
  return (ps);
}

void delete_pen_stroke_array(u_int size,pen_stroke* ps,bool delete_points_p)
{
  int i;
  
    if( ps != NULL ) {

      for( i = 0; i < size; i++ ) {

	    if( ps[i].ps_state != NULL ) {
		free(ps[i].ps_state);
	    }

	    if( ps[i].ps_trans != NULL ) {
	        free(ps[i].ps_trans);
	    }

	    if( delete_points_p ) {
		delete_pen_point_array(ps[i].ps_pts);
	    }

      }
	
      free(ps);
    }
}

/*
 * pen_point
*/


pen_point* make_pen_point_array(u_int size)
{
    pen_point* pp = (pen_point*)safe_malloc(size * sizeof(pen_point));
    int i;

    for( i = 0; i < size; i++ ) {
	pp[i].time = 0;
	pp[i].x = pp[i].y = 0;
    }

    return(pp);
}

void delete_pen_point_array(pen_point* pp)
{
    if( pp != NULL ) {
	free(pp);
    }
}

/*
 * pen_state
*/


pen_state* make_pen_state_array(u_int size)
{
  int i;

  pen_state* ps = (pen_state*)safe_malloc(size*sizeof(pen_state));
  
  for( i = 0; i < size; i++ ) {
    ps[i].pt_button = 0;
    ps[i].pt_pen = 0;
    ps[i].pt_pressure = 0;
    ps[i].pt_anglex = 0.0;
    ps[i].pt_angley = 0.0;
    ps[i].pt_barrelrotate = 0.0;
  }

  return(ps);

}

pen_state* initialize_pen_state(pen_state* ps,
				u_short button,
				u_short pen,
				short pressure,
				double anglex,
				double angley,
				double barrelrotate)
{
  if( ps != NULL ) {
    ps->pt_button = button;
    ps->pt_pen = pen;
    ps->pt_pressure = pressure;
    ps->pt_anglex = anglex;
    ps->pt_angley = angley;
    ps->pt_barrelrotate = barrelrotate;
  }

  return(ps);
}

void delete_pen_state_array(pen_state* ps)
{
  if( ps != NULL ) {
    free(ps);
  }
}

/*
 * gesture 
*/

gesture*
make_gesture_array(u_int size)
{
    return((gesture*)safe_malloc(size * sizeof(gesture)));
}

gesture* initialize_gesture(gesture* g,
			    char* name,
			    u_int nhs,
			    pen_point* hspots,
			    pen_rect bbox,
			    xgesture fn,
			    void* wsinfo)
{
    if( g != NULL ) {

	/*We don't do points, 'cause they come from the window system.*/

	g->g_nhs = nhs;
	g->g_hspots = hspots;

	g->g_name = strdup(name);

	g->g_bbox.x = bbox.x;
	g->g_bbox.y = bbox.y;
	g->g_bbox.width = bbox.width;
	g->g_bbox.height = bbox.height;
	g->g_action = fn;
	g->g_wsinfo = wsinfo;
    }

    return(g);
}

void
delete_gesture_array(u_int size,gesture* ga,bool delete_points_p)
{
    int i;

    if( ga != NULL ) {

      for( i = 0; i < size; i++ ) {
	
	free(ga[i].g_name);
	
	if( delete_points_p ) {
	  delete_pen_point_array(ga[i].g_hspots);
	}
      }
      
      free(ga);
    }
}

/*
 * copy fns for stroke buffer management.
*/

static pen_stroke* 
copy_pen_stroke(pen_stroke* ps1,pen_stroke* ps2)
{
  u_int* trans = NULL;
  pen_state* state = NULL;
  
  if( (trans = 
       copy_state_trans_array(ps2->ps_nstate,
			      ps2->ps_trans)) == NULL ) {
    return(NULL);
  }
  
  if( (state = 
       copy_pen_state_array(ps2->ps_nstate,
			    ps2->ps_state)) == NULL ) {
    free(trans);
    return(NULL);
  }
  
  initialize_pen_stroke(ps1,
			ps2->ps_npts,
			ps2->ps_pts,
			ps2->ps_nstate,
			trans,
			state);
  return(ps1);

}

pen_stroke*
 copy_pen_stroke_array(u_int nstrokes,
		    pen_stroke* strokes)
{
  int i;
  pen_stroke* ps = make_pen_stroke_array(nstrokes);

  if( ps != NULL ) {

    for( i = 0; i < nstrokes; i++ ) {

      copy_pen_stroke(&ps[i],&strokes[i]);

    }

  }

  return(ps);
}

pen_state*
 copy_pen_state_array(u_int nstate,pen_state* state)
{
  pen_state* ps = make_pen_state_array(nstate);
  int i;

  if( ps != NULL ) {

    for( i = 0; i < nstate; i++ ) {
      
      initialize_pen_state(&ps[i],
			   state[i].pt_button,
			   state[i].pt_pen,
			   state[i].pt_pressure,
			   state[i].pt_anglex,
			   state[i].pt_angley,
			   state[i].pt_barrelrotate);
    }
    
  }

  return(ps);
}

u_int*
 copy_state_trans_array(u_int ntrans,u_int* trans)
{
  u_int* pt = (u_int*)safe_malloc(ntrans*sizeof(u_int));
  int i;

  for( i = 0; i < ntrans; i++ ) {
    pt[i] = trans[i];
  }
  return(pt);

}

pen_stroke*
concatenate_pen_strokes(int nstrokes1,
			pen_stroke* strokes1,
			int nstrokes2,
			pen_stroke* strokes2,
			int* nstrokes3,
			pen_stroke** strokes3)
{
  int i;
  int ns;
  pen_stroke* ps;

  /*Measure new strokes*/

  ns = nstrokes1 + nstrokes2;

  /*Allocate memory*/

  if( (ps = make_pen_stroke_array(ns)) == NULL ) {
    return(NULL);
  }

  /*Copy old ones into new.*/

  for( i = 0; i < nstrokes1; i++ ) {
    if( copy_pen_stroke(&ps[i],&strokes1[i]) == NULL ) {
      delete_pen_stroke_array(ns,ps,false);
      return(NULL);
    }
  }

  for( ; i < ns; i++ ) {
    if( copy_pen_stroke(&ps[i],&strokes2[i - nstrokes1]) == NULL ) {
      delete_pen_stroke_array(ns,ps,false);
      return(NULL);
    }
  }

  *nstrokes3 = ns;
  *strokes3 = ps;

  return(ps);
}
