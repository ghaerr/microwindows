/*
 * Microwindows unistd.h for Windows systems
 *
 * Usage: replace #include <unistd.h> with "uni_std.h"
 *
 * https://stackoverflow.com/a/826027/1202830
 */

#if !WIN32 && !defined(_MSC_VER)
#include <unistd.h>

#else /* WIN32 || defined(_MSC_VER)*/

//#include <stdlib.h>
#include <io.h>
//#include <getopt.h> /* getopt at: https://gist.github.com/ashelly/7776712 */
//#include <process.h> /* for getpid() and the exec..() family */
#include <direct.h> /* for _getcwd() and _chdir() */

#define srandom srand
#define random rand

/* Values for the second argument to access*/
#define R_OK    4       /* Test for read permission.  */
#define W_OK    2       /* Test for write permission.  */
#define X_OK    1     	/* execute permission - unsupported in windows*/
#define F_OK    0       /* Test for existence.  */

#define access _access
#define dup2 _dup2
#define execve _execve
#define ftruncate _chsize
#define unlink _unlink
#define fileno _fileno
#define getcwd _getcwd
#define chdir _chdir
#define isatty _isatty
#define lseek _lseek

/* read, write, and close are defined here, but they may not work for sockets, e.g. closesocket()*/
#define open _open
#define read _read
#define write _write
#define close _close

#ifdef _WIN64
#define ssize_t __int64
#else
#define ssize_t long
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* must include this file before "device.h" for this to work*/
#define alloca		_alloca

/* string routines*/
#define strcasecmp		_stricmp
#define strncasecmp		_strnicmp

/* should be in some equivalent to <sys/types.h> */
//typedef __int8            int8_t;
//typedef __int16           int16_t; 
//typedef __int32           int32_t;
//typedef __int64           int64_t;
//typedef unsigned __int8   uint8_t;
//typedef unsigned __int16  uint16_t;
//typedef unsigned __int32  uint32_t;
//typedef unsigned __int64  uint64_t;

#endif /* !WIN32 && !defined(_MSC_VER)*/
