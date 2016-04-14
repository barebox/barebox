/*
 * (C) Copyright 2014 Sascha Hauer, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN(x, a)		__ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <asm-generic/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <linux/err.h>

#include "../include/image-metadata.h"

#define eprintf(args...) fprintf(stderr, ## args)

static void debug(const char *fmt, ...)
{
	va_list ap;

	if (!imd_command_verbose)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

int imd_command_setenv(const char *variable_name, const char *value)
{
	fprintf(stderr, "-s option ignored\n");

	return -EINVAL;
}

static int read_file_2(const char *filename, size_t *size, void **outbuf, loff_t max_size)
{
	off_t fsize;
	ssize_t rsize;
	int ret, fd;
	void *buf;

	*size = 0;
	*outbuf = NULL;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open %s: %s\n", filename, strerror(errno));
		return -errno;
	}

	fsize = lseek(fd, 0, SEEK_END);
	if (fsize == -1) {
		fprintf(stderr, "Cannot get size %s: %s\n", filename, strerror(errno));
		ret = -errno;
		goto close;
	}

	if (fsize < max_size)
		max_size = fsize;

	if (lseek(fd, 0, SEEK_SET) == -1) {
		fprintf(stderr, "Cannot seek to start %s: %s\n", filename, strerror(errno));
		ret = -errno;
		goto close;
	}

	buf = malloc(max_size);
	if (!buf) {
		fprintf(stderr, "Cannot allocate memory\n");
		ret = -ENOMEM;
		goto close;
	}

	*outbuf = buf;
	while (*size < max_size) {
		rsize = read(fd, buf, max_size-*size);
		if (rsize == 0) {
			ret = -EIO;
			goto free;
		} else if (rsize < 0) {
			if (errno == EAGAIN)
				continue;
			else {
				ret = -errno;
				goto free;
			}
		} /* ret > 0 */
		buf += rsize;
		*size += rsize;
	}

	ret = 0;
	goto close;
free:
	*outbuf = NULL;
	free(buf);
close:
	close(fd);
	return ret;
}

static unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	return strtoul(cp, endp, base);
}

#include "../common/imd.c"

static void usage(const char *prgname)
{
	printf(
"Extract metadata from a barebox image\n"
"\n"
"Usage: %s [OPTIONS] FILE\n"
"Options:\n"
"-t <type>    only show information of <type>\n"
"-n <no>      for tags with multiple strings only show string <no>\n"
"\n"
"Without options all information available is printed. Valid types are:\n"
"release, build, model, of_compatible\n",
	prgname);
}

int main(int argc, char *argv[])
{
	int ret;

	ret = imd_command(argc, argv);
	if (ret == -ENOSYS) {
		usage(argv[0]);
		exit(1);
	}

	if (ret)
		fprintf(stderr, "%s\n", strerror(-ret));

	return ret ? 1 : 0;
}
