/*
 * ubootenv.c - generate or read a U-Boot environment archive
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

void *xmalloc(size_t size)
{
	void *p = NULL;

	if (!(p = malloc(size))) {
		printf("ERROR: out of memory\n");
		exit(1);
	}

	return p;
}

#include "../include/envfs.h"
#include "../commands/environment.c"

void usage(char *prgname)
{
	printf( "Usage : %s [OPTION] DIRECTORY FILE\n"
		"Load an u-boot environment sector into a directory or\n"
		"save a directory into an u-boot environment sector\n"
		"\n"
		"options:\n"
		"  -s        save (directory -> environment sector)\n"
		"  -l        load (environment sector -> directory)\n"
		"  -p <size> pad output file to given size\n",
		prgname);
}

int main(int argc, char *argv[])
{
	int opt;
	int save = 0, load = 0, pad = 0, fd;
	char *filename = NULL, *dirname = NULL;

	while((opt = getopt(argc, argv, "slp:")) != -1) {
		switch (opt) {
		case 's':
			save = 1;
			break;
		case 'l':
			load = 1;
			break;
		case 'p':
			pad = strtoul(optarg, NULL, 0);
			break;
		}
	}

	if (optind + 1 >= argc) {
		usage(argv[0]);
		exit(1);
	}

	dirname = argv[optind];
	filename = argv[optind + 1];

	if ((!load && !save) || (load && save) || !filename || !dirname) {
		usage(argv[0]);
		exit(1);
	}

	if (save) {
		fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
		if (fd < 0) {
			perror("open");
			exit(1);
		}
		close(fd);
	}

	if (save && pad) {
		if (truncate(filename, pad)) {
			perror("truncate");
			exit(1);
		}
	}

	if (load) {
		printf("loading env from file %s to %s\n", filename, dirname);
		envfs_load(filename, dirname);
	}
	if (save) {
		printf("saving contents of %s to file %s\n", dirname, filename);
		envfs_save(filename, dirname);
	}
	exit(0);
}
