#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if UNIX
#include <time.h>
#endif
#define MWINCLUDECOLORS
#include "nano-X.h"
/*
 * logical font demo for Nano-X
 */

#define MAXW 		630
#define MAXH 		470

int main(int argc, char **argv)
{
	GR_WINDOW_ID 	window;
	GR_EVENT 	event;
        GR_GC_ID 	gc;
	GR_FONT_ID	fontid;
        int 		x, y, rnd = 0;
	MWLOGFONT	lf;
	char		description[128];
   
        srand(time(0));
   
        GrOpen();
	window = GrNewWindow(GR_ROOT_WINDOW_ID, 5, 5, MAXW, MAXH, 4, BLACK, BLUE);
	GrMapWindow(window);

        gc = GrNewGC();

        GrSelectEvents(window,GR_EVENT_MASK_ALL);
        GrSetGCUseBackground(gc,GR_FALSE);
	GrSetGCBackground(gc, GR_RGB(0, 0, 0));

	y = 30;
	x = 0;

	while(1) {
	      GrCheckNextEvent(&event);

	      if(event.type == GR_EVENT_TYPE_CLOSE_REQ) {
			GrClose();
			exit(0);
	      }

	      /*sleep(1);*/

	      MWLF_Clear(&lf);
	      description[0] = '\0';
	      /*lf.lfSerif = 1;*/

	      if ( rnd & 1 ) {
		      lf.lfWeight = MWLF_WEIGHT_BOLD;
		      strcat(description,"Bold ");
	      }
	      

	      if ( rnd & 2 ) {
		      lf.lfItalic = 1;
		      strcat(description,"Italics ");
	      }
	      if ( rnd & 4 ) {
		      lf.lfOblique = 1;
		      strcat(description,"Oblique ");
	      }

	      if ( rnd & 8 ) {
		      lf.lfMonospace = 1;
		      strcat(description,"Monospace ");
	      } else {
		      lf.lfProportional = 1;
		      strcat(description,"Proportional ");
	      }

	      if ( argc > 1 )
		      strcpy(lf.lfFaceName,argv[1]);
	      else
		      strcpy(lf.lfFaceName,"fantasy");

	      fontid = GrCreateFont(0, 0, &lf);
	      /* GrSetFontSize(fontid, 1+(int)(80.0 * rand() / (RAND_MAX+1.0))); */
	      GrSetFontSize(fontid,26);
	      GrSetFontRotation(fontid, 330);	/* 33 degrees*/
  	      GrSetFontAttr(fontid, GR_TFKERNING | GR_TFANTIALIAS, 0);
  	      GrSetGCFont(gc, fontid);
	      /*GrSetGCBackground(gc, rand() & 0xffffff);*/
 	      GrSetGCForeground(gc, 0xffffff);
	      /* x = (int) ((MAXW * 1.0) *rand()/(RAND_MAX+1.0));
		 y = (int) ((MAXH * 1.0) *rand()/(RAND_MAX+1.0)); */

	      GrText(window, gc,x,y, description, -1, GR_TFASCII);

	      GrDestroyFont(fontid);

	      rnd++;
	      y += 30;
	      if ( y > 460 )
		      y = 0;
	}

	GrClose();
}
