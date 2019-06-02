#include "nxlib.h"
#include "X11/Xutil.h"
#include "X11/Xatom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
XMisc.c -  code fragments taken from xlib/XlibUtil.c
(c) Andreas Foedrowitz  
******************************************************************************/

char *
XDisplayString(Display * display)
{
	if (display->display_name)
		return display->display_name;
	return getenv("DISPLAY");
}

int
XReadBitmapFile(Display * display, Drawable d, _Xconst char *filename,
		unsigned int *width_return, unsigned int *height_return,
		Pixmap * bitmap_return, int *x_hot_return, int *y_hot_return)
{
	unsigned char *data;

	int result = XReadBitmapFileData(filename, width_return, height_return,
			&data, x_hot_return, y_hot_return);
	if (result == BitmapSuccess) {
		*bitmap_return = XCreateBitmapFromData(display, d, (char *)data,
					*width_return, *height_return);
	}
	if (data)
		Xfree(data);
	return result;
}

int
XReadBitmapFileData(_Xconst char *filename, unsigned int *width_return,
		    unsigned int *height_return, unsigned char **data_return,
		    int *x_hot_return, int *y_hot_return)
{
	FILE *file = fopen(filename, "r");
	*width_return = 0;
	*height_return = 0;
	*data_return = NULL;
	*x_hot_return = 0;
	*y_hot_return = 0;
	int ret;

	if (file) {
		const char *define = "#define";
		char str[100];

		ret = fscanf(file, "%s", str);
		if (ret > 0 && !strcmp(str, define)) {
			ret = fscanf(file, "%s%d", str, width_return);
			if (ret > 0) ret = fscanf(file, "%s", str);
			if (ret > 0 && !strcmp(str, define)) {
				ret = fscanf(file, "%s%d", str, height_return);
				if (ret > 0) ret = fscanf(file, "%s", str);
				if (ret > 0 && !strcmp(str, define)) {
					ret = fscanf(file, "%s%d", str, x_hot_return);
					if (ret > 0) ret = fscanf(file, "%s", str);
					if (ret > 0 && !strcmp(str, define)) {
						ret = fscanf(file, "%s%d", str, y_hot_return);
					}
				}
			}
		}
		if (ret <= 0) {
			fclose(file);
			return BitmapFileInvalid;
		}
		if ((*width_return > 0) && (*height_return > 0)) {
			int dataSize = ((*width_return + 7) / 8) * (*height_return);
			int c, value, i, dataIndex = 0;

			*data_return = (unsigned char *)Xmalloc(dataSize);
			if (!*data_return) {
				fclose(file);
				return BitmapNoMemory;
			}
			while ((!feof(file)) && (dataIndex < dataSize)) {
				c = fgetc(file);
				if (c == 'x') {
					value = 0;
					for (i = 0; i < 2; i++) {
						/*** read 2 hex nibbles ***/
						c = fgetc(file);
						value <<= 4;
						if ((c >= '0') && (c <= '9'))
							value |= c - '0';
						else if ((c >= 'A')
							 && (c <= 'F'))
							value |= c - 'A' + 10;
						else if ((c >= 'a')
							 && (c <= 'f'))
							value |= c - 'a' + 10;
					}
					(*data_return)[dataIndex++] = (unsigned char) value;
				}
			}
			fclose(file);
			return BitmapSuccess;
		} else {
			fclose(file);
			return BitmapFileInvalid;
		}
	}
	return BitmapOpenFailed;
}

int
XStoreBuffer(Display * display, _Xconst char *bytes, int nbytes, int buffer)
{
	if ((buffer >= 0) && (buffer <= 7)) {
		return XChangeProperty(display, display->screens[display->default_screen].root,
				       (Atom) (XA_CUT_BUFFER0 + buffer), XA_STRING, 8, PropModeReplace,
				       (unsigned char *) bytes, nbytes);
	}
	return 0;
}

int
XStoreBytes(Display * display, _Xconst char *bytes, int nbytes)
{
	return XStoreBuffer(display, bytes, nbytes, 0);
}
