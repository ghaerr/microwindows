/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Modified by Tom Walton at Altia, Jan. 2002. to:
 * 1.  Handle fonts with widths up to 80 pixels.
 * 2.  Support passing command line arguments for the
 *     font name, pixel height, average pixel width (optional),
 *     bold (optional), and italic (optional).  If the font
 *     name has spaces, enclose it in double-quotes ("myfont").
 * 3.  Use the average width in the output file name
 *     instead of the maximum width.  And, the max width is
 *     computed dynamically as characters are converted.  This
 *     max width is the value assigned to the data structure's
 *     maxwidth element.
 * 4.  The window created for converting fonts remains open and
 *     displays information about what is being converted,
 *     what the output file name is, and when the conversion
 *     is done.  The window is closed like any regular Windows
 *     application after it reports that the conversion is done.
 *
 * MS Windows Font Grabber for Micro-Windows
 *
 * Usage: convfnt32 [1|2|3|4]
 *        convfnt32 "fontname" [pixel_height [pixel_width] [bold] [italic]]
 * Example:  convfnt32 "my font" 25 12
 *
 * Note: a Microsoft License is required to use MS Fonts
 */
#define FONT_NORMAL 0
#define FONT_BOLD 1
#define FONT_ITALIC 2
#define FONT_BOLDITALIC (FONT_BOLD | FONT_ITALIC)
static char *Font_Name = "MS Sans Serif";
static int Font_Height = 20;
static int Font_Width = 0;
static int Font_Style = FONT_NORMAL;
static char *Font_Style_String = "normal";

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BITS_HEIGHT	48	/* max character height*/
#define MAX_BITS_WIDTH 	80	/* max character width*/
typedef unsigned short	IMAGEBITS;	/* bitmap image unit size*/

/* IMAGEBITS macros*/
#define	IMAGE_SIZE(width, height)  ((height) * (((width) + sizeof(IMAGEBITS) * 8 - 1) / (sizeof(IMAGEBITS) * 8)))
#define IMAGE_WORDS(x)		(((x)+15)/16)
#define	IMAGE_BITSPERIMAGE	(sizeof(IMAGEBITS) * 8)
#define	IMAGE_FIRSTBIT		((IMAGEBITS) 0x8000)
#define	IMAGE_NEXTBIT(m)	((IMAGEBITS) ((m) >> 1))
#define	IMAGE_TESTBIT(m)	((m) & IMAGE_FIRSTBIT)	  /* use with shiftbit*/
#define	IMAGE_SHIFTBIT(m)	((IMAGEBITS) ((m) << 1))  /* for testbit*/

/* global data*/
HINSTANCE	ghInstance;
char 		APPWINCLASS[] = "convfnt";
int 		MAX_WIDTH = 0;
int 		AVE_WIDTH;
int 		CHAR_HEIGHT;
int		CHAR_ASCENT;
char 		fontname[64];
FILE *		fp;
HFONT		hfont;
int 		FIRST_CHAR = ' ';
int 		LAST_CHAR = 256;
int 		curoff = 0;
int 		offsets[256];
int 		widths[256];
int			haveArgs = 0;


/* forward decls*/
LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wp,LPARAM lp);

HWND InitApp(void);
int  InitClasses(void);
void doit(HDC hdc);
void convfnt(HDC hdc);
void print_char(int ch,IMAGEBITS *b, int w, int h);
void print_bits(IMAGEBITS *bits, int width, int height);
HFONT WINAPI GetFont(HDC hDC, LPSTR name, int height, int width, int style);
HFONT WINAPI GetFontEx(HDC hDC, LPSTR name, int height, int width, int style,
		int charset);

int WINAPI 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nShowCmd)
{
	MSG     msg;
	HDC     hdc;
	int     i;
	char    *argv[10];
	int     argc;
	char    cmdLine[1024];
	char    *cmdLinePtr;

	ghInstance = hInstance;
	InitClasses();
	InitApp();

	strncpy(cmdLine, lpCmdLine, 1024);
	cmdLine[1023] = '\0';

	i = atoi(cmdLine);
	hdc = GetDC(NULL);
	switch(i) {
	case 0:
		if(*cmdLine == 0)
		{
			haveArgs = 0;
			strcpy(cmdLine, "\"MS Sans Serif\"");
		}
		else
			haveArgs = 1;
		argc = 0;
		cmdLinePtr = cmdLine;
		do
		{
			while (*cmdLinePtr != '\0'
			       && (*cmdLinePtr == ' ' || *cmdLinePtr == '\t'))
				cmdLinePtr++;
			if (*cmdLinePtr != '\0')
			{
				if(*cmdLinePtr == '"' || *cmdLinePtr == '\'')
				{
					cmdLinePtr++;
					argv[argc] = cmdLinePtr;
					argc++;
					while (*cmdLinePtr != '\0' && *cmdLinePtr != '"'
					       && *cmdLinePtr != '\t')
						cmdLinePtr++;
					if (*cmdLinePtr == '\0')
						break;
					else
						*cmdLinePtr++ = '\0';
				}
				else
				{
					argv[argc] = cmdLinePtr;
					argc++;
					while (*cmdLinePtr != '\0' && *cmdLinePtr != ' '
					       && *cmdLinePtr != '\t')
						cmdLinePtr++;
					if (*cmdLinePtr == '\0')
						break;
					else
						*cmdLinePtr++ = '\0';
				}
			}
		} while (argc < 10 && *cmdLinePtr != '\0');

		if (argc == 0)
			haveArgs = 0;

		if (argc >= 1)
			Font_Name = argv[0];

		if (argc >= 2)
			Font_Height = atoi(argv[1]);

		if (argc >= 3 && *(argv[2]) >= '0' && *(argv[2]) <= '9')
			Font_Width = atoi(argv[2]);
		if (Font_Width < 0)
			Font_Width = 0;

		for (i = 2; i < argc; i++)
		{
			if (stricmp(argv[i], "italic") == 0)
				Font_Style |= FONT_ITALIC;
			else if (stricmp(argv[i], "bold") == 0)
				Font_Style |= FONT_BOLD;
		}

		switch(Font_Style)
		{
			case FONT_BOLD:
				Font_Style_String = "bold";
				break;
			case FONT_ITALIC:
				Font_Style_String = "italic";
				break;
			case FONT_BOLD | FONT_ITALIC:
				Font_Style_String = "bold italic";
				break;
		}

		hfont = GetFont(hdc, Font_Name, -Font_Height,
		                Font_Width, Font_Style);
		break;
	case 1:
		hfont = GetStockObject(DEFAULT_GUI_FONT);	/* winMSSansSerif11x13 */
		break;
	case 2:
		hfont = GetStockObject(SYSTEM_FONT);		/* winSystem14x16 */
		break;
	case 3:
		hfont = GetStockObject(OEM_FIXED_FONT);	/* winTerminal8x12 */
		break;
	case 4:
		hfont = GetStockObject(ANSI_VAR_FONT);	/* winMSSansSerif11x13 */
		break;
	}
	ReleaseDC(NULL, hdc);

	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

int
InitClasses(void)
{
	WNDCLASS	wc;

	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = ghInstance;
	wc.hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE( 1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName =  APPWINCLASS;
	return RegisterClass( &wc);
}

HWND
InitApp(void)
{
	HWND	hwnd;

	hwnd = CreateWindowEx( 0L, APPWINCLASS,
		"Font Grabber",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		ghInstance,
		NULL);

	if( hwnd == NULL)
		return( 0);

	ShowWindow( hwnd, SW_SHOW);
	return hwnd;
}


LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT		ps;
	HDC				hdc;
	LOGFONT			lf;
	TEXTMETRIC		tm;
	char			outfile[64];
	char 			*p, *q;

	switch( msg) {
	case WM_CREATE:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		SelectObject(hdc, hfont);

		GetObject(hfont, sizeof(lf), &lf);
		GetTextMetrics(hdc, &tm);
		MAX_WIDTH = 0; /* was tm.tmMaxCharWidth, now we compute it */
		AVE_WIDTH = tm.tmAveCharWidth;
		CHAR_HEIGHT = tm.tmHeight;
		CHAR_ASCENT = tm.tmAscent;
		FIRST_CHAR = tm.tmFirstChar;
		LAST_CHAR = tm.tmLastChar + 1;
		strcpy(fontname, lf.lfFaceName);
		q = p = fontname;
		while(*p) {
			if(*p != ' ')
				*q++ = *p;
			++p;
		}
		*q = 0;

		if (haveArgs)
		{
			char sample[1024];
			sprintf(sample, "Converting font \"%s\", Height %d, Width %d, Style \"%s\" ", Font_Name, Font_Height, Font_Width, Font_Style_String);
			TextOut(hdc, 0, 150, sample, strlen(sample));
			wsprintf(outfile, "win%s%dx%d.c", fontname,
			         AVE_WIDTH, CHAR_HEIGHT);
			sprintf(sample,
			        "To file \"%s\" (%dx%d is Width x Height)",
			        outfile, AVE_WIDTH, CHAR_HEIGHT);
			TextOut(hdc, 0, 200, sample, strlen(sample));
			fp = fopen(outfile, "wt");
			doit(hdc);
			fclose(fp);
			TextOut(hdc, 0, 250, " DONE! ", 7);
		}
		else
		{
			char *usage = "Usage:  convfnt.exe  \"fontname\"  [ pixel_height  [pixel_width]  [bold]  [italic] ] ";
			TextOut(hdc, 0, 0, usage, strlen(usage));
			usage = "Example:  convfnt.exe  \"my font\"  25  12  bold  italic ";
			TextOut(hdc, 0, 30, usage, strlen(usage));
		}
		EndPaint(hwnd, &ps);
		break;

	case WM_LBUTTONDOWN:
		break;

	default:
		return DefWindowProc( hwnd, msg, wp, lp);
	}
	return( 0);
}


void
convfnt(HDC hdc)
{
	SIZE	size;
   	unsigned char	ch;
	int		i;
	int		x, y, w;
	int		word_width;
	IMAGEBITS mask_value;
	IMAGEBITS *image_ptr;
	IMAGEBITS c;
	IMAGEBITS	image[MAX_BITS_HEIGHT * IMAGE_WORDS(MAX_BITS_WIDTH)];
	static IMAGEBITS mask[IMAGE_BITSPERIMAGE * IMAGE_WORDS(MAX_BITS_WIDTH)] = { 
		0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
		0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
		0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
		0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
		0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
		0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
		0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
		0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
		0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
		0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001
	};

	for(i = FIRST_CHAR; i < LAST_CHAR; ++i)
	{
		ch = i;
		TextOut(hdc, 0, 0, &ch, 1);
		GetTextExtentPoint32(hdc, &ch, 1, &size);
		if (size.cx > MAX_BITS_WIDTH)
		{
			offsets[ch] = curoff;
			widths[ch] = 0;
		}
		else if (size.cx > MAX_WIDTH)
			MAX_WIDTH = size.cx;
		word_width = IMAGE_WORDS(size.cx);
		for(y = 0; y < size.cy; ++y)
		{
			for (w = 0; w < word_width; w++)
			{
				image_ptr = &(image[(y * word_width) + w]);
				*image_ptr = 0;
				for(x=IMAGE_BITSPERIMAGE * w;
				    x < (int)((IMAGE_BITSPERIMAGE * w) + 16) && x < size.cx;
				    ++x)
				{
					c = GetPixel(hdc, x, y)? 0: 1;
					mask_value = mask[x];
					*image_ptr = (*image_ptr & ~mask_value)
					             | (c << (15 - (x % 16)));
				}
			}
		}
		offsets[ch] = curoff;
		widths[ch] = size.cx;
		print_char(ch, image, size.cx, size.cy);
		print_bits(image, size.cx, size.cy);
		curoff += (size.cy * word_width);
		fprintf(fp, "\n");
	}
}


void
doit(HDC hdc)
{
	int		i;

	fprintf(fp, "/* Generated by convfnt.exe*/\n");
	fprintf(fp, "#include \"device.h\"\n\n");
	fprintf(fp, "/* Windows %s %dx%d Font */\n",
		fontname, AVE_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "/* Originated from: \"%s\", Height %d, Width %d, Style \"%s\" */\n\n",
		Font_Name, Font_Height, Font_Width, Font_Style_String);
	fprintf(fp, "static MWIMAGEBITS win%s%dx%d_bits[] = {\n\n",
		fontname, AVE_WIDTH, CHAR_HEIGHT);

	convfnt(hdc);

	fprintf(fp, "};\n\n");

	fprintf(fp, "/* Character->glyph data. */\n");
	fprintf(fp, "static unsigned short win%s%dx%d_offset[] = {\n",
		fontname, AVE_WIDTH, CHAR_HEIGHT);
	for(i=FIRST_CHAR; i<LAST_CHAR; ++i)
		fprintf(fp, "  %d,\t /* %c (0x%02x) */\n", offsets[i], i<' '? ' ':i , i);
	fprintf(fp, "};\n\n");

	fprintf(fp, "/* Character width data. */\n");
	fprintf(fp, "static unsigned char win%s%dx%d_width[] = {\n",
		fontname, AVE_WIDTH, CHAR_HEIGHT);
	for(i=FIRST_CHAR; i<LAST_CHAR; ++i)
		fprintf(fp, "  %d,\t /* %c (0x%02x) */\n", widths[i], i<' '? ' ':i , i);
	fprintf(fp, "};\n\n");


	fprintf(fp, "/* Exported structure definition. */\n"
		"MWCFONT font_win%s%dx%d = {\n",
		fontname, AVE_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "\t\"win%s%dx%d\",\n", fontname, AVE_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "\t%d,\n", MAX_WIDTH);
	fprintf(fp, "\t%d,\n", CHAR_HEIGHT);
	fprintf(fp, "\t%d,\n", CHAR_ASCENT);
	fprintf(fp, "\t%d,\n\t%d,\n", FIRST_CHAR, LAST_CHAR-FIRST_CHAR);
	fprintf(fp, "\twin%s%dx%d_bits,\n", fontname, AVE_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "\twin%s%dx%d_offset,\n", fontname, AVE_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "\twin%s%dx%d_width,\n", fontname, AVE_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "};\n");
}

/* Character ! (0x21):
   ht=16, width=8
   +----------------+
   |                |
   |                |
   | *              |
   | *              |
   | *              |
   | *              |
   | *              |
   | *              |
   |                |
   | *              |
   |                |
   |                |
   +----------------+ */

void
print_char(int ch,IMAGEBITS *bits, int width, int height)
{
	int 		x, word_width;
	int 		bitcount;	/* number of bits left in bitmap word */
	IMAGEBITS	bitvalue;	/* bitmap word value */

	fprintf(fp, "/* Character %c (0x%02x):\n", (ch < ' '? ' ': ch), ch);
	fprintf(fp, "   ht=%d, width=%d\n", height, width);
	fprintf(fp, "   +");
	for(x = 0; x < width; ++x)
		fprintf(fp, "-");
	fprintf(fp, "+\n");
	x = 0;
	bitcount = 0;
	word_width = IMAGE_WORDS(width);
	while (height > 0)
	{
	    if (bitcount <= 0)
	    {
		    fprintf(fp, "   |");
		    bitcount = IMAGE_BITSPERIMAGE * word_width;
		    bitvalue = *bits++;
	    }
	    if (IMAGE_TESTBIT(bitvalue))
	    	fprintf(fp, "*");
	    else
	    	fprintf(fp, " ");
	    bitvalue = IMAGE_SHIFTBIT(bitvalue);
	    --bitcount;
	    if (bitcount > 0 && (bitcount % IMAGE_BITSPERIMAGE) == 0)
	    	bitvalue = *bits++;
	    if (x++ == width-1)
	    {
	    	x = 0;
	    	--height;
	    	bitcount = 0;
	    	fprintf(fp, "|\n");
	    }
	}
	fprintf(fp, "   +");
	for(x=0; x<width; ++x)
		fprintf(fp, "-");
	fprintf(fp, "+ */\n");
}

#define	IMAGE_GETBIT4(m)	(((m) & 0xf000) >> 12)
#define	IMAGE_SHIFTBIT4(m)	((IMAGEBITS) ((m) << 4))

void
print_bits(IMAGEBITS *bits, int width, int height)
{
	int 		x, word_width;
	int 		bitcount;	/* number of bits left in bitmap word */
	IMAGEBITS	bitvalue;	/* bitmap word value */

	x = 0;
	bitcount = 0;
	word_width = IMAGE_WORDS(width);
	while (height > 0)
	{
	    if (bitcount <= 0)
	    {
	    	fprintf(fp, "0x");
	    	bitcount = IMAGE_BITSPERIMAGE * word_width;
	    	bitvalue = *bits++;
	    }
	    fprintf(fp, "%x", IMAGE_GETBIT4(bitvalue));
	    bitvalue = IMAGE_SHIFTBIT4(bitvalue);
	    bitcount -= 4;
	    if (bitcount > 0 && (bitcount % IMAGE_BITSPERIMAGE) == 0)
	    {
	    	fprintf(fp, ",0x");
	    	bitvalue = *bits++;
	    }
		x += 4;
	    if (x >= width)
	    {
	    	if(IMAGE_BITSPERIMAGE > (width % IMAGE_BITSPERIMAGE)
	    	   && (width % IMAGE_BITSPERIMAGE) != 0)
	    		for(x = IMAGE_BITSPERIMAGE - (width % IMAGE_BITSPERIMAGE);
	    		    x > 3; x -= 4)
	    	{
	    		fprintf(fp, "0");
	    	}
	    	x = 0;
	    	--height;
	    	bitcount = 0;
	    	fprintf(fp, ",\n");
	    }
	}
}

/*
 * WIN Draw Library
 *
 * GetFont style bits:
 *			01 bold
 *			02 italic
 * fontSize > 0		points (must pass hDC for non-screen font)
 * fontSize < 0		pixels (no HDC needed)
 */

HFONT WINAPI
GetFont(HDC hDC, LPSTR fontName,int fontSize,int fontWidth,int fontStyle)
{
	return GetFontEx(hDC, fontName, fontSize, fontWidth,
	                 fontStyle, ANSI_CHARSET);
}

HFONT WINAPI
GetFontEx(HDC hDC, LPSTR fontName,int fontSize,int fontWidth,
          int fontStyle,int charset)
{
	LOGFONT	lf;
	HDC		hdc;

	memset( &lf, 0, sizeof(LOGFONT));

	if( fontSize < 0 || hDC)
		hdc = hDC;
	else hdc = GetDC( GetDesktopWindow());

	/* calculate font size from passed point size*/
	if( fontSize < 0)
		lf.lfHeight = -fontSize;
	else lf.lfHeight = -MulDiv( fontSize,
				GetDeviceCaps( hdc, LOGPIXELSY), 72);
	if( fontName)
		strncpy( lf.lfFaceName, fontName, LF_FACESIZE);
	else lf.lfFaceName[ 0] = '\0';
	lf.lfWeight = (fontStyle & 01)? FW_BOLD: FW_NORMAL;
	if( fontStyle & 02)
		lf.lfItalic = 1;
	lf.lfCharSet = charset;
	lf.lfWidth = fontWidth;

	if( fontSize > 0 && !hDC)
		ReleaseDC( GetDesktopWindow(), hdc);
	return CreateFontIndirect( &lf);
}
