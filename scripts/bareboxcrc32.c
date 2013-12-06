/*
 * bareboxcrc32.c - generate crc32 checksum in little endian
 *
 * Copyright (c) 2013 Michael Grzeschik <mgr@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>

#include "compiler.h"

#define debug(...)

#include "../crypto/crc32.c"

int main(int argc, char *argv[])
{
	loff_t start = 0, size = ~0;
	ulong crc = 0, total = 0;
	char *filename = NULL;
	int i;

	if (!filename && argc < 2) {
		printf("usage: %s filename\n", argv[0]);
		exit(1);
	}

	for (i = 1; i < argc; i++) {
		filename = argv[i];
		if (file_crc(filename, start, size, &crc, &total) < 0)
			exit(1);
		printf("%08lx\t%s\n", crc, filename);
	}

	exit(0);

}
