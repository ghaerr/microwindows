#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
/*
 * queryfont.c - NXLIB test program to print font infomation
 */

#define FONT "-adobe-helvetica-medium-r-normal--11-80-100-100-p-56-iso8859-1" /*helvR08*/
void
printc(char *str, XCharStruct *cs)
{
	printf("%s\n", str);
	if (cs->rbearing - cs->lbearing > cs->width) printf("WIDTH\n");
	printf("  lbearing %d\n", cs->lbearing);
	printf("  rbearing %d\n", cs->rbearing);
	printf("  width %d\n", cs->width);
	printf("  ascent %d\n", cs->ascent);
	printf("  descent %d\n", cs->descent);
	printf("  attributes %d\n", cs->attributes);
}

int
main(int ac, char **av)
{
	Display *d;
	XFontStruct *fs;
	char *font;

	if (ac > 1)
		font = av[1];
	else font = FONT;
	d = XOpenDisplay(NULL);
	if (!d)
		exit(1);
	fs = XLoadQueryFont(d, font);
	if (fs) {
		int i, size;

		printf("font %s\n", font);
		printf("fid 0x%x\n", (unsigned int)fs->fid);
		printf("direction %d\n", fs->direction);
		printf("min_byte2 %d\n", fs->min_char_or_byte2);
		printf("max_byte2 %d\n", fs->max_char_or_byte2);
		printf("min_byte1 %d\n", fs->min_byte1);
		printf("max_byte1 %d\n", fs->max_byte1);
		printf("all_exist %d\n", fs->all_chars_exist);
		printf("default_char %d\n", fs->default_char);
		printf("n_props %d\n", fs->n_properties);
		printf("ascent %d\n", fs->ascent);
		printf("descent %d\n", fs->descent);
		printc("min bounds", &fs->min_bounds);
		printc("max bounds", &fs->max_bounds);
		size = (fs->max_byte1 - fs->min_byte1 + 1) *
			(fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1);
		printf("size %d\n", size);
		for (i=0; i<size; ++i) {
			char buf[32];
			if (!fs->per_char)
				continue;
			sprintf(buf, "char %d (%d):", i, i+fs->min_char_or_byte2);
			printc(buf, &fs->per_char[i]);
		}
	}
	return 0;
}
