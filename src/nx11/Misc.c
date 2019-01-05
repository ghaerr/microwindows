#include "nxlib.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int
_XOpenFile(_Xconst char *path, int flags)
{
	return open(path, flags);
}
