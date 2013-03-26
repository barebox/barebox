/*
 * Copyright (C) 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <endian.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static void usage(char *name)
{
	printf("Usage: %s barebox-flash-image <outfile>\n", name);
}

int main(int argc, char *argv[])
{
	FILE *ifile, *ofile;
	unsigned int *buf;
	const char *infile;
	const char *outfile;
	struct stat st;
	unsigned int i;
	unsigned long sum = 0;

	if (argc != 3) {
		usage(argv[0]);
		exit(1);
	}

	infile = argv[1];
	outfile = argv[2];

	if (stat(infile, &st) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	buf = malloc(st.st_size);
	if (!buf) {
		fprintf(stderr, "Unable to allocate buffer\n");
		return -1;
	}
	ifile = fopen(infile, "rb");
	if (!ifile) {
		fprintf(stderr, "Cannot open %s for reading\n",
			infile);
		free(buf);
		exit(EXIT_FAILURE);
	}
	ofile = fopen(outfile, "wb");
	if (!ofile) {
		fprintf(stderr, "Cannot open %s for writing\n",
			outfile);
		fclose(ifile);
		free(buf);
		exit(EXIT_FAILURE);
	}

	fread(buf, 4, st.st_size, ifile);

	for (i = 0x8; i < 0x12; i++)
		sum += htole32(buf[i]);

	sum = ~sum;
	buf[i] = sum;

	fwrite(buf, st.st_size / 4, 4, ofile);

	fclose(ofile);
	fclose(ifile);
	free(buf);

	return 0;
}
