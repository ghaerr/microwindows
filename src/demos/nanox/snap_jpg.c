/*
 * snap_jpg - screen snapshot for Nano-X, jpeg format
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <nano-X.h>
#include <jpeglib.h>

/* create RGB colorval (0x00BBGGRR) from 8/8/8 format pixel*/
#define PIXEL888TOCOLORVAL(p)	\
	((((p) & 0xff0000) >> 16) | ((p) & 0xff00) | (((p) & 0xff) << 16))

/* create RGB colorval (0x00BBGGRR) from 5/6/5 format pixel*/
#define PIXEL565TOCOLORVAL(p)	\
	((((p) & 0xf800) >> 8) | (((p) & 0x07e0) << 5) | (((p) & 0x1f) << 19))

#define PIXEL555TOCOLORVAL(p)	\
	((((p) & 0x7c00) >> 7) | (((p) & 0x03e0) << 6) | (((p) & 0x1f) << 19))

/* create RGB colorval (0x00BBGGRR) from 3/3/2 format pixel*/
#define PIXEL332TOCOLORVAL(p)	\
	((((p) & 0xe0)) | (((p) & 0x1c) << 11) | (((p) & 0x03) << 19))


int
save_image(unsigned char *fb, GR_WINDOW_FB_INFO * info, char *file)
{
	int y;
	FILE *fp;
	unsigned long colorval = 0;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	fp = fopen(file, "wb");
	if (!fp) {
		fprintf(stderr, "Bad file name (error [%s])\n",
			strerror(errno));
		return (-1);
	}

	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fp);

	cinfo.in_color_space = JCS_RGB;

	cinfo.image_width = info->xres;
	cinfo.image_height = info->yres;

	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);

	jpeg_start_compress(&cinfo, TRUE);

	for (y = 0; y < info->yres; y++) {
		JSAMPROW row[1];

		int x;
		unsigned char *dest = alloca(info->xres * 3);

		unsigned char *ptr = dest;
		unsigned char *ch = (fb + (y * info->pitch));

		for (x = 0; x < info->xres; x++) {
			switch (info->bpp) {
			case 8:
				colorval = PIXEL332TOCOLORVAL(*ch);
				break;

			case 16:
				colorval =
					PIXEL555TOCOLORVAL(*((unsigned short *)ch));
				break;

			case 24:
			case 32:
				colorval = PIXEL888TOCOLORVAL(*((unsigned long *)ch));
				break;
			}

			ptr[2] = (colorval >> 16) & 0xFF;
			ptr[1] = (colorval >> 8) & 0xFF;
			ptr[0] = (colorval & 0xFF);

			ptr += 3;
			ch += info->bytespp;
		}

		row[0] = dest;
		jpeg_write_scanlines(&cinfo, row, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(fp);

	return (0);
}

int
main(int argc, char **argv)
{
	GR_WINDOW_FB_INFO fbinfo;
	unsigned char *fb;

	if (argc < 2) {
		printf("Usage:  snap_jpg <filename>\n");
		return (0);
	}

	if (GrOpen() == -1) {
		fprintf(stderr, "Error - the Nano-X server is not running\n");
		return (-1);
	}

	printf("Taking the picture and storing it in [%s]\n", argv[1]);

	fb = GrOpenClientFramebuffer();

	if (!fb) {
		fprintf(stderr,
			"Error - Unable to get the snapshot.  leaving!\n");
		return (-1);
	}

	GrGetWindowFBInfo(GR_ROOT_WINDOW_ID, &fbinfo);

	if (save_image(fb, &fbinfo, argv[1]) == -1)
		printf("Error!\n");
	else
		printf("Sucessfully saved [%s]\n", argv[1]);

	GrCloseClientFramebuffer();
	GrClose();
	return (0);
}
