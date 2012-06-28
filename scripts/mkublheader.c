/*
 * mkublheader.c - produce the header needed to load barebox on OMAP-L138
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

#define MAGICNUM 0xa1aced00

struct ubl_header
{
  uint32_t magicNum; /* Expected magic number */
  uint32_t epAddr;   /* Entry point of the user application */
  uint32_t imgSize;  /* Number of bytes of the application image */
  uint32_t imgAddr;  /* SPI memory offset where application image is located */
  uint32_t ldAddr;   /* Address where image is copied to */
};

void usage(char *prgname)
{
	printf( "Usage : %s [OPTION] FILE > HEADER\n"
		"\n"
		"options:\n"
		"  -a <address> image flash address\n"
		"  -e <address> entry point memory address\n"
		"  -l <address> load memory address\n",
		prgname);
}

int main(int argc, char *argv[])
{
	struct stat sb;
	struct ubl_header uh;
	int opt;
	uint32_t imgAddr = 0x00040000 + sizeof(uh);
	uint32_t epAddr = 0xc1080000, ldAddr = 0xc1080000;

	while((opt = getopt(argc, argv, "ael:")) != -1) {
		switch (opt) {
		case 'a':
			imgAddr = strtoul(optarg, NULL, 0);
			break;
		case 'e':
			epAddr = strtoul(optarg, NULL, 0);
			break;
		case 'l':
			ldAddr = strtoul(optarg, NULL, 0);
			break;
		}
	}

	if (optind >= argc) {
		usage(argv[0]);
		exit(1);
	}

	if (stat(argv[optind], &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	uh.magicNum = htole32(MAGICNUM);
	uh.epAddr = htole32(epAddr);
	uh.imgSize = htole32((uint32_t)sb.st_size);
	uh.imgAddr = htole32(imgAddr);
	uh.ldAddr = htole32(ldAddr);

	fwrite(&uh, sizeof(uh), 1, stdout);

	exit(EXIT_SUCCESS);
}
