/*
 * font demo for Nano-X
 * also includes region clipping demo
 */
#include <stdio.h>
#include <stdlib.h>
#if UNIX | DOS_DJGPP
#include <time.h>
#endif
#define MWINCLUDECOLORS
#include "nano-X.h"

#define MAXW 		(630-50)
#define MAXH 		(470-50)

#define CLIP_POLYGON	0	/* =1 for polygonal region test*/

#if HAVE_HZK_SUPPORT
#define BIG5

#define MAXFONTS 1
#ifndef BIG5 
#define FONT1 "HZKFONT"
#define FONT2 "HZKFONT"
#define FONT3 "HZKFONT"
#define FONT4 "HZKFONT"
#define FONT5 "HZKFONT"
#else
#define FONT1 "HZXFONT"
#define FONT2 "HZXFONT"
#define FONT3 "HZXFONT"
#define FONT4 "HZXFONT"
#define FONT5 "HZXFONT"
#endif
#elif HAVE_T1LIB_SUPPORT
#define MAXFONTS 5
#define FONT1 "bchr"
#define FONT2 "bchb"
#define FONT3 "dcr10"
#define FONT4 "dcbx10"
#define FONT5 "bchri"
#else
/* truetype*/
#define MAXFONTS 5
#define FONT1 "lt1-r-omega-serif"
#define FONT2 "arial"
#define FONT3 "times"
#define FONT4 "cour"
#define FONT5 "timesi"
#endif

static char * names[5] = { FONT1, FONT2, FONT3, FONT4, FONT5 };

int main()
{
	GR_WINDOW_ID 	window;
	GR_EVENT 	event;
        GR_GC_ID 	gc;
	GR_FONT_ID	fontid;
        int 		i, x, y;
	GR_REGION_ID	regionid = 0;
#if CLIP_POLYGON
	GR_POINT	points[]={ {100, 100},
				{300, 100},
				{300, 300},
				{100, 300}};
#else
	GR_RECT		clip_rect={100,100,300,300};
#endif
   
        srand(time(0));
   
        GrOpen();
	window = GrNewWindow(GR_ROOT_WINDOW_ID, 50,50, MAXW,MAXH, 4, BLACK,BLUE);
	GrMapWindow(window);

        gc = GrNewGC();

#if CLIP_POLYGON
	/* polygon clip region*/
	regionid = GrNewPolygonRegion(MWPOLY_EVENODD, 3, points);
#else
	/* rectangle clip region*/
        regionid = GrNewRegion();
	GrUnionRectWithRegion(regionid, &clip_rect);
#endif

	GrSetGCRegion(gc, regionid);
	
        GrSelectEvents(window,GR_EVENT_MASK_ALL);
        GrSetGCUseBackground(gc,GR_FALSE);
	GrSetGCBackground(gc, GR_RGB(0, 0, 0));
	while(1) {
	      GrCheckNextEvent(&event);
	   
	      i = (int)((float)MAXFONTS * rand() / (RAND_MAX + 1.0));
	      fontid = GrCreateFont(names[i], 20, NULL);
	      GrSetFontSize(fontid, 1+(int)(80.0 * rand() / (RAND_MAX+1.0)));
	      GrSetFontRotation(fontid, 330);	/* 33 degrees*/
  	      GrSetFontAttr(fontid, GR_TFKERNING | GR_TFANTIALIAS, 0);
  	      GrSetGCFont(gc, fontid);
	      /*GrSetGCBackground(gc, rand() & 0xffffff);*/
 	      GrSetGCForeground(gc, rand() & 0xffffff);
	      x = (int) ((MAXW * 1.0) *rand()/(RAND_MAX+1.0));
	      y = (int) ((MAXH * 1.0) *rand()/(RAND_MAX+1.0));

#if HAVE_HZK_SUPPORT
             {	/* to test Unicode 16 chinese characters display ,use HZK font Bitmap font (Metrix font). */
#ifndef BIG5		
		char buffer[256];
		buffer[0]=0x6c;
		buffer[1]=0x49;
		buffer[2]=0x73;
		buffer[3]=0x8b;
		buffer[4]=0x79;
		buffer[5]=0xd1;
		buffer[6]=0x62;
		buffer[7]=0x80;
		buffer[8]=0x61;
		buffer[9]=0x00;
		buffer[10]=0x41;
		buffer[11]=0x00;

		buffer[12]=0x00;
		buffer[13]=0xa1;
		buffer[14]=0x00;
		buffer[15]=0xa6;
		buffer[16]=0x6c;
		buffer[17]=0x49;
		buffer[18]=0x0;
		buffer[19]=0x0;
		GrText(window, gc,x,y+20, buffer,17, GR_TFUC16);
		x=0;y=16;
		GrText(window, gc,x,y+20, buffer,17, GR_TFUC16);
#else
		unsigned short buffer[7];
		buffer[0]=0x9060;
		buffer[1]=0x898b;
		buffer[2]=0x79d1;
		buffer[3]=0x6280;
		buffer[4]=0x0061;
		buffer[5]=0x0041;
		buffer[6]=0x0;
		GrText(window, gc,x,y+20, buffer,7, GR_TFUC16);
		x=0;y=16;
		GrText(window, gc,x,y+20, buffer,7, GR_TFUC16);
#endif
	      }

#ifndef BIG5
	      x=0;y=16;
	      /* HZK Metrix font test, includes Chinese and English*/
	      GrText(window, gc,x,y, "Microwindows,欢迎使用中英文点阵字体",
		      -1, GR_TFASCII);
#else	
	      GrText(window, gc,x,y, "Microwindows,舧ㄏノい璣ゅ翴皚砰",
		      -1, GR_TFASCII);
	      x=0;y=16*3+4;
	      GrText(window, gc,x,y, "８９：", -1, GR_TFASCII);
#endif
	      GrFlush();

#else /* !HZK_FONT_SUPPORT*/

#if HAVE_BIG5_SUPPORT
	      /* ENCODING_BIG5 test*/
	      GrText(window, gc,x,y, "眃眃", -1, GR_TFASCII);
#else
#if HAVE_GB2312_SUPPORT
	      /* ENCODING_GB2312 test*/
	      GrText(window, gc,x,y, "\275\241\275\241", -1, GR_TFASCII);
#else
	      /* ASCII test*/
	      GrText(window, gc,x,y, "Microwindows", -1, GR_TFASCII);
#endif
#endif

#endif /* HZK_FONT_SUPPORT*/


	      GrDestroyFont(fontid);

		if(event.type == GR_EVENT_TYPE_CLOSE_REQ) {
			GrClose();
			exit(0);
		}
	}

	GrDestroyRegion(regionid);
	GrClose();
}
