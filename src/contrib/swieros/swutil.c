/*
 * Util Clib routines for swieros environment
 */

#define memcpy	xmemcpy
#define memchr	xmemchr
#define strlen	xstrlen
#define strcpy	xstrcpy
#define strcat	xstrcat
#define strcasecmp xstrcasecmp
#define strncpy	xstrncpy
#define calloc	xcalloc
#define atoi	xatoi

char *memcpy(char *d, char *s, int n);
char *memchr(char *s, int c, int n);

int
strlen(char *s)
{
	return memchr(s, 0, -1) - s;
}

char *
strcpy(char *d, char *s)
{
	return memcpy(d, s, strlen(s)+1);
}

char *
strcat(char *d, char *s)
{
	memcpy(memchr(d, 0, -1), s, strlen(s)+1);
	return d;
}

int
strcasecmp(char *d, char *s)
{
    int f, l;

    do {
        if ((f = *d++) >= 'A' && f <= 'Z')
            f -= 'A' - 'a';
        if ((l = *s++) >= 'A' && l <= 'Z')
            l -= 'A' - 'a';
    } while (f && (f == l));

    return (f - l);
}

char *
strncpy(char *d, char *s, int count)
{
        char *start = d;

        while (count && (*d++ = *s++))
                --count;
        if (count)
                while (--count)
                        *d++ = '\0';

        return start;
}

char *
calloc(int count, int size)
{
	int nbytes = count * size;
	char *p = malloc(nbytes);

	if (!p)
		return 0;
	memset(p, 0, nbytes);
	return p;
}

int
atoi(char *str)
{
	int n = 0;

	while (*str >= '0' && *str <= '9')
		n = n*10 + *str++ - '0';
	return n;
}

/* for mmap*/
int
getpagesize(void)
{
	return 4096;
}
