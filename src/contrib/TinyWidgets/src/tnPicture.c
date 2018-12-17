 /*C Source file for the Picture widget
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

#include "../include/tnWidgets.h"
/*Create Picture widget*/
void
CreatePicture(TN_WIDGET * widget,
	      TN_WIDGET * parent,
	      int posx,
	      int posy,
	      char *filename,
	      GR_SIZE height,
	      GR_SIZE width,
	      GR_BOOL stretch)
{
  int i;
  if (height == 0) 
      height = TN_PICTURE_HEIGHT;
  if(width==0)
      width = TN_PICTURE_WIDTH;
  widget->WSpec.picture.imageid = 0;    
  if(filename != NULL)
      {
	  widget->WSpec.picture.filename =
	   (char *)malloc((strlen(filename)+1)*sizeof(char));    
  	  strcpy ((widget->WSpec.picture.filename), filename);
	  widget->WSpec.picture.imageid = 
	    GrLoadImageFromFile(filename,0);
      }
  else
	  widget->WSpec.picture.filename = NULL;    
	  
  widget->WSpec.picture.stretch = stretch;    
  for (i = 0; i < PICTURE_CALLBACKS; i++)
        widget->WSpec.picture.CallBack[i].fp = NULL;
  widget->wid =
    GrNewWindow (parent->wid, posx, posy, width, height, 0,0, 0);
  GrSetGCUseBackground(widget->gc,GR_FALSE);
  

  GrSelectEvents (widget->wid,GR_EVENT_MASK_BUTTON_UP|GR_EVENT_MASK_BUTTON_DOWN|GR_EVENT_MASK_EXPOSURE);
  return;
}

/*Event Handler for the Picture*/
void
PictureEventHandler (GR_EVENT * event, TN_WIDGET * widget)
{
  switch (event->type)
    {
    case GR_EVENT_TYPE_BUTTON_UP:
      break;
    case GR_EVENT_TYPE_BUTTON_DOWN:
      if (widget->WSpec.picture.CallBack[CLICKED].fp)
	      (*(widget->WSpec.picture.CallBack[CLICKED].fp)) (widget,widget->WSpec.picture.CallBack[CLICKED].dptr);
      break;
      
    case GR_EVENT_TYPE_EXPOSURE:
      DrawPicture (widget);
      break;
    }
}


/*Picture Draw routine*/
void
DrawPicture (TN_WIDGET * picture)
{
  GR_WINDOW_INFO winfo;
  GR_IMAGE_INFO iinfo;
  if(picture->WSpec.picture.imageid == 0)
	  return;
  GrGetWindowInfo (picture->wid, &winfo);
  GrGetImageInfo(picture->WSpec.picture.imageid,&iinfo);
  if(picture->WSpec.picture.stretch)
  {
	  GrResizeWindow(picture->wid,iinfo.width,iinfo.height);
	  GrGetWindowInfo(picture->wid,&winfo);
  }
  GrDrawImageToFit(picture->wid,picture->gc,0,0,winfo.width,winfo.height,picture->WSpec.picture.imageid);
  /*  GrDrawImageFromFile(picture->wid,picture->gc,0,0,winfo.width,winfo.height,picture->WSpec.picture.filename,0);*/
  return;
}

int 
tnSetPictureStretch(TN_WIDGET *widget,GR_BOOL stretch)
{
	if(widget->type != TN_PICTURE)
		return -1;
	widget->WSpec.picture.stretch = stretch;
	GrClearWindow(widget->wid,GR_FALSE);
	DrawPicture(widget);
	return 1;
}

int 
tnSetPicture(TN_WIDGET *widget ,char *filename)
{
         if(widget->type != TN_PICTURE)
		return -1;
	 if(filename == NULL)
		 return -1;	
	 free(widget->WSpec.picture.filename); 	
	 widget->WSpec.picture.filename =
	            (char *)malloc((strlen(filename)+1)*sizeof(char));
	 strcpy ((widget->WSpec.picture.filename), filename);
	 GrFreeImage(widget->WSpec.picture.imageid);
         widget->WSpec.picture.imageid =
             GrLoadImageFromFile(filename,0);
	 GrClearWindow(widget->wid,GR_FALSE);
	 DrawPicture(widget);
	 return 1;
}

void DestroyPicture(TN_WIDGET *widget)
{
	free(widget->WSpec.picture.filename);
	DeleteFromRegistry(widget);
	return;
}

int
tnPictureDup(TN_WIDGET *dest,TN_WIDGET *src)
{
	if(!dest || dest->type!=TN_PICTURE)
		return -1;
	if(!src || src->type!=TN_PICTURE)
		return -1;
	dest->WSpec.picture.imageid = src->WSpec.picture.imageid;
	GrClearWindow(dest->wid,GR_TRUE);
	return 1;
}

int
tnGetPictureProps(TN_WIDGET *widget,GR_BOOL *stretch, char **filename)
{
	 if(widget->type != TN_PICTURE)
		 return -1;
	 *stretch = widget->WSpec.picture.stretch;
	 if(widget->WSpec.picture.filename)
		 *filename = strdup(widget->WSpec.picture.filename);
	 return 1;
}

int 
tnGetPictureImageProps(TN_WIDGET *widget, TN_IMAGE_INFO *iinfo)
{
	
	 GR_IMAGE_INFO griinfo;
	 if(widget->type != TN_PICTURE || iinfo == NULL )
		 return -1;
	 if(widget->WSpec.picture.imageid == 0)
		 return -1;
	 GrGetImageInfo(widget->WSpec.picture.imageid, &griinfo);
	 iinfo->height = griinfo.height;
	 iinfo->width = griinfo.width;
	 iinfo->bpp = griinfo.bpp;
	 iinfo->bytesperpixel = 4; //griinfo.bytesperpixel;		// FIXME
	 return 1;
}
