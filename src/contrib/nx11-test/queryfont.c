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
	printf("%s  ", str);
	if (cs->rbearing - cs->lbearing > cs->width) printf("WIDTH ");
	printf("\tlbearing %d,\t", cs->lbearing);
	printf("\trbearing %d,\t", cs->rbearing);
	printf("\twidth %d,\t", cs->width);
	printf("\tascent %d,\t", cs->ascent);
	printf("\tdescent %d,\t", cs->descent);
	printf("\tattributes %d\n ", cs->attributes);
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

		printf("font %s\n\n", font);
		printf("fid 0x%x, ", (unsigned int)fs->fid);
		printf("direction %d, ", fs->direction);
		printf("min_byte2 %d, ", fs->min_char_or_byte2);
		printf("max_byte2 %d, ", fs->max_char_or_byte2);
		printf("min_byte1 %d, ", fs->min_byte1);
		printf("max_byte1 %d, ", fs->max_byte1);
		printf("all_exist %d, ", fs->all_chars_exist);
		printf("default_char %d, ", fs->default_char);
		printf("n_props %d\n", fs->n_properties);
		printf("ascent %d, ", fs->ascent);
		printf("descent %d, ", fs->descent);
		printc("min bounds, ", &fs->min_bounds);
		printc("max bounds, ", &fs->max_bounds);
		size = (fs->max_byte1 - fs->min_byte1 + 1) *
			(fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1);
		printf("\nsize %d\n ", size);
		for (i=0; i<size; ++i) {
			char buf[32];
			if (!fs->per_char)
				continue;
			sprintf(buf, "char %d (%d):  ", i, i+fs->min_char_or_byte2);
			printc(buf, &fs->per_char[i]);
		}
		printf("\n");
	} else {
	printf("Could not load font: %s\n",font);
	}
	return 0;
}
