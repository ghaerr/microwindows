/* 
 *  hre_api.h:        User-Level API for Handwriting Recognition
 *  Author:           James Kempf
 *  Created On:       Mon Nov  2 14:01:25 1992
 *  Last Modified By: James Kempf
 *  Last Modified On: Fri Sep 23 13:50:15 1994
 *  Update Count:     22
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

#ifndef _HRE_API_H_

#define _HRE_API_H_

/*
 * Opaque type for the recognizer. The toolkit must access through
 * appropriate access functions.
*/

typedef void* recognizer;

/*
 * Opaque type for recognizers to implement dictionaries.
*/

typedef void* wordset;

#include <hre.h>
#include <hre_api_internal.h>


#endif
