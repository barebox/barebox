/*
 * bareboxenv.c - generate or read a barebox environment archive
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
#include <libgen.h>

#include "compiler.h"

#define debug(...)
#define printk_once(...)

/* Find out if the last character of a string matches the one given.
 * Don't underrun the buffer if the string length is 0.
 */
static char *last_char_is(const char *s, int c)
{
	if (s && *s) {
		size_t sz = strlen(s) - 1;
		s += sz;
		if ( (unsigned char)*s == c)
			return (char*)s;
	}
	return NULL;
}

enum {
	ACTION_RECURSE     = (1 << 0),
	ACTION_FOLLOWLINKS = (1 << 1),
	ACTION_DEPTHFIRST  = (1 << 2),
	/*ACTION_REVERSE   = (1 << 3), - unused */
	ACTION_SORT        = (1 << 4),
};

int recursive_action(const char *fileName, unsigned flags,
	int (*fileAction) (const char *fileName, struct stat* statbuf, void* userData, int depth),
	int (*dirAction) (const char *fileName, struct stat* statbuf, void* userData, int depth),
	void* userData, const unsigned depth);

#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))

/* concatenate path and file name to new allocation buffer,
 * not adding '/' if path name already has '/'
 */
static char *concat_path_file(const char *path, const char *filename)
{
	char *lc, *str;

	if (!path)
		path = "";
	lc = last_char_is(path, '/');
	while (*filename == '/')
		filename++;

	str = xmalloc(strlen(path) + (lc==0 ? 1 : 0) + strlen(filename) + 1);
	sprintf(str, "%s%s%s", path, (lc==NULL ? "/" : ""), filename);

	return str;
}

/*
 * This function make special for recursive actions with usage
 * concat_path_file(path, filename)
 * and skipping "." and ".." directory entries
 */

static char *concat_subpath_file(const char *path, const char *f)
{
	if (f && DOT_OR_DOTDOT(f))
		return NULL;
	return concat_path_file(path, f);
}

#include <linux/list.h>
#include <linux/list_sort.h>
#include "../lib/list_sort.c"
#include "../lib/recursive_action.c"
#include "../include/envfs.h"
#include "../crypto/crc32.c"
#include "../lib/make_directory.c"
#include "../common/environment.c"

static void usage(char *prgname)
{
	printf( "Usage : %s [OPTION] DIRECTORY FILE\n"
		"Load a barebox environment sector into a directory or\n"
		"save a directory into a barebox environment sector\n"
		"\n"
		"options:\n"
		"  -s        save (directory -> environment sector)\n"
		"  -z        force the built-in default environment at startup\n"
		"  -l        load (environment sector -> directory)\n"
		"  -p <size> pad output file to given size\n"
		"  -v        verbose\n",
		prgname);
}

int main(int argc, char *argv[])
{
	int opt;
	int save = 0, load = 0, pad = 0, err = 0, fd;
	char *filename = NULL, *dirname = NULL;
	unsigned envfs_flags = 0;
	int verbose = 0;

	while((opt = getopt(argc, argv, "slp:vz")) != -1) {
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
		case 'z':
			envfs_flags |= ENVFS_FLAGS_FORCE_BUILT_IN;
			save = 1;
			break;
		case 'v':
			verbose = 1;
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
		if (verbose)
			printf("loading env from file %s to %s\n", filename, dirname);

		err = envfs_load(filename, dirname, 0);

		if (verbose && err)
			printf("loading env failed: %d\n", err);
	}
	if (save) {
		if (verbose)
			printf("saving contents of %s to file %s\n", dirname, filename);

		err = envfs_save(filename, dirname, envfs_flags);

		if (verbose && err)
			printf("saving env failed: %d\n", err);
	}
	exit(err ? 1 : 0);
}
