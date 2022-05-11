/*
* Header file for scr_djvesa.c driver
*
* June 2011 Georg Potthast
* based on examples from the DJGPP website
*
*/
#include <dpmi.h>
#include <go32.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <sys/farptr.h>

#pragma pack(1)

typedef struct VESA_INFO
   { 
      char  VESASignature[4] ;// __attribute__ ((packed));
      unsigned short VESAVersion          __attribute__ ((packed));
      unsigned long  OEMStringPtr         __attribute__ ((packed));
      unsigned char  Capabilities[4]  ;// __attribute__ ((packed));
      unsigned long  VideoModePtr         __attribute__ ((packed));
      unsigned short TotalMemory          __attribute__ ((packed));
      unsigned short OemSoftwareRev       __attribute__ ((packed));
      unsigned long  OemVendorNamePtr     __attribute__ ((packed));
      unsigned long  OemProductNamePtr    __attribute__ ((packed));
      unsigned long  OemProductRevPtr     __attribute__ ((packed));
      unsigned char  Reserved[222]    ;// __attribute__ ((packed));
      unsigned char  OemData[256]     ;// __attribute__ ((packed));
   } VESA_INFO;


   typedef struct MODE_INFO
   {
      unsigned short ModeAttributes       __attribute__ ((packed));
      unsigned char  WinAAttributes   ;// __attribute__ ((packed));
      unsigned char  WinBAttributes   ;// __attribute__ ((packed));
      unsigned short WinGranularity       __attribute__ ((packed));
      unsigned short WinSize              __attribute__ ((packed));
      unsigned short WinASegment          __attribute__ ((packed));
      unsigned short WinBSegment          __attribute__ ((packed));
      unsigned long  WinFuncPtr           __attribute__ ((packed));
      unsigned short BytesPerScanLine     __attribute__ ((packed));
      unsigned short XResolution          __attribute__ ((packed));
      unsigned short YResolution          __attribute__ ((packed));
      unsigned char  XCharSize        ;// __attribute__ ((packed));
      unsigned char  YCharSize        ;// __attribute__ ((packed));
      unsigned char  NumberOfPlanes   ;// __attribute__ ((packed));
      unsigned char  BitsPerPixel     ;// __attribute__ ((packed));
      unsigned char  NumberOfBanks    ;// __attribute__ ((packed));
      unsigned char  MemoryModel      ;// __attribute__ ((packed));
      unsigned char  BankSize         ;// __attribute__ ((packed));
      unsigned char  NumberOfImagePages;//__attribute__ ((packed));
      unsigned char  Reserved_page    ;// __attribute__ ((packed));
      unsigned char  RedMaskSize      ;// __attribute__ ((packed));
      unsigned char  RedMaskPos       ;// __attribute__ ((packed));
      unsigned char  GreenMaskSize    ;// __attribute__ ((packed));
      unsigned char  GreenMaskPos     ;// __attribute__ ((packed));
      unsigned char  BlueMaskSize     ;// __attribute__ ((packed));
      unsigned char  BlueMaskPos      ;// __attribute__ ((packed));
      unsigned char  ReservedMaskSize ;// __attribute__ ((packed));
      unsigned char  ReservedMaskPos  ;// __attribute__ ((packed));
      unsigned char  DirectColorModeInfo;//__attribute__ ((packed));
      unsigned long  PhysBasePtr          __attribute__ ((packed));
      unsigned long  OffScreenMemOffset   __attribute__ ((packed));
      unsigned short OffScreenMemSize     __attribute__ ((packed));
      unsigned char  Reserved[206]    ;// __attribute__ ((packed));
   } MODE_INFO;

//char bufferflag = 0;

VESA_INFO vesa_info;

int get_vesa_info()
/* read the vesa_info structure once the vesa mode is selected */
   {
      __dpmi_regs r;
      long dosbuf;
      int c;

      /* use the conventional memory transfer buffer */
      dosbuf = __tb & 0xFFFFF;

      /* initialize the buffer to zero */
      for (c=0; c<sizeof(struct VESA_INFO); c++)
	 _farpokeb(_dos_ds, dosbuf+c, 0);

      dosmemput("VBE2", 4, dosbuf);

      /* call the VESA function */
      r.x.ax = 0x4F00;
      r.x.di = dosbuf & 0xF;
      r.x.es = (dosbuf>>4) & 0xFFFF;
      __dpmi_int(0x10, &r);

      /* quit if there was an error */
      if (r.h.ah)
	 return -1;

      /* copy the resulting data into our structure */
      dosmemget(dosbuf, sizeof(struct VESA_INFO), &vesa_info);

      /* check that we got the right magic marker value */
      if (strncmp(vesa_info.VESASignature, "VESA", 4) != 0)
	 return -1;

	 return 0;
   }

MODE_INFO mode_info;

/*
VBE 2.0 provides a bank switching mechanism that can be used in a protected mode 
environment without the expensive switch to real mode. This is implemented as a 
small stub of relocatable 32 bit code provided by the VESA driver, which can be 
copied into your address space and then called directly to perform the bank switch, 
hardware scrolling, and palette setting functions. 
The 32 bit code stubs are obtained by calling VESA function 0x4F0A, see below
*/
typedef struct VESA_PM_INFO
   {
      unsigned short setWindow            __attribute__ ((packed));
      unsigned short setDisplayStart      __attribute__ ((packed));
      unsigned short setPalette           __attribute__ ((packed));
      unsigned short IOPrivInfo           __attribute__ ((packed));
   } VESA_PM_INFO;


VESA_PM_INFO *vesa_pm_info;

static void *pm_bank_switcher=0;


int get_vesa_pm_functions()
   {
/*
This code will give you a pointer to the protected mode bank switching function, 
but you cannot call this directly from C because it uses a special register based 
argument passing convention. A little bit of inline asm is needed to make sure the 
parameters go into the correct registers, see set_vesa_bank() below
*/
      __dpmi_regs r;

      /* check that the driver is at least VBE version 2.0 */
      if (vesa_info.VESAVersion < 0x200) 
	 return -1;

      /* call the VESA function */
      r.x.ax = 0x4F0A;
      r.x.bx = 0;
      __dpmi_int(0x10, &r);
      if (r.h.ah)
	 return -1;

      /* allocate space for the code stubs */
      vesa_pm_info = malloc(r.x.cx);

      /* copy the code into our address space */
      dosmemget(r.x.es*16+r.x.di, r.x.cx, vesa_pm_info);

      /* store a pointer to the bank switch routine */
      pm_bank_switcher = (void *)((char *)vesa_pm_info + vesa_pm_info->setWindow);
      return 0;
   }


/* Function prototype */
void vhline(short x1, short y1, short length, unsigned long c);



int get_mode_info(int mode)
/* read the mode_info structure once the vesa mode is selected */
   {
      __dpmi_regs r;
      long dosbuf;
      int c;

      /* use the conventional memory transfer buffer */
      dosbuf = __tb & 0xFFFFF;

      /* initialize the buffer to zero */
      for (c=0; c<sizeof(MODE_INFO); c++)
	 _farpokeb(_dos_ds, dosbuf+c, 0);

      /* call the VESA function */
      r.x.ax = 0x4F01;
      r.x.di = dosbuf & 0xF;
      r.x.es = (dosbuf>>4) & 0xFFFF;
      r.x.cx = mode;
      __dpmi_int(0x10, &r);

      /* quit if there was an error */
      if (r.h.ah)
	 return -1;

      /* copy the resulting data into our structure */
      dosmemget(dosbuf, sizeof(MODE_INFO), &mode_info);

      return 0;
   }

unsigned int find_vesa_mode(int w, int h, char bpp)
/* find and return the vesa NUMBER for the mode given by width, heigth and bitsperpixel */
   {
      int mode_list[256];
      int number_of_modes;
      long mode_ptr;
      int c;

      /* check that the VESA driver exists, and get information about it */
      if (get_vesa_info() != 0)
	 return 0;

      /* convert the mode list pointer from seg:offset to a linear address */
      mode_ptr = ((vesa_info.VideoModePtr & 0xFFFF0000) >> 12) + 
		  (vesa_info.VideoModePtr & 0xFFFF);

      number_of_modes = 0;

      /* read the list of available modes */
      while (_farpeekw(_dos_ds, mode_ptr) != 0xFFFF) {
	 mode_list[number_of_modes] = _farpeekw(_dos_ds, mode_ptr);
	 number_of_modes++;
	 mode_ptr += 2;
      }

      /* scan through the list of modes looking for the one that we want */
      for (c=0; c<number_of_modes; c++) {

	 /* get information about this mode */
	 if (get_mode_info(mode_list[c]) != 0)
	    continue;

	 /* check the flags field to make sure this is a color graphics mode,
	  * and that it is supported by the current hardware */
	 if ((mode_info.ModeAttributes & 0x19) != 0x19)
	    continue;

	 /* check that this mode is the right size */
	 if ((mode_info.XResolution != w) || (mode_info.YResolution != h))
	    continue;

	 /* check that there is only one color plane */
	 if (mode_info.NumberOfPlanes != 1)
	    continue;

	 /* check that it is a packed-pixel mode (other values are used for
	  * different memory layouts, eg. 6 for a truecolor resolution) */
	 //if (mode_info.MemoryModel != 4)
	 //   continue;

	 /* check that this is an 8-bit (256 color) mode */
	 if (mode_info.BitsPerPixel != bpp)
	    continue;

	 /* if it passed all those checks, this must be the mode we want! */
	 return mode_list[c];
      }

      /* there was no mode matching the one we wanted */
      return 0; 
   }

int set_vesa_mode(int w, int h, char bpp)
/* sets the vesa mode to a mode specified by width, height and bitsperpixel */
   {
      __dpmi_regs r;
      unsigned int mode_number;
	  
      /* find the number for this mode */
      mode_number = find_vesa_mode(w, h, bpp);
      if (!mode_number) return -1;

      /* call the VESA mode set function */
      r.x.ax = 0x4F02;
      r.x.bx = mode_number;
      __dpmi_int(0x10, &r);
      if (r.x.ax == 0x4F)
		{get_vesa_pm_functions();
	     return mode_number;
	    }  
	  if (r.h.ah == 0)
		{get_vesa_pm_functions();
		return mode_number;}  
	  
	  return -1; //none of the above - error
   }

int init_mode(unsigned short mode)
/* could use this to set screen mode directly by specifing the VESA number */
{
  __dpmi_regs regs;
  regs.x.ax = 0x4F02;
  regs.x.bx = mode;
  __dpmi_int(0x10, &regs);
  if(regs.x.ax == 0x4F)
  {return 1;}
  else
  {return 0;}
}

void done_VBE_mode(void)
/* return to text mode */
{
   union REGS regs;

   regs.h.ah=0x00;
   regs.h.al=0x03;
   int86(0x10, &regs, &regs);
}

void set_vesa_bank(int bank_number)
/* helper function */
   {
      __dpmi_regs r;
      static int curbank=0;

      if (bank_number == curbank) return;  
      curbank = bank_number;     

if (pm_bank_switcher == 0)  { //this branch if not vesa 2.0
      r.x.ax = 0x4F05;
      r.x.bx = 0;
      r.x.dx = bank_number;
      __dpmi_int(0x10, &r);
   }
else { //if vesa 2.0 use PM routine installed above with get_vesa_pm_functions()
    asm(
	"call *%0"
    :                             /* no outputs */

    :"r" (pm_bank_switcher),     /* function pointer in any register */
	"b" (0),                    /* set %ebx to zero */
	"d" (bank_number)           /* bank number in %edx */

    :"%ecx",                     /* clobber list, put unused regs in here */
    "%esi",                      /* "b"=ebx, "d"=edx, "r"->gcc will grab eax */
	"%edi"
	);
} /*if/else*/

} /*set vesa bank*/

/*
void putpixel(short x, short y, char c)
{
  long int addr = y * mode_info.BytesPerScanLine + x*mode_info.BitsPerPixel/8;
  set_vesa_bank((addr >> 16));
  _farpokeb(_dos_ds,0xA0000 + (addr & 0xFFFF), c);
  
  return;
}
*/

unsigned long getpixel(int x, int y)
/* read pixel color from vesa screen */
   {
   unsigned long color;

      long int address = y*mode_info.BytesPerScanLine+x*mode_info.BitsPerPixel/8;
      int bank_size = mode_info.WinGranularity*1024;
      int bank_number = address/bank_size;
      int bank_offset = address%bank_size;

      set_vesa_bank(bank_number);

	  switch (mode_info.BitsPerPixel)
	  {
	  case 32:
		color = _farpeekl(_dos_ds, 0xA0000+bank_offset);
		break;
	  case 16:
		color = _farpeekw(_dos_ds, 0xA0000+bank_offset);
		break;
	  case 8:
		color = _farpeekb(_dos_ds, 0xA0000+bank_offset);
	    break;
	  }
	  return color;
   }

void putpixel(int x, int y, unsigned long color)
/* write pixel on vesa screen */
   {
      long int address = y*mode_info.BytesPerScanLine+x*mode_info.BitsPerPixel/8;
      int bank_size = mode_info.WinGranularity*1024;
      int bank_number = address/bank_size;
      int bank_offset = address%bank_size;

      set_vesa_bank(bank_number);

      //color ^= getpixel(x,y);

	  switch (mode_info.BitsPerPixel)
	  {
	  case 32:
		_farpokel(_dos_ds, 0xA0000+bank_offset, color);
		break;
	  case 16:
		_farpokew(_dos_ds, 0xA0000+bank_offset, color & 0xFFFF);
		break;
	  case 8:
		_farpokeb(_dos_ds, 0xA0000+bank_offset, color & 0xFF);
	    break;
	  }
   }

void putpixelb(char *vesabuffer, int x, int y, unsigned long color)
/* write pixel into offscreen buffer */
   {
      long int address = y*mode_info.BytesPerScanLine+x*mode_info.BitsPerPixel/8;
      	  
	  switch (mode_info.BitsPerPixel)
	  {
	  case 32:
        vesabuffer[address]=color & 0xFF;
		vesabuffer[address+1]=(color >>8) & 0xFF;
		vesabuffer[address+2]=(color >>16) & 0xFF;
		vesabuffer[address+3]=(color >>24) & 0xFF;
		break;
	  case 16:
        vesabuffer[address]=color & 0xFF;
		vesabuffer[address+1]=color>>8;
		break;
	  case 8:
		vesabuffer[address]=(unsigned char)color;
	    break;
	  }
   }

void copy_to_vesa_screen(char *memory_buffer, int screen_size)
/* write from offscreen buffer to VESA screen*/
	{
      int bank_size = mode_info.WinSize*1024;
      int bank_granularity = mode_info.WinGranularity*1024;
      int bank_number = 0;
      int todo = screen_size;
      int copy_size;

      while (todo > 0) {
	 /* select the appropriate bank */
	 set_vesa_bank(bank_number);

	 /* how much can we copy in one go? */
	 if (todo > bank_size)
	    copy_size = bank_size;
	 else
	    copy_size = todo;

	 /* copy a bank of data to the screen */
	 dosmemput(memory_buffer, copy_size, 0xA0000);

	 /* move on to the next bank of data */
	 todo -= copy_size;
	 memory_buffer += copy_size;
	 bank_number += bank_size/bank_granularity;
      }
   }

void blit_to_vesa_screen(unsigned char *memory_buffer, int x, int y, int w, int h)
/* write block_size data from offscreen buffer to VESA screen beginning at any addr */
   {
      long int address = y*mode_info.BytesPerScanLine+x*mode_info.BitsPerPixel/8;
	  long int endaddress = (y+h-1)*mode_info.BytesPerScanLine+(x+w)*mode_info.BitsPerPixel/8;
      int bank_size = mode_info.WinGranularity*1024;
	  int bank_granularity = mode_info.WinGranularity*1024;
      int bank_number = address/bank_size;
      int bank_offset = address%bank_size;
      int todo = endaddress-address;
      int copy_size;
	  
      while (todo > 0) {
	 /* select the appropriate bank */
	 set_vesa_bank(bank_number);

	 /* how much can we copy in one go? */
	 if (todo > bank_size-bank_offset)
	    copy_size = bank_size-bank_offset;
	 else
	    copy_size = todo;

	 /* copy a bank of data to the screen */
	 dosmemput(memory_buffer, copy_size, 0xA0000+bank_offset);

	 /* move on to the next bank of data */
	 bank_offset=0;
	 todo -= copy_size;
	 memory_buffer += copy_size;
	 bank_number += bank_size/bank_granularity;
     } //while
   }

void rect(short x1, short y1, short x2, short y2, unsigned long c)
{
short x, y;
for (x=x1; x<(x2+1); x++)
  {
  putpixel(x, y1, c);
  putpixel(x, y2, c);
  }
for (y=y1; y<(y2+1); y++)
  {
  putpixel(x1, y, c);
  putpixel(x2, y, c);
  }
}

void filledrect(short x1, short y1, short x2, short y2, unsigned long c){

short y;

for (y=y1; y<(y2+1); y++)
  {
  vhline(x1, y, x2, c);
  }
}

void vhline(short x1, short y1, short length, unsigned long c){
/* write horizontal line on vesa screen */
short x;

for (x=x1; x<(length+1); x++)
  {
  putpixel(x, y1, c);
  }

}

void vvline(short x1, short y1, short length, unsigned long c){
/* write vertical line on vesa screen */
short y;

for (y=y1; y<(length+1); y++)
  {
  putpixel(x1, y, c);
  }
}


void line(short x, short y, short x2, short y2, unsigned long c) {
/* write line on vesa screen with any angle */  
  int i, steep = 0, sx, sy, dx, dy, e;

  dx = abs(x2 - x);
  sx = ((x2 - x) > 0) ? 1 : -1;
  dy = abs(y2 - y);
  sy = ((y2 - y) > 0) ? 1 : -1;

  if(dy > dx) {
    steep = 1;
    x ^= y;  // swap x and y
    y ^= x;
    x ^= y;
    dx ^= dy; // swap dx and dy
    dy ^= dx;
    dx ^= dy;
    sx ^= sy; // swap sx and sy
    sy ^= sx;
    sx ^= sy;
  }

  e = 2 * dy - dx;
  for(i = 0;i < dx;i++)	{
    if(steep) putpixel(y, x, c);
    else  putpixel(x, y, c);
    while(e >= 0) {
      y += sy;
      e -= 2 * dx;
    }
    x += sx;
    e += 2 * dy;
  }
  putpixel(x2, y2, c);
}

void vcircle(short xc, short yc, short r, unsigned long c) {
  int x = 0, y = r, d = 2 * (1 - r);

  while(y >= 0) {
    putpixel(xc + x, yc + y, c);
    putpixel(xc + x, yc - y, c);
    putpixel(xc - x, yc + y, c);
    putpixel(xc - x, yc - y, c);
    if(d + y > 0)  {
      y -= 1;
      d -= (2 * y) - 1;
    }
    if(x > d) {
    x += 1;
    d += (2 * x) + 1;
    }
  }
  return;
}

int set_vpalette_entry(int pnr,int count, MWPALENTRY *pal) {
/* set vesa palette entry - pal data= 0:red,1:blue,2:green,3:align-unused */
  __dpmi_regs regs;
    long dosbuf;
 
    /* use the conventional memory transfer buffer */
    dosbuf = __tb & 0xFFFFF;
    
	/* copy the palette data into dosbuf*/
    dosmemput(&pal[pnr], count*4, dosbuf);
 
    regs.x.ax = 0x4F09;
    regs.x.bx = 0; //set palette data
    regs.x.cx = count; //set this number of palette registers
    regs.x.dx = pnr; //(start) palette number
    regs.x.di = dosbuf & 0xF;
    regs.x.es = (dosbuf>>4) & 0xFFFF;
  __dpmi_int(0x10, &regs);
  
  if(regs.x.ax == 0x4F)
  {return 1;}
  else
  {return 0;}
}

int get_vpalette_entry(int pnr,int count, unsigned long *pal) {
/* get vesa palette entry - pal data= 0:red,1:blue,2:green,3:align-unused */
  __dpmi_regs regs;
    long dosbuf;
 
    /* use the conventional memory transfer buffer */
    dosbuf = __tb & 0xFFFFF;
    
    regs.x.ax = 0x4F09;
    regs.x.bx = 1; //get palette data
    regs.x.cx = count; //read this number of palette registers
    regs.x.dx = pnr; //(start) palette number
    regs.x.di = dosbuf & 0xF;
    regs.x.es = (dosbuf>>4) & 0xFFFF;
  __dpmi_int(0x10, &regs);
  
	/* copy the palette data out of dosbuf*/
    dosmemget(dosbuf, count*4, &pal[pnr]);
 
  if(regs.x.ax == 0x4F)
  {return 1;}
  else
  {return 0;}
}
