/*
 * Copyright (c) 2000, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Supports dynamically loading HZK font files
 * Han Zi Ku routines contributed by Tanghao and Jauming
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"
#include "devfont.h"

#ifndef HZK_FONT_DIR
#define HZK_FONT_DIR	"fonts/chinese"	/* default dir for {asc,hzk,hzx}{12,16} and *.KU files*/
#endif

/*
 * 12x12 and 16x16 ascii and chinese fonts
 * Big5 and GB2312 encodings supported
 */
#define MAX_PATH	256
typedef struct {
	int	width;
	int	height;
	int	size;
	unsigned long use_count;
	char *	pFont;
	char	file[MAX_PATH + 1];
} HZKFONT;

static int use_big5=1;
static HZKFONT CFont[2];	/* font cache*/
static HZKFONT AFont[2];	/* font cache*/

/* jmt: moved inside MWHZKFONT*/
static int afont_width = 8;
static int cfont_width = 16;
static int font_height = 16;
static char *afont_address;
static char *cfont_address;

typedef struct MWHZKFONT {
	PMWFONTPROCS fontprocs;	/* common hdr*/
	MWCOORD		fontsize;
	MWCOORD		fontwidth;
	int			fontrotation;
	int			fontattr;		

	HZKFONT 	CFont;		/* hzkfont stuff */
	HZKFONT 	AFont;
	int 		afont_width;
	int 		cfont_width;
	int 		font_height;
	char 		*afont_address;
	char 		*cfont_address;
} MWHZKFONT, *PMWHZKFONT;

static int  hzk_init(PSD psd);
PMWFONT hzk_createfont(const char *name, MWCOORD height, MWCOORD width, int fontattr);

static MWBOOL hzk_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo);
static void hzk_gettextsize(PMWFONT pfont, const void *text, int cc,
		MWTEXTFLAGS flags, MWCOORD *pwidth, MWCOORD *pheight,
		MWCOORD *pbase);
#if 0
static void hzk_gettextbits(PMWFONT pfont, int ch, const IMAGEBITS **retmap,
		MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase);
#endif
static void hzk_destroyfont(PMWFONT pfont);
static void hzk_drawtext(PMWFONT pfont, PSD psd, MWCOORD x, MWCOORD y,
		const void *text, int cc, MWTEXTFLAGS flags);
		
/* handling routines for MWHZKFONT*/
static MWFONTPROCS hzk_procs = {
	0,					/* can't scale*/
	MWTF_ASCII,			/* routines expect ASCII*/
	hzk_init,
	hzk_createfont,
	hzk_getfontinfo,
	hzk_gettextsize,
	NULL,				/* hzk_gettextbits*/
	hzk_destroyfont,
	hzk_drawtext,
	NULL,				/* setfontsize*/
	NULL, 				/* setfontrotation*/
	NULL,				/* setfontattr*/
	NULL				/* duplicate*/
};

/* temp extern decls*/
extern MWPIXELVAL gr_foreground;
extern MWPIXELVAL gr_background;

/* UniCode-16 (MWTF_UC16) to GB(MWTF_ASCII) Chinese Characters conversion.
 * a single 2-byte UC16 character is encoded into a surrogate pair.
 * return -1 ,if error;
 * The destination array must be able to hold as many
 * as there are Unicode-16 characters.
 *
 * Copyright (c) 2000 Tang Hao (TownHall)(tang_hao@263.net).
 */
int
UC16_to_GB(const unsigned char *uc16, int cc, unsigned char *ascii)
{
	FILE* fp;
	char buffer[256];
	unsigned char *uc16p;
	int i=0,j=0, k;
	unsigned char *filebuffer;
	unsigned short *uc16pp,*table;
	unsigned short uc16px;
	int length=31504;

	if (use_big5)
		length=54840;

    	uc16p=(unsigned char *) uc16;
	uc16pp=(unsigned short *) uc16;

	strcpy(buffer,HZK_FONT_DIR);
	if (use_big5)
    		strcat(buffer,"/BG2UBG.KU");
	else
    		strcat(buffer,"/UGB2GB.KU");
	if(!(fp = fopen(buffer, "rb"))) 
	{
   	  	 printf ("Error.\nThe %s file can not be found!\n",buffer);
   		 return -1;
    	}

	filebuffer= (unsigned char *)malloc ( length);

	if(fread(filebuffer, sizeof(char),length, fp) < length) {
	   	  printf ("Error in reading ugb2gb.ku file!\n");
	   	  fclose(fp);
 	     	  return -1;
	}
    	fclose(fp);

	if (use_big5)
	{
		table=(unsigned short *)filebuffer;
		while(1)
		{
			if(j>=cc)
			{
				ascii[i]=0;
				break;
			}
			uc16px=*uc16pp;
			if((uc16px)<=0x00ff)
			{
				ascii[i]=(char)(*uc16pp);
				i++;
			}
			else
			{
				ascii[i]=0xa1; ascii[i+1]=0x40;
				for (k=0; k<13710; k++)
				{
					if (*(table+(k*2+1))==(uc16px))
					{
						ascii[i]=(char)((*(table+(k*2)) & 0xff00) >> 8);
						ascii[i+1]=(char)(*(table+(k*2)) & 0x00ff);
						break;
					}
				}
				i+=2;
			}
			uc16pp++; j++;
		}
	}
	else
	{
	while(1)
	{
		if(j>=cc)
		{
			ascii[i]=0;
			break;
		}
		if((*((uc16p)+j)==0)&&(*((uc16p)+j+1)==0))
		{
			ascii[i]=0;
			break;
		}
		else
		{
			if(*((uc16p)+j+1)==0)
			{
				ascii[i]=*((uc16p)+j);
				i++;
				j+=2;
			}
			else
			{
			/* to find the place of unicode charater .¶þ·Ö·¨Æ¥Åä*/
            		{
				int p1=0,p2=length-4,p;
				unsigned int c1,c2,c,d;
				c1=((unsigned int )filebuffer[p1])*0x100+(filebuffer[p1+1]);
                		c2=((unsigned int )filebuffer[p2])*0x100+(filebuffer[p2+1]);
				d=((unsigned int )*((uc16p)+j))*0x100+*((uc16p)+j+1);
                		if(c1==d)
				{
					ascii[i]=filebuffer[p1+2];
					ascii[i+1]=filebuffer[p1+3];
					goto findit;
 	            		}
                		if(c2==d)
				{
					ascii[i]=filebuffer[p2+2];
					ascii[i+1]=filebuffer[p2+3];
					goto findit;
                		}
				while(1)
				{
					p=(((p2-p1)/2+p1)>>2)<<2;
					c=((unsigned int )filebuffer[p])*0x100+(filebuffer[p+1]);
					if(d==c)	/* find it*/
					{
						ascii[i]=filebuffer[p+2];
						ascii[i+1]=filebuffer[p+3];
						break;
          	   	   		}
					else if(p2<=p1+4)	/* can't find.*/
					{
						ascii[i]='.';	/* ((uc16p)+j);*/
						ascii[i+1]='.';	/* ((uc16p)+j+1);*/
						break;
					}
					else if(d<c)
					{
						p2=p;
						c2=c;										
                  			}
					else
					{
						p1=p;
						c1=c;														
					}
				}
	            	}
			findit:
  			i+=2;
			j+=2;
			}
		}
	}
	}
	free(filebuffer);

	return i;
}

/* ************************ functions definition ******************************/

static int hzk_id( PMWHZKFONT pf )
{
	switch(pf->font_height) {
	case 12:
		return 0;
	case 16: default:
		return 1;
	}
}

/* This function get Chinese font info from etc file.*/
static MWBOOL GetCFontInfo( PMWHZKFONT pf )
{
	int charset;

	if (use_big5)
		charset=(13094+408);
	else
		charset=8178;

    	CFont[hzk_id(pf)].width = pf->cfont_width;
    	pf->CFont.width = pf->cfont_width;

    	CFont[hzk_id(pf)].height = pf->font_height;
    	pf->CFont.height = pf->font_height;

    	CFont[hzk_id(pf)].size = ((pf->CFont.width + 7) / 8) *
		pf->CFont.height * charset;
    	pf->CFont.size = ((pf->CFont.width + 7) / 8) * pf->CFont.height * charset;

    	if(pf->CFont.size < charset * 8)
        	return FALSE;

	strcpy(CFont[hzk_id(pf)].file,HZK_FONT_DIR);
	strcpy(pf->CFont.file,HZK_FONT_DIR);

	if(pf->font_height==16)
	{
		strcat(CFont[hzk_id(pf)].file,"/hzk16");
		strcat(pf->CFont.file,"/hzk16");
	}
	else
	{
		strcat(CFont[hzk_id(pf)].file,"/hzk12");
		strcat(pf->CFont.file,"/hzk12");
	}

    	if (use_big5)
    	{
		CFont[hzk_id(pf)].file[strlen(pf->CFont.file)-3]+=use_big5;
		pf->CFont.file[strlen(pf->CFont.file)-3]+=use_big5;
    	}

    	return TRUE;
}

/* This function get ASCII font info from etc file.*/
static MWBOOL GetAFontInfo( PMWHZKFONT pf )
{
    	AFont[hzk_id(pf)].width = pf->afont_width;
    	pf->AFont.width = pf->afont_width;

    	AFont[hzk_id(pf)].height = pf->font_height;
    	pf->AFont.height = pf->font_height;

    	AFont[hzk_id(pf)].size = ((pf->AFont.width + 7) / 8) *
		pf->AFont.height * 255;
    	pf->AFont.size = ((pf->AFont.width + 7) / 8) * pf->AFont.height * 255;
    
	if(pf->AFont.size < 255 * 8)
        	return FALSE;

	strcpy(AFont[hzk_id(pf)].file,HZK_FONT_DIR);
	strcpy(pf->AFont.file,HZK_FONT_DIR);
	
	if(pf->font_height==16)
	{
	    	strcat(AFont[hzk_id(pf)].file,"/asc16");
	    	strcat(pf->AFont.file,"/asc16");
	}
  	else
	{
	    	strcat(AFont[hzk_id(pf)].file,"/asc12");
	    	strcat(pf->AFont.file,"/asc12");
	}
    	return TRUE;
}

/* This function load system font into memory.*/
static MWBOOL LoadFont( PMWHZKFONT pf )
{
    	FILE* fp;

	if(!GetCFontInfo(pf))
	{
		printf ("Get Chinese HZK font info failure!\n");
		return FALSE;
	}
    	if(CFont[hzk_id(pf)].pFont == NULL)	/* check font cache*/
	{
		

 	   	/* Allocate system memory for Chinese font.*/
 		if( !(CFont[hzk_id(pf)].pFont = (char *)malloc(pf->CFont.size)) )
 		{
	 	       	printf ("Allocate memory for Chinese HZK font failure.\n");
		        return FALSE;
	 	}
 	
		/* Open font file and read information to the system memory.*/
 		printf ("hzk_createfont: loading '%s'\n", pf->CFont.file);
		if(!(fp = fopen(CFont[hzk_id(pf)].file, "rb"))) 
		{
   		  	printf ("Error.\nThe Chinese HZK font file can not be found!\n");
   	    	 	return FALSE;
    		}
	    	if(fread(CFont[hzk_id(pf)].pFont, sizeof(char), pf->CFont.size, fp) < pf->CFont.size) 
		{
	      	  	printf ("Error in reading Chinese HZK font file!\n");
	 	     	fclose(fp);
 	       		return FALSE;
		}

		fclose(fp);

		CFont[hzk_id(pf)].use_count=0;

	}
	cfont_address = CFont[hzk_id(pf)].pFont;
	pf->cfont_address = CFont[hzk_id(pf)].pFont;
	pf->CFont.pFont = CFont[hzk_id(pf)].pFont;

	CFont[hzk_id(pf)].use_count++;

	if(!GetAFontInfo(pf))
	{
	       printf ("Get ASCII HZK font info failure!\n");
	       return FALSE;
	}
    	if(AFont[hzk_id(pf)].pFont == NULL)	/* check font cache*/
	{
		
 		
 		/* Allocate system memory for ASCII font.*/
 		if( !(AFont[hzk_id(pf)].pFont = (char *)malloc(pf->AFont.size)) )
 		{
 		       	printf ("Allocate memory for ASCII HZK font failure.\n");
 		       	free(CFont[hzk_id(pf)].pFont);
 		       	CFont[hzk_id(pf)].pFont = NULL;
			return FALSE;
 		}
 	
	 	/* Load ASCII font information to the near memory.*/
 		printf ("hzk_createfont: loading '%s'\n", pf->AFont.file );
 		if(!(fp = fopen(AFont[hzk_id(pf)].file, "rb"))) 
		{
 		       	printf ("Error.\nThe ASCII HZK font file can not be found!\n");
 		       	return FALSE;
 		}
	 	if(fread(AFont[hzk_id(pf)].pFont, sizeof(char), pf->AFont.size, fp) < pf->AFont.size) 
		{
 		       	printf ("Error in reading ASCII HZK font file!\n");
 		       	fclose(fp);
 		       	return FALSE;
	 	}
 	
 		fclose(fp);
 	
		AFont[hzk_id(pf)].use_count=0;

  	}
	afont_address = AFont[hzk_id(pf)].pFont;
	pf->afont_address = AFont[hzk_id(pf)].pFont;
	pf->AFont.pFont = AFont[hzk_id(pf)].pFont;

	AFont[hzk_id(pf)].use_count++;

  	return TRUE;
}

/* This function unload system font from memory.*/
static void UnloadFont( PMWHZKFONT pf )
{
	CFont[hzk_id(pf)].use_count--;
	AFont[hzk_id(pf)].use_count--;

	if (!CFont[hzk_id(pf)].use_count)
	{	
	    	free(pf->CFont.pFont);
	    	free(pf->AFont.pFont);

	    	CFont[hzk_id(pf)].pFont = NULL;
	    	AFont[hzk_id(pf)].pFont = NULL;
	}
}

int
hzk_init(PSD psd)
{
	/* FIXME: *.KU file should be opened and
	 * read in here...*/
	return 1;
}

PMWFONT
hzk_createfont(const char *name, MWCOORD height, MWCOORD width, int attr)
{
	PMWHZKFONT	pf;

	if(strcmp(name,"HZKFONT")!=0 && strcmp(name,"HZXFONT")!=0)
		return FALSE;

	/*printf("hzk_createfont(%s,%d)\n",name,height);*/

	use_big5=name[2]-'K';

	/* allocate font structure*/
	pf = (PMWHZKFONT)calloc(sizeof(MWHZKFONT), 1);
	if (!pf)
		return NULL;
	pf->fontprocs = &hzk_procs;

	pf->fontsize = height;
	pf->fontwidth = width;
	pf->fontrotation = 0;
	pf->fontattr = attr;

	if(height==12)
	{		
		afont_width = 6;
		cfont_width = 12;
		font_height = 12;

		pf->afont_width = 6;
		pf->cfont_width = 12;
		pf->font_height = 12;
	}
	else 	
	{		
		afont_width = 8;
		cfont_width = 16;
		font_height = 16;

		pf->afont_width = 8;
		pf->cfont_width = 16;
		pf->font_height = 16;
	}

    	/* Load the font library to the system memory.*/
	if(!LoadFont(pf))
  	      	return FALSE;

	return (PMWFONT)pf;
}

int IsBig5(int i)
{
	if ((i>=0xa140 && i<=0xa3bf) || /* a140-a3bf(!a3e0) */
	    (i>=0xa440 && i<=0xc67e) || /* a440-c67e        */
	    (i>=0xc6a1 && i<=0xc8d3) || /* c6a1-c8d3(!c8fe) */
	    (i>=0xc940 && i<=0xf9fe))   /* c940-f9fe        */
		return 1;
	else
		return 0;
}

/*
 * following several function is used in hzk_drawtext
 */

static int getnextchar(char* s, unsigned char* cc)
{
    	if( s[0] == '\0') return 0;

    	cc[0] = (unsigned char)(*s);
    	cc[1] = (unsigned char)(*(s + 1));

    	if (use_big5)
    	{
		if( IsBig5( (int) ( (cc[0] << 8) + cc[1]) ) )
			return 1;
    	}
    	else
	{
    		if( ((unsigned char)cc[0] > 0xa0) &&
		    ((unsigned char)cc[1] > 0xa0) )
        		return 1;
	}

    	cc[1] = '\0';

    	return 1;
}

static void
expandcchar(PMWHZKFONT pf, int bg, int fg, unsigned char* c, MWPIXELVAL* bitmap)
{
	int i=0;
    	int c1, c2, seq;
	int x,y;
    	unsigned char *font;
    	int b = 0;		/* keep gcc happy with b = 0 - MW */

	int pixelsize;
	pixelsize=sizeof(MWPIXELVAL);

   	c1 = c[0];
    	c2 = c[1];
	if (use_big5)
	{
		seq=0;
		/* ladd=loby-(if(loby<127)?64:98)*/
		c2-=(c2<127?64:98);   

		/* hadd=(hiby-164)*157*/
		if (c1>=0xa4)	/* standard font*/
		{
			seq=(((c1-164)*157)+c2);
			if (seq>=5809) seq-=408;
		}

		/* hadd=(hiby-161)*157*/
		if (c1<=0xa3)	/* special font*/
			seq=(((c1-161)*157)+c2)+13094;
	}
	else
    		seq=((c1 - 161)*94 + c2 - 161); 

	font = pf->cfont_address + ((seq) * (pf->font_height * ((pf->cfont_width + 7) / 8)));

     	for (y = 0; y < pf->font_height; y++)
        	for (x = 0; x < pf->cfont_width; x++) 
		{
            		if (x % 8 == 0)
                		b = *font++;

            		if (b & (128 >> (x % 8)))   /* pixel */
			  	bitmap[i++]=fg;
			else
				bitmap[i++]=bg;
		}		
}

static void expandchar(PMWHZKFONT pf, int bg, int fg, int c, MWPIXELVAL* bitmap)
{
	int i=0;
	int x,y;
    	unsigned char *font;
	int pixelsize;
    	int b = 0;		/* keep gcc happy with b = 0 - MW */

	pixelsize=sizeof(MWPIXELVAL);

    	font = pf->afont_address + c * (pf->font_height *
		((pf->afont_width + 7) / 8));

  	for (y = 0; y < pf->font_height; y++)
		for (x = 0; x < pf->afont_width; x++) 
		{
	    		if (x % 8 == 0)
				b = *font++;
	    		if (b & (128 >> (x % 8)))	/* pixel */
				bitmap[i++]=fg;
			else
				bitmap[i++]=bg;
  		}
}

/*
 * Draw ASCII text string using HZK type font
 */
static void
hzk_drawtext(PMWFONT pfont, PSD psd, MWCOORD ax, MWCOORD ay,
	const void *text, int cc, MWTEXTFLAGS flags)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;

    	unsigned char c[2];
	MWPIXELVAL *bitmap;
    	unsigned char s1[3];
 	char *s,*sbegin;

	s=(char *)text;

	if(cc==1)
	{
		s1[0]=*((unsigned char*)text);
		s1[1]=0x0;
		s1[2]=0x0;
		s=s1;
    	}

	sbegin=s;
    	bitmap = (MWPIXELVAL *)ALLOCA(pf->cfont_width * pf->font_height *
			sizeof(MWPIXELVAL));

    	while( getnextchar(s, c) )
	{
              	if( c[1] != '\0') 
		{
                	expandcchar(pf, gr_background,gr_foreground,
                            c, bitmap);
			/* Now draw the bitmap ... */
			
			if (flags&MWTF_TOP)
				GdArea(psd,ax, ay, pf->cfont_width,
					pf->font_height, bitmap, MWPF_PIXELVAL);
			else
				GdArea(psd,ax, ay-pf->font_height+2,
					pf->cfont_width, pf->font_height,
					bitmap, MWPF_PIXELVAL);

                	s += 2;
                	ax += pf->cfont_width;
            	}
            	else 
		{
                	expandchar(pf, gr_background,gr_foreground,
                           c[0], bitmap);
			/* Now draw the bitmap ... */

			if (flags&MWTF_TOP) 
				GdArea(psd,ax, ay, pf->afont_width,
					pf->font_height, bitmap, MWPF_PIXELVAL);
			else
				GdArea(psd,ax, ay-pf->font_height+2,
					pf->afont_width, pf->font_height,
					bitmap, MWPF_PIXELVAL);

                	s += 1;
                	ax += pf->afont_width;
            	}
						
		if(s>=sbegin+cc)break;
    	}

	FREEA(bitmap);
}

/*
 * Return information about a specified font.
 */
static MWBOOL
hzk_getfontinfo(PMWFONT pfont, PMWFONTINFO pfontinfo)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;

	int i;

	pfontinfo->height = pf->font_height;
	pfontinfo->maxwidth = pf->cfont_width;
	pfontinfo->baseline = pf->font_height - 2;

	/* FIXME: calculate these properly: */
	pfontinfo->linespacing = pfontinfo->height;
	pfontinfo->descent = pfontinfo->height - pfontinfo->baseline;
	pfontinfo->maxascent = pfontinfo->baseline;
	pfontinfo->maxdescent = pfontinfo->descent;

	pfontinfo->firstchar = 0;
	pfontinfo->lastchar = 0;
	pfontinfo->fixed = TRUE;
		
	for(i=0; i<=256; i++)
		pfontinfo->widths[i] = pf->afont_width;

	return TRUE;
}

static void
hzk_gettextsize(PMWFONT pfont, const void *text, int cc, MWTEXTFLAGS flags,
	MWCOORD *pwidth, MWCOORD *pheight, MWCOORD *pbase)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;

   	unsigned char c[2];
 	char *s,*sbegin;
    	unsigned char s1[3];

	int ax=0;
	s=(char *)text;
	if(cc==0)
	{
		*pwidth = 0;
		*pheight = pf->font_height;
		*pbase = pf->font_height-2;

	}
	if(cc==1)
	{
		s1[0]=*((unsigned char*)text);
		s1[1]=0x0;
		s1[2]=0x0;
		s=s1;
    	}
	sbegin=s;
    	while( getnextchar(s, c) )
	{
		if( c[1] != '\0') 
		{
           		s += 2;
           		ax += pf->cfont_width;
        	}
        	else 
		{
           		s += 1;
           		ax += pf->afont_width;
        	}
		if(s>=sbegin+cc) {
			/*printf("s=%x,sbegin=%x,cc=%x\n",s,sbegin,cc);*/
			break;
		}

    	}
	/*printf("ax=%d,\n",ax);*/

	*pwidth = ax;
	*pheight = pf->font_height;
	*pbase = pf->font_height-2;
}

static void
hzk_destroyfont(PMWFONT pfont)
{
	PMWHZKFONT pf=(PMWHZKFONT)pfont;
	UnloadFont(pf);
	free(pf);
}
