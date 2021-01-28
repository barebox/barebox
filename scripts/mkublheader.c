// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2012 Jan Luebbe <j.luebbe@pengutronix.de>

/* mkublheader.c - produce the header needed to load barebox on OMAP-L138 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

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

static void usage(char *prgname)
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
