/*
 * Copyright (C) 2012 Alexey Galakhov
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

#define DEFAULT_BUF_SIZE 8192

static int usage(const char* me)
{
	printf("Usage: %s <input> <output> [<bufsize]\n", me);
	return 1;
}

static void put32(uint8_t *ptr, uint32_t value)
{
	ptr[0] = value & 0xFF;
	ptr[1] = (value >> 8) & 0xFF;
	ptr[2] = (value >> 16) & 0xFF;
	ptr[3] = (value >> 24) & 0xFF;
}

static size_t safe_fread(void *buf, size_t len, FILE* file)
{
	size_t rd = fread(buf, 1, len, file);
	if (! rd) {
		if (ferror(file))
			fprintf(stderr, "Error reading file: %s\n", strerror(errno));
		else
			fprintf(stderr, "Unexpected end of file\n");
	}
	return rd;
}

static size_t safe_fwrite(const void *buf, size_t len, FILE* file)
{
	size_t wr = fwrite(buf, 1, len, file);
	if (wr != len) {
		fprintf(stderr, "Error writing file: %s\n", strerror(errno));
		return 0;
	}
	return wr;
}

static int process(FILE *input, FILE *output, uint8_t *buf, unsigned bufsize)
{
	size_t rd;
	unsigned i;
	uint32_t cksum;
	/* Read first chunk */
	rd = safe_fread(buf + 16, bufsize - 16, input);
	if (! rd)
		return 4;
	/* Calculate header */
	put32(buf + 0, bufsize);
	cksum = 0;
	for (i = 16; i < bufsize; ++i)
		cksum += (uint32_t)buf[i];
	put32(buf + 8, cksum);
	if (! safe_fwrite(buf, bufsize, output))
		return 4;
	/* Copy the rest of file */
	while (! feof(input)) {
		rd = safe_fread(buf, bufsize, input);
		if (! rd)
			return 4;
		if (! safe_fwrite(buf, rd, output))
			return 4;
	}
	return 0;
}

static int work(const char* me, const char *infile, const char *outfile, unsigned bufsize)
{
	uint8_t *buf;
	FILE *input;
	FILE *output;
	int ret;
	if (bufsize < 512 || bufsize > 65536)
		return usage(me);
	buf = calloc(1, bufsize);
	if (! buf) {
		fprintf(stderr, "Unable to allocate %u bytes of memory\n", bufsize);
		return 2;
	}
	input = fopen(infile, "r");
	if (! input) {
		fprintf(stderr, "Cannot open `%s' for reading\n", infile);
		free(buf);
		return 3;
	}
	output = fopen(outfile, "w");
	if (! output) {
		fprintf(stderr, "Cannot open `%s' for writing\n", outfile);
		fclose(input);
		free(buf);
		return 3;
	}

	ret = process(input, output, buf, bufsize);

	fclose(output);
	fclose(input);
	free(buf);
	return ret;
}

int main(int argc, char** argv)
{
	switch (argc) {
	case 3:
		return work(argv[0], argv[1], argv[2], DEFAULT_BUF_SIZE);
	case 4:
		return work(argv[0], argv[1], argv[2], atoi(argv[3]));
	default:
		return usage(argv[0]);
	}
}
