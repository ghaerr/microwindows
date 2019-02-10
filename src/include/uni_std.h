/*
 * Microwindows unistd.h
 *
 * Required header for ALLOCA, MSC functions, strdup/strn string functions
 *
 * Usage: replace #include <unistd.h> with "uni_std.h"
 *
 * https://stackoverflow.com/a/826027/1202830
 */

#if !defined(_MSC_VER)
#include <unistd.h>

#if __MINGW32__
#include <malloc.h> 		/* for alloca */
#endif

#if RTEMS | PSP | __ECOS | __MINGW32__
#define  srandom  srand
#define  random   rand
#endif

#else /* defined(_MSC_VER)*/

#include <io.h>
#include <direct.h>			/* for _getcwd() and _chdir()*/
//#include <getopt.h>		/* getopt at: https://gist.github.com/ashelly/7776712*/
//#include <process.h>		/* for getpid() and exec() family*/

#define alloca		_alloca
#define access		_access
#define dup2		_dup2
#define execve		_execve
#define ftruncate	_chsize
#define unlink		_unlink
#define fileno		_fileno
#define getcwd		_getcwd
#define chdir		_chdir
#define isatty		_isatty
#define lseek		_lseek

/* read, write, and close are defined here, but they may not work for sockets, e.g. closesocket()*/
#define open		_open
#define read		_read
#define write		_write
#define close		_close

#ifdef _WIN64
#define ssize_t		__int64
#else
#define ssize_t		long
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* Values for the second argument to access*/
#define R_OK    4       /* Read permission*/
#define W_OK    2       /* Write permission*/
#define X_OK    1     	/* Execute permission - unsupported in windows*/
#define F_OK    0       /* File exists*/

/* math routines*/
#define srandom			srand
#define random			rand

/* string routines*/
#define strcasecmp		_stricmp
#define strncasecmp		_strnicmp
#define strnicmp		_strnicmp
#define strdup			_strdup
#define snprintf		_snprintf 
#define vsnprintf		_vsnprintf 

#endif /* defined(_MSC_VER)*/
