/*
 *
 * Screen Driver using DJGPP & VESA  Library by Georg Potthast
 *
 * reads the environment variable NANOSCR to allow to specify settings after 
 * compilation, 
 * NANOSCR=[SCREEN_WIDTH] [SCREEN_HEIGHT] [SCREEN_PIXTYPE]
 * valid PIXTYPEs = 8888 for 32bit, 888 for 24bit, 565 for 16bit, 8 for 8bit
 * e.g. "NANOSCR=800 600 565" for TRUECOLOR565
 *
 * 555 and 8 bit apparently do not work - GP
 */

#include <stdio.h>
#include <stdlib.h>
#include "uni_std.h" //for dup
#include "device.h"  
#include "genfont.h" 
#include "genmem.h"
#include "fb.h"

#include "djvesa.h"

static PSD  vesa_open(PSD psd);
static void vesa_close(PSD psd);
static void vesa_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void vesa_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static void vesa_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height);

void toggle_text_mode();

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1024
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 768
#endif

#ifndef SCREEN_PIXTYPE
#define SCREEN_PIXTYPE MWPF_TRUECOLORARGB //32
#endif

//these are set using nanoscr - 1 stdout, 2 stderr, 3 toggle_text_mode
int NANOX_REDIRECT_STDOUT=0; //no redirection for default
int NANOX_REDIRECT_STDERR=0; //no redirection for default
int START_TOGGLE_TEXT_MODE=0; //startup in text mode for debugging

//need these global variables for GDB support
int set_text_mode_for_gdb=0;
unsigned char *saved_addr;
int vwidth,vheight,vbpp;
char std_out;

//for redirection of stdout to NUL
static int old_stdout; 
static int old_stderr;
static int nulfilenr;
static FILE* nulfile;

SUBDRIVER subdriver;

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	vesa_open,
	vesa_close,
	vesa_setpalette,       
	vesa_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,               /*gen_setportrait*/
	vesa_update,		/* Update*/
	NULL				/* PreSelect*/
};

/*
**	Open graphics
*/
static PSD
vesa_open(PSD psd)
{
	PSUBDRIVER subdriver;
	int vbemode;
   
	vwidth = SCREEN_WIDTH;
	vheight = SCREEN_HEIGHT;
	if(SCREEN_PIXTYPE == MWPF_TRUECOLORARGB) {
		vbpp=32;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLORRGB) {
		vbpp=24;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR565)  {
		vbpp=16;
	} else if(SCREEN_PIXTYPE == MWPF_TRUECOLOR555)  {
		vbpp=15;
	} else {
		vbpp=8; //palette
	}

	//now check if the environment variable NANOSCR is specified and take the
	//screen resolution and pixel type from that if available
	char *nanoscr = (char *)getenv("NANOSCR");

	if (nanoscr != NULL) { //if environment variable found
		// " %d" - blank is important, it eats blanks in front
		sscanf(nanoscr, "%d %d %d %c", &vwidth, &vheight, &vbpp, &std_out); //vbpp as tmp variable here
		if(vbpp == 8888) {
			vbpp=32;
		} else if(vbpp == 888) {
			vbpp=24;
		} else if(vbpp == 565)  {
			vbpp=16;
		} else if(vbpp == 555)  {
			vbpp=15;
		} else if(vbpp == 8)  {
			vbpp=8; //palette
		} else {
			vbpp=32; //default to 8888
		}
		if (std_out == '1') NANOX_REDIRECT_STDOUT=1;
		if (std_out == '2') NANOX_REDIRECT_STDERR=1;
		if (std_out == '3') START_TOGGLE_TEXT_MODE=1;
	}
	
   //find VESA mode for these parameters
   vbemode = set_vesa_mode(vwidth,vheight,vbpp);
   if (vbemode == -1) {
	   //try falling back to 1024x768 and 32 bit
	    done_VBE_mode(); /* set to text mode again first*/
		vwidth=1024;
		vheight=768;
		vbpp=32;
		vbemode = set_vesa_mode(vwidth,vheight,vbpp);
		if (vbemode == -1) {
			//try falling back to 800x600 and 16 bit
			done_VBE_mode(); /* set to text mode again first*/
			vwidth=800;
			vheight=600;
			vbpp=16;
			vbemode = set_vesa_mode(vwidth,vheight,vbpp);
	   
			if (vbemode == -1) { //still no valid mode?
			DPRINTF("Invalid screen resolution or pixeltype specified\n");
			return NULL; //no valid mode found
			}
		} //second vbemode == -1
	} //first vbemode == -1

   //read in the mode_info structure for the selected VESA mode
   get_mode_info(vbemode);
	psd->xres = psd->xvirtres = mode_info.XResolution;
	psd->yres = psd->yvirtres = mode_info.YResolution;
	psd->planes = 1;
	psd->bpp = mode_info.BitsPerPixel;
	psd->ncolors = psd->bpp >= 24 ? (1 << 24) : (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_ADDRMALLOC;
	/* Calculate the correct size and linelen here */
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp,
		&psd->size, &psd->pitch);
    if(psd->bpp == 32) {
		psd->pixtype = MWPF_TRUECOLORARGB;	
	} else if(psd->bpp == 16) {
		psd->pixtype = MWPF_TRUECOLOR565; 
	} else if(psd->bpp == 15) {
		psd->pixtype = MWPF_TRUECOLOR555; 
	} else if(psd->bpp == 24)  {
		psd->pixtype = MWPF_TRUECOLORRGB;
	} else {
		psd->pixtype = MWPF_PALETTE;
	}
  psd->portrait = MWPORTRAIT_NONE;
  psd->data_format = set_data_format(psd);

  /*
   * set and initialize subdriver into screen driver
   * psd->size is calculated by subdriver init
   */
  subdriver = select_fb_subdriver(psd);
  psd->orgsubdriver = subdriver;
  set_subdriver(psd, subdriver);
  if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;

  saved_addr=psd->addr; //for GDB support
  
  /*
  Now open the NUL device as a file and use dup() i.e. DOS 0x45
  and dup2() i.e. DOS 0x46 to redirect stdout to the NUL device.
  Output to text mode while in VESA mode will corrupt the display.
  */
  if (NANOX_REDIRECT_STDOUT == 1){
  nulfile = fopen ("nul", "w");
  if (nulfile != NULL){ //NUL device successfully opened
        old_stdout = dup(fileno(stdout)); //save handle of stdout for restore
        nulfilenr = fileno(nulfile); //need the handle
		//redirect stdout to NULL device
		dup2(nulfilenr,fileno(stdout));
  }
  } // redirect_on
  if (NANOX_REDIRECT_STDERR == 1){
        old_stderr = dup(fileno(stderr)); //save handle of stdout for restore
        //redirect stderr to stdout
		dup2(fileno(stdout),fileno(stderr));
  }
  //} // redirect_on

  //do it here to allow to restore screen with alt-comma
  if (START_TOGGLE_TEXT_MODE==1) toggle_text_mode();

  return psd;
}

/*
**	Close graphics
*/
static void
vesa_close(PSD psd)
{
	if (nulfile != NULL){ //NUL device had been successfully opened
		//remove redirection of stdout by forcing stdout to track the duplicate
		//in old_stdout
		dup2(old_stdout,fileno(stdout));
        close(old_stdout); //close duplicate - no longer needed
    }
	if (NANOX_REDIRECT_STDERR==1){
		dup2(old_stderr,fileno(stderr));
        close(old_stderr); //close duplicate - no longer needed
    }
  
	/* free framebuffer memory */
	free(psd->addr);
	done_VBE_mode();
}

/*
**	Get Screen Info
*/
static void
vesa_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	gen_getscreeninfo(psd, psi);

	if(scrdev.yvirtres > 800) {
		/* SVGA 1400x1050*/
		psi->xdpcm = 58;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 58;	/* assumes screen height of 18 cm*/        
	} else if(scrdev.yvirtres > 600) {
		/* SVGA 1024x768*/
		psi->xdpcm = 42;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 42;	/* assumes screen height of 18 cm*/        
	} else if(scrdev.yvirtres > 480) {
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if(scrdev.yvirtres > 350) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
}

/*
**	Set Palette
*/
static void
vesa_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{ /* this function is untested */ 
	set_vpalette_entry(first,count, &pal[0]);  
}
/*
**	Blit
*/
static void
vesa_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
	if (set_text_mode_for_gdb == 1) return; //no update if text mode enabled

	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

	MWCOORD x,y;
	MWPIXELVAL c;

/* got to read from psd->addr and write with putpixel()*/

	//if (!((width == 1) || (height == 1))) return;
	//printf("U: %d %d %d %d ",destx,desty,width,height);

if (psd->pixtype == MWPF_TRUECOLOR332)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
        //use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				c = addr[x];
				putpixel(destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		}
	}
else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
	{	
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);
		//use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*2; x++) {
				MWPIXELVAL c = ((unsigned short *)addr)[x]; //((ADDR16)addr)[x];
				putpixel(destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		}
	}
else if (psd->pixtype == MWPF_TRUECOLORRGB)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx * 3;
		//use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		unsigned int extra = psd->pitch - width * 3;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*3; x++) {
				MWPIXELVAL c = RGB2PIXEL888(addr[2], addr[1], addr[0]);
				putpixel(destx+x, desty+y, c); 
				addr += 3;
			}
			addr += extra; 
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		} 
	}
else if (((MWPIXEL_FORMAT == MWPF_TRUECOLORARGB) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)) & (psd->bpp != 8))
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx <<2);
		//use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		//if (TRUE) {  
		if (width == 1) {  
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x++) {				
				MWPIXELVAL c = ((unsigned long *)addr)[x]; //MWPIXELVAL=uint32_t
				putpixel(destx+x, desty+y, c);
			}
			addr += psd->pitch;     
		} //for  
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		} 
	} 
else /* MWPF_PALETTE*/ 
	{ 
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		//printf (" psd-addr %d addr %d desty:%d destx:%d width:%d height:%d\n",(int)psd->addr,(int)addr,desty,destx,width,height);
		
		//use putpixel for single pixel writes and blit_to_vesa_screen for data blocks
		if (width == 1) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				MWPIXELVAL c = ((unsigned short *)addr)[x];
				putpixel(destx+x, desty+y, c);
			}
			addr += psd->pitch;
		} //for
		} else { 
		blit_to_vesa_screen(addr, destx, desty, width, height);
		}
	}

}

/* to allow for debugging with GDB in text mode, ALT-Comma will toggle into text mode and back */
void toggle_text_mode(){
	if (set_text_mode_for_gdb == 0){
		if (NANOX_REDIRECT_STDOUT == 1) dup2(old_stdout,fileno(stdout)); //remove redirection of stdout
		if (NANOX_REDIRECT_STDERR == 1) dup2(old_stderr,fileno(stderr)); //remove redirection of stderr
		done_VBE_mode(); /* return to text mode */
		set_text_mode_for_gdb = 1;
	} 
	else {
        if (NANOX_REDIRECT_STDOUT == 1) dup2(nulfilenr,fileno(stdout)); //redirect stdout again
		if (NANOX_REDIRECT_STDERR == 1) dup2(fileno(stdout),fileno(stderr)); //redirect stderr again
		set_vesa_mode(vwidth,vheight,vbpp);
		//if (set_vesa_mode(vwidth,vheight,vbpp) == -1) {
		//	DPRINTF("Invalid screen resolution or pixeltype specified\n");
		//	return NULL;} //no valid mode found
        blit_to_vesa_screen(saved_addr, 1, 1, vwidth, vheight); //update entire screen
		set_text_mode_for_gdb = 0;
	}
}
