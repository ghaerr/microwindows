/*
 * Copyright (c) 2002 Alex Holden <alex@alexholden.net>
 *
 * A simple program to convert a binary PBM bitmap graphics file (use xbmtopbm
 * if your graphics program doesn't support it) into a Nano-X format bitmap
 * that can be included in a header file.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

char *strdup_toupper(char *in);
void out_word(unsigned short word);

char *strdup_toupper(char *in)
{
	char *p, *out;
      
	p = out = strdup(in);

	if(out) do { *p++ = toupper(*in); } while(*in++);

	return out;
}

void out_word(unsigned short word)
{
	static int words = 0;

	printf("0x%04x, ", word);

	if(++words == 8) {
		words = 0;
		printf("\n\t");
	}
}

int main(int argc, char *argv[])
{
	FILE *fp;
	size_t len;
	char *prefix;
	unsigned short word = 0;
	int width, height, x = 0, i, b, w = 0;
	unsigned char buf[BUFSIZ];

	if(argc != 3) {
		fprintf(stderr, "Usage: convpbm [input.pbm] prefix > "
				"output.h\n");
		return 1;
	}

	if(!(fp = fopen(argv[1], "r"))) {
		fprintf(stderr, "Couldn't open input file \"%s\"\n", argv[1]);
		return 1;
	}

	if(!(fgets(buf, BUFSIZ, fp))) goto readerr;

	if(strcmp(buf, "P4\n")) {
		fprintf(stderr, "Didn't find valid header. Perhaps this is "
				"not a binary PBM file?\n");
		return 1;
	}

	if(fscanf(fp, "%d %d\n", &width, &height) != 2) {
		fprintf(stderr, "Failed to read image dimensions\n");
		return 1;
	}

	if(!(prefix = strdup_toupper(argv[2]))) goto nomem;
	
	printf("/*\n * Microwindows bitmap converted by convbmp\n * Image "
		"dimensions: %d * %d\n * Prefix: %s\n * Original file name: %s"
		"\n */\n\n#define %s_WIDTH %d\n#define %s_HEIGHT %d\n\n"
		"static GR_BITMAP %s_bits[] = {\n\t", width, height, argv[2],
		argv[1], prefix, width, prefix, height, argv[2]);

	free(prefix);

	while(!feof(fp)) {
		len = fread(buf, 1, BUFSIZ, fp);
		for(i = 0; i < len; i++) {
			for(b = 0; b < 8; b++) {
				word <<= 1;
				word |= (buf[i] & 0x80) ? 0 : 1;
				buf[i] <<= 1;
				if(++x >= width) {
					x = 0;
					word <<= 8 - b - 1;
				       	if(w == 0) {
						word <<= 8;
						w = 1;
					}
					break;
				}
			}
			if(++w == 2) {
				w = 0;
				out_word(word);
				word = 0;
			}
		}
	}

	printf("};\n");
	
	fclose(fp);
	return 0;

readerr:
	fprintf(stderr, "Read error: %s\n", strerror(errno));
	return 1;

nomem:
	fprintf(stderr, "Out of memory\n");
	return 1;

}
