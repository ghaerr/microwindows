/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * MS Windows Font Grabber for Micro-Windows
 *
 * Usage: convfnt32 [1|2|3|4|<fontname>]
 *
 * Note: a Microsoft License is required to use MS Fonts
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#define MAX_CHAR_HEIGHT	16	/* max character height*/
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
int 		CHAR_WIDTH;
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


/* forward decls*/
LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wp,LPARAM lp);

HWND InitApp(void);
int  InitClasses(void);
void doit(HDC hdc);
void convfnt(HDC hdc);
void print_char(int ch,IMAGEBITS *b, int w, int h);
void print_bits(IMAGEBITS *bits, int width, int height);
HFONT WINAPI GetFont(HDC hDC, LPSTR fontName,int fontSize,int fontStyle);
HFONT WINAPI GetFontEx(HDC hDC, LPSTR fontName,int fontSize,int fontStyle,
		int charset);

int WINAPI 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nShowCmd)
{
	MSG		msg;
	HDC		hdc;
	int		i;
	char *	q;
	char	arg[80];

	ghInstance = hInstance;
	InitClasses();
	InitApp();

	i = atoi(lpCmdLine);
	hdc = GetDC(NULL);
	switch(i) {
	case 0:
		if(*lpCmdLine == 0)
			lpCmdLine = "MS Sans Serif";
		q = arg;
		for(q=arg; *lpCmdLine; ++lpCmdLine) {
			if(*lpCmdLine == '"' || *lpCmdLine == '\'')
				continue;
			*q++ = *lpCmdLine;
		}
		*q = 0;
		hfont = GetFont(hdc, arg, 8, 0);
		break;
	case 1:
		hfont = GetStockObject(DEFAULT_GUI_FONT);	/* winMSSansSerif11x13 */
		break;
	case 2:
		hfont = GetStockObject(SYSTEM_FONT);		/* winSystem14x16 */
		break;
	case 3:
		hfont = GetStockObject(OEM_FIXED_FONT);		/* winTerminal8x12 */
		break;
	case 4:
		hfont = GetStockObject(ANSI_VAR_FONT);		/* winMSSansSerif11x13 */
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
		CHAR_WIDTH = tm.tmMaxCharWidth;
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

		wsprintf(outfile, "win%s%dx%d.c", fontname, CHAR_WIDTH, CHAR_HEIGHT);
		fp = fopen(outfile, "wt");
		doit(hdc);
		fclose(fp);
		exit(1);
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
	int		x, y;
	USHORT 	c;
	IMAGEBITS	image[MAX_CHAR_HEIGHT];
	static USHORT mask[] = { 
		0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
		0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001
	};

	for(i=FIRST_CHAR; i<LAST_CHAR; ++i) {
		ch = i;
		TextOut(hdc, 0, 0, &ch, 1);
		GetTextExtentPoint32(hdc, &ch, 1, &size);
		for(y=0; y<size.cy; ++y) {
			image[y] = 0;
			for(x=0; x<size.cx; ++x) {
				c = GetPixel(hdc, x, y)? 0: 1;
				image[y] = (image[y] & ~mask[x&15]) | (c << (15 - (x & 15)));
			}
		}
		offsets[ch] = curoff;
		widths[ch] = size.cx;
		print_char(ch, image, size.cx, size.cy);
		print_bits(image, size.cx, size.cy);
		curoff += size.cy;
		fprintf(fp, "\n");
	}
}


void
doit(HDC hdc)
{
	int		i;

	fprintf(fp, "/* Generated by convfnt.exe*/\n");
	fprintf(fp, "#include \"device.h\"\n\n");
	fprintf(fp, "/* Windows %s %dx%d Font */\n\n",
		fontname, CHAR_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "static MWIMAGEBITS win%s%dx%d_bits[] = {\n\n",
		fontname, CHAR_WIDTH, CHAR_HEIGHT);

	convfnt(hdc);

	fprintf(fp, "};\n\n");

	fprintf(fp, "/* Character->glyph data. */\n");
	fprintf(fp, "static unsigned short win%s%dx%d_offset[] = {\n",
		fontname, CHAR_WIDTH, CHAR_HEIGHT);
	for(i=FIRST_CHAR; i<LAST_CHAR; ++i)
		fprintf(fp, "  %d,\t /* %c (0x%02x) */\n", offsets[i], i<' '? ' ':i , i);
	fprintf(fp, "};\n\n");

	fprintf(fp, "/* Character width data. */\n");
	fprintf(fp, "static unsigned char win%s%dx%d_width[] = {\n",
		fontname, CHAR_WIDTH, CHAR_HEIGHT);
	for(i=FIRST_CHAR; i<LAST_CHAR; ++i)
		fprintf(fp, "  %d,\t /* %c (0x%02x) */\n", widths[i], i<' '? ' ':i , i);
	fprintf(fp, "};\n\n");


	fprintf(fp, "/* Exported structure definition. */\n"
		"MWCFONT font_win%s%dx%d = {\n",
		fontname, CHAR_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "\t\"win%s%dx%d\",\n", fontname, CHAR_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "\t%d,\n", CHAR_WIDTH);
	fprintf(fp, "\t%d,\n", CHAR_HEIGHT);
	fprintf(fp, "\t%d,\n", CHAR_ASCENT);
	fprintf(fp, "\t%d,\n\t%d,\n", FIRST_CHAR, LAST_CHAR-FIRST_CHAR);
	fprintf(fp, "\twin%s%dx%d_bits,\n", fontname, CHAR_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "\twin%s%dx%d_offset,\n", fontname, CHAR_WIDTH, CHAR_HEIGHT);
	fprintf(fp, "\twin%s%dx%d_width,\n", fontname, CHAR_WIDTH, CHAR_HEIGHT);
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
	int 		x;
	int 		bitcount;	/* number of bits left in bitmap word */
	IMAGEBITS	bitvalue;	/* bitmap word value */

	fprintf(fp, "/* Character %c (0x%02x):\n", (ch < ' '? ' ': ch), ch);
	fprintf(fp, "   ht=%d, width=%d\n", height, width);
	fprintf(fp, "   +");
	for(x=0; x<width; ++x)
		fprintf(fp, "-");
	fprintf(fp, "+\n");
	x = 0;
	bitcount = 0;
	while (height > 0) {
	    if (bitcount <= 0) {
		    fprintf(fp, "   |");
		    bitcount = IMAGE_BITSPERIMAGE;
		    bitvalue = *bits++;
	    }
		if (IMAGE_TESTBIT(bitvalue))
			    fprintf(fp, "*");
		else fprintf(fp, " ");
	    bitvalue = IMAGE_SHIFTBIT(bitvalue);
	    --bitcount;
	    if (x++ == width-1) {
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
	int 		x;
	int 		bitcount;	/* number of bits left in bitmap word */
	IMAGEBITS	bitvalue;	/* bitmap word value */

	x = 0;
	bitcount = 0;
	while (height > 0) {
	    if (bitcount <= 0) {
		    fprintf(fp, "0x");
		    bitcount = IMAGE_BITSPERIMAGE;
		    bitvalue = *bits++;
	    }
		fprintf(fp, "%x", IMAGE_GETBIT4(bitvalue));
	    bitvalue = IMAGE_SHIFTBIT4(bitvalue);
	    bitcount -= 4;
		x += 4;
	    if (x >= width) {
			if(IMAGE_BITSPERIMAGE > width)
				for(x=IMAGE_BITSPERIMAGE-width; x>3; ) {
					fprintf(fp, "0");
					x -= 4;
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
GetFont(HDC hDC, LPSTR fontName,int fontSize,int fontStyle)
{
	return GetFontEx(hDC, fontName, fontSize, fontStyle, ANSI_CHARSET);
}

HFONT WINAPI
GetFontEx(HDC hDC, LPSTR fontName,int fontSize,int fontStyle,int charset)
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

	if( fontSize > 0 && !hDC)
		ReleaseDC( GetDesktopWindow(), hdc);
	return CreateFontIndirect( &lf);
}
