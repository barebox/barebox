// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>

#include "common.h"

int read_file_2(const char *filename, size_t *size, void **outbuf, size_t max_size)
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

	buf = mmap(NULL, max_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (buf == MAP_FAILED ) {
		buf = malloc(max_size);
		if (!buf) {
			fprintf(stderr, "Cannot allocate memory\n");
			ret = -ENOMEM;
			goto close;
		}

		*outbuf = buf;

		while (*size < max_size) {
			rsize = read(fd, buf, max_size - *size);
			if (rsize == 0) {
				ret = -EIO;
				goto free;
			}

			if (rsize < 0) {
				ret = -errno;
				goto free;
			}

			buf += rsize;
			*size += rsize;
		}
	} else {
		*outbuf = buf;
		*size = max_size;
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

void *read_file(const char *filename, size_t *size)
{
	int ret;
	void *buf;

	ret = read_file_2(filename, size, &buf, (size_t)-1);
	if (!ret)
		return buf;

	errno = -ret;

	return NULL;
}

int write_file(const char *filename, const void *buf, size_t size)
{
	int fd, ret = 0;
	int now;

	fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		fprintf(stderr, "Cannot open %s: %s\n", filename, strerror(errno));
		return -errno;
	}

	while (size) {
		now = write(fd, buf, size);
		if (now < 0) {
			fprintf(stderr, "Cannot write to %s: %s\n", filename,
				strerror(errno));
			ret = -errno;
			goto out;
		}
		size -= now;
		buf += now;
	}

out:
	close(fd);

	return ret;
}

int read_full(int fd, void *buf, size_t size)
{
	size_t insize = size;
	int now;
	int total = 0;

	while (size) {
		now = read(fd, buf, size);
		if (now == 0)
			return total;
		if (now < 0)
			return now;
		total += now;
		size -= now;
		buf += now;
	}

	return insize;
}

int write_full(int fd, void *buf, size_t size)
{
	size_t insize = size;
	int now;

	while (size) {
		now = write(fd, buf, size);
		if (now <= 0)
			return now;
		size -= now;
		buf += now;
	}

	return insize;
}
