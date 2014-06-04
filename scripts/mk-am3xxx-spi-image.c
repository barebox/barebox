/*
 * mk-am35xx-spi-image.c - convert a barebox image for SPI loading on AM35xx
 *
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/**
 * @file
 * @brief convert a barebox image for SPI loading on AM35xx
 *
 * FileName: scripts/mk-am35xx-spi-image.c
 *
 * Booting from SPI on an AM35xx (and possibly other TI SOCs) requires
 * a special format:
 *
 * - 32 bit image size in big-endian
 * - 32 bit load address in big-endian
 * - binary image converted from little- to big-endian
 *
 * This tool converts barebox.bin to the required format.
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <endian.h>

enum soc {
	SOC_AM33XX,
	SOC_AM35XX,
	SOC_UNKNOWN,
};

static char *soc_names[] = {
	[SOC_AM33XX] = "am33xx",
	[SOC_AM35XX] = "am35xx",
};

void usage(char *prgname)
{
	printf("usage: %s [OPTION] FILE > IMAGE\n"
	       "\n"
	       "options:\n"
	       "  -a <address> memory address for the loaded image in SRAM\n"
	       "  -s <soc>     SoC to use (am33xx, am35xx)\n",
	       prgname);
}

int main(int argc, char *argv[])
{
	FILE *input;
	int opt, i;
	off_t pos;
	size_t size;
	uint32_t addr = 0x40200000;
	uint32_t temp;
	enum soc soc = SOC_UNKNOWN;
	char *socname = NULL;

	while((opt = getopt(argc, argv, "a:s:")) != -1) {
		switch (opt) {
		case 'a':
			addr = strtoul(optarg, NULL, 0);
			break;
		case 's':
			socname = optarg;
			break;
		}
	}

	if (!socname) {
		fprintf(stderr, "SoC not specified. Use -s <soc>\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < 2; i++) {
		if (!strcmp(socname, soc_names[i])) {
			soc = i;
			break;
		}
	}

	if (soc == SOC_UNKNOWN) {
		fprintf(stderr, "SoC %s unknown\n", socname);
		exit(EXIT_FAILURE);
	}

	if (optind >= argc) {
		usage(argv[0]);
		exit(1);
	}

	input = fopen(argv[optind], "r");
	if (input == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	if (fseeko(input, 0, SEEK_END) == -1) {
		perror("fseeko");
		exit(EXIT_FAILURE);
	}

	pos = ftello(input);
	if (pos == -1) {
		perror("ftello");
		exit(EXIT_FAILURE);
	}
	if (pos > 0x100000) {
		fprintf(stderr, "error: image should be smaller than 1 MiB\n");
		exit(EXIT_FAILURE);
	}

	if (fseeko(input, 0, SEEK_SET) == -1) {
		perror("fseeko");
		exit(EXIT_FAILURE);
	}

	pos = (pos + 3) & ~3;

	/* image size */
	if (soc == SOC_AM35XX) {
		temp = htobe32((uint32_t)pos);
		fwrite(&temp, sizeof(uint32_t), 1, stdout);

		/* memory address */
		temp = htobe32(addr);
		fwrite(&temp, sizeof(uint32_t), 1, stdout);
	}

	for (;;) {
		size = fread(&temp, 1, sizeof(uint32_t), input);
		if (!size)
			break;
		if (size < 4 && !feof(input)) {
			perror("fread");
			exit(EXIT_FAILURE);
		}
		temp = htobe32(le32toh(temp));
		if (fwrite(&temp, 1, sizeof(uint32_t), stdout) != 4) {
			perror("fwrite");
			exit(EXIT_FAILURE);
		}
	}

	if (fclose(input) != 0) {
		perror("fclose");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
