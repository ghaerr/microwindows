 /*Base Header file 
 * This file is part of `TinyWidgets', a widget set for the nano-X GUI which is  * a part of the Microwindows project (www.microwindows.org).
 * Copyright C 2000
 * Sunil Soman <sunil_soman@vsnl.com>
 * Amit Kulkarni <amms@vsnl.net>
 * Navin Thadani <navs@vsnl.net>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation; either version 2.1 of the License, 
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _TNBASE_H_
#define _TNBASE_H_

#include "nano-X.h"

typedef struct widget TN_WIDGET;
typedef int TN_WIDGET_TYPE;     /*Type of Widget */
typedef void *DATA_POINTER;     /*Pointer to user data */
typedef int USER_EVENT;         /*Type of user event */

/*Function pointer later on used to define callbacks*/
typedef void (*CallBackFuncPtr) (TN_WIDGET *, DATA_POINTER);
typedef void (*DestroyFuncPtr) (TN_WIDGET *);
typedef void (*EventDispatchFuncPtr) (GR_EVENT *, TN_WIDGET *);
typedef struct
{
	  CallBackFuncPtr fp;
	    DATA_POINTER dptr;
}
CallBackStruct;

#endif /*_TNBASE_H_*/
