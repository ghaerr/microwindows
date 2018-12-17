 /*Header file for the Picture widget
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

#ifndef _TNPICTURE_H_
#define _TNPICTURE_H_
#define PICTURE_CALLBACKS 1 
#include "tnBase.h"

#define TN_PICTURE_HEIGHT 200 
#define TN_PICTURE_WIDTH  200

typedef struct
{
	char *filename;
	GR_IMAGE_ID imageid;
	GR_BOOL stretch;
	CallBackStruct CallBack[PICTURE_CALLBACKS];
}
TN_STRUCT_PICTURE;

typedef struct
{
	int height;		/* Image height in Pixels */
	int width;		/* Image width in Pixels */
	int bpp;		/* bits per pixel (1,4 or 8) */
	int bytesperpixel;	/* bytes per pixel */
}
TN_IMAGE_INFO;

void
CreatePicture(TN_WIDGET *,
	      TN_WIDGET *,
	      int,
	      int,
	      char *, GR_SIZE, GR_SIZE, GR_BOOL );
void DrawPicture (TN_WIDGET *);
void PictureEventHandler(GR_EVENT *,TN_WIDGET *);
int tnSetPictureStretch(TN_WIDGET *,GR_BOOL);
int tnSetPicture(TN_WIDGET *,char *);
void DestroyPicture(TN_WIDGET*);
int tnGetPictureProps(TN_WIDGET *,GR_BOOL *, char **);
int tnGetPictureImageProps(TN_WIDGET *, TN_IMAGE_INFO *);
int tnPictureDup(TN_WIDGET *,TN_WIDGET *);
#endif /*_TNPICTURE_H_*/
