/*
 * Copyright (c) 2019 Greg Haerr <greg@censoft.com>
 *
 * Operating System Dependent Routines
 */

/**
 * Output error message
 */
int
GdError(const char *format, ...)
{
	va_list args;
	char 	buf[1024];

	va_start(args, format);
	vsprintf(buf, format, args);
	write(2, buf, strlen(buf));
	va_end(args);
	return -1;
}

/*
 * Return # milliseconds elapsed since start of Microwindows
 * Granularity is 25 msec
 */
MWTIMEOUT
GdGetTickCount(void)
{
	static MWTIMEOUT startTicks = 0;

	return ++startTicks;
}

/* ring bell*/
void
GdBell(void)
{
	write(2, "\7", 1);
}

void
GdPlatformInit(void)
{
}

void nop(void) {}

int	MWMIN(int a,int b)		{ return ((a) < (b) ? (a) : (b)); }
int	MWMAX(int a,int b) 		{ return ((a) > (b) ? (a) : (b)); }
int MWABS(int x)			{ return ((x) < 0 ? -(x) : (x)); }

int MWIMAGE_SIZE(int width, int height)
{
	return ((height) * (((width) + MWIMAGE_BITSPERIMAGE - 1) / MWIMAGE_BITSPERIMAGE));
}

/* create MWCOLORVAL (0xAABBGGRR format)*/
MWCOLORVAL MWARGB(int a,int r,int g,int b)
{
	return ((MWCOLORVAL)(((unsigned char)(r)|
				(((uint32_t)(unsigned char)(g))<<8))|
				(((uint32_t)(unsigned char)(b))<<16)|
				(((uint32_t)(unsigned char)(a))<<24)));
}

MWCOLORVAL MWRGB(int r,int g,int b)
{
	return MWARGB(255,r,g,b);		/* argb 255 alpha*/
}
