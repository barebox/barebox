// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "common.h"
#include "compiler.h"

int read_file_2(const char *filename, size_t *size, void **outbuf, size_t max_size)
{
	off_t fsize;
	ssize_t read_size, now;
	int ret, fd;
	void *buf;

	*size = 0;
	*outbuf = NULL;

	fd = open(filename, O_RDONLY | O_BINARY);
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

	if (max_size < fsize)
		read_size = max_size;
	else
		read_size = fsize;

	if (lseek(fd, 0, SEEK_SET) == -1) {
		fprintf(stderr, "Cannot seek to start %s: %s\n", filename, strerror(errno));
		ret = -errno;
		goto close;
	}

	buf = malloc(read_size);
	if (!buf) {
		fprintf(stderr, "Cannot allocate memory\n");
		ret = -ENOMEM;
		goto close;
	}

	*outbuf = buf;

	while (read_size) {
		now = read(fd, buf, read_size);
		if (now == 0) {
			ret = -EIO;
			goto free;
		}

		if (now < 0) {
			ret = -errno;
			goto free;
		}

		buf += now;
		*size += now;
		read_size -= now;
	}

	ret = 0;
	goto close;
free:
	free(*outbuf);
	*outbuf = NULL;
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

/**
 * read_fd - read from a file descriptor to an allocated buffer
 * @filename:  The file descriptor to read
 * @size:      After successful return contains the size of the file
 *
 * This function reads a file descriptor from offset 0 until EOF to an
 * allocated buffer.
 *
 * Return: On success, returns a nul-terminated buffer with the file's
 * contents that should be deallocated with free().
 * On error, NULL is returned and errno is set to an error code.
 */
void *read_fd(int fd, size_t *out_size)
{
	struct stat st;
	ssize_t ret;
	void *buf;

	ret = fstat(fd, &st);
	if (ret < 0)
		return NULL;

	/* For user convenience, we always nul-terminate the buffer in
	 * case it contains a string. As we don't want to assume the string
	 * to be either an array of char or wchar_t, we just unconditionally
	 * add 2 bytes as terminator. As the two byte terminator needs to be
	 * aligned, we just make it three bytes
	 */
	buf = malloc(st.st_size + 3);
	if (!buf)
		return NULL;

	ret = pread_full(fd, buf, st.st_size, 0);
	if (ret < 0) {
		free(buf);
		return NULL;
	}

	memset(buf + st.st_size, '\0', 3);
	*out_size = st.st_size;

	return buf;
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

/*
 * pread_full - read to filedescriptor at offset
 *
 * Like pread, but this function only returns less bytes than
 * requested when the end of file is reached.
 */
int pread_full(int fd, void *buf, size_t size, loff_t offset)
{
	size_t insize = size;
	int now;

	while (size) {
		now = pread(fd, buf, size, offset);
		if (now == 0)
			break;
		if (now < 0)
			return now;
		size -= now;
		buf += now;
	}

	return insize - size;
}

int write_full(int fd, const void *buf, size_t size)
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
