/*
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <common.h>
#include <fs.h>
#include <fcntl.h>
#include <malloc.h>
#include <libfile.h>
#include <progress.h>
#include <stdlib.h>
#include <linux/stat.h>

/*
 * pwrite_full - write to filedescriptor at offset
 *
 * Like pwrite, but guarantees to write the full buffer out, else it
 * returns with an error.
 */
int pwrite_full(int fd, const void *buf, size_t size, loff_t offset)
{
	size_t insize = size;
	int now;

	while (size) {
		now = pwrite(fd, buf, size, offset);
		if (now == 0) {
			errno = ENOSPC;
			return -errno;
		}
		if (now < 0)
			return now;
		size -= now;
		buf += now;
		offset += now;
	}

	return insize;
}
EXPORT_SYMBOL(pwrite_full);

/*
 * write_full - write to filedescriptor
 *
 * Like write, but guarantees to write the full buffer out, else
 * it returns with an error.
 */
int write_full(int fd, const void *buf, size_t size)
{
	size_t insize = size;
	int now;

	while (size) {
		now = write(fd, buf, size);
		if (now == 0) {
			errno = ENOSPC;
			return -errno;
		}
		if (now < 0)
			return now;
		size -= now;
		buf += now;
	}

	return insize;
}
EXPORT_SYMBOL(write_full);

/*
 * read_full - read from filedescriptor
 *
 * Like read, but this function only returns less bytes than
 * requested when the end of file is reached.
 */
int read_full(int fd, void *buf, size_t size)
{
	size_t insize = size;
	int now;

	while (size) {
		now = read(fd, buf, size);
		if (now == 0)
			break;
		if (now < 0)
			return now;
		size -= now;
		buf += now;
	}

	return insize - size;
}
EXPORT_SYMBOL(read_full);

/*
 * read_file_line - read a line from a file
 *
 * Used to compose a filename from a printf format and to read a line from this
 * file. All leading and trailing whitespaces (including line endings) are
 * removed. The returned buffer must be freed with free(). This function is
 * supposed for reading variable like content into a buffer, so files > 1024
 * bytes are ignored.
 */
char *read_file_line(const char *fmt, ...)
{
	va_list args;
	char *filename;
	char *buf, *line = NULL;
	size_t size;
	int ret;
	struct stat s;

	va_start(args, fmt);
	filename = bvasprintf(fmt, args);
	va_end(args);

	ret = stat(filename, &s);
	if (ret)
		goto out;

	if (s.st_size > 1024)
		goto out;

	buf = read_file(filename, &size);
	if (!buf)
		goto out;

	line = strim(buf);

	line = xstrdup(line);
	free(buf);
out:
	free(filename);
	return line;
}
EXPORT_SYMBOL_GPL(read_file_line);

/**
 * read_file_2 - read a file to an allocated buffer
 * @filename:  The filename to read
 * @size:      After successful return contains the size of the file
 * @outbuf:    contains a pointer to the file data after successful return
 * @max_size:  The maximum size to read. Use FILESIZE_MAX for reading files
 *             of any size.
 *
 * This function reads a file to an allocated buffer. At maximum @max_size
 * bytes are read. The actual read size is returned in @size. -EFBIG is
 * returned if the file is bigger than @max_size, but the buffer is read
 * anyway up to @max_size in this case. Free the buffer with free() after
 * usage. The allocated buffer is actually one byte bigger than the file
 * and the extra byte is initialized to '\0' so that the returned buffer
 * can safely be interpreted as a string.
 *
 * Return: 0 for success, or negative error code. -EFBIG is returned
 * when the file has been bigger than max_size.
 */
int read_file_2(const char *filename, size_t *size, void **outbuf,
		loff_t max_size)
{
	int fd;
	struct stat s;
	void *buf = NULL;
	const char *tmpfile = "/.read_file_tmp";
	int ret;
	loff_t read_size;

again:
	ret = stat(filename, &s);
	if (ret)
		return ret;

	if (max_size == FILESIZE_MAX)
		read_size = s.st_size;
	else
		read_size = max_size;

	if (read_size == FILESIZE_MAX) {
		ret = copy_file(filename, tmpfile, 0);
		if (ret)
			return ret;
		filename = tmpfile;
		goto again;
	}

	buf = calloc(read_size + 1, 1);
	if (!buf) {
		ret = -ENOMEM;
		errno = ENOMEM;
		goto err_out;
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		ret = fd;
		goto err_out;
	}

	ret = read_full(fd, buf, read_size);
	if (ret < 0)
		goto err_out1;

	close(fd);

	if (size)
		*size = ret;

	if (filename == tmpfile)
		unlink(tmpfile);

	*outbuf = buf;

	if (read_size < s.st_size)
		return -EFBIG;

	return 0;

err_out1:
	close(fd);
err_out:
	free(buf);

	if (filename == tmpfile)
		unlink(tmpfile);

	return ret;
}
EXPORT_SYMBOL(read_file_2);

/**
 * read_file - read a file to an allocated buffer
 * @filename:  The filename to read
 * @size:      After successful return contains the size of the file
 *
 * This function reads a file to an allocated buffer.
 * Some TFTP servers do not transfer the size of a file. In this case
 * the file is first read to a temporary file.
 *
 * Return: The buffer containing the file or NULL on failure
 */
void *read_file(const char *filename, size_t *size)
{
	int ret;
	void *buf;

	ret = read_file_2(filename, size, &buf, FILESIZE_MAX);
	if (!ret)
		return buf;

	return NULL;
}
EXPORT_SYMBOL(read_file);

/**
 * write_file - write a buffer to a file
 * @filename:    The filename to write
 * @size:        The size of the buffer
 *
 * Return: 0 for success or negative error value
 */
int write_file(const char *filename, const void *buf, size_t size)
{
	int fd, ret;

	fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT);
	if (fd < 0)
		return fd;

	ret = write_full(fd, buf, size);

	close(fd);

	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(write_file);

/**
 * write_file_flash - write a buffer to a file backed by flash
 * @filename:    The filename to write
 * @size:        The size of the buffer
 *
 * Functional this is identical to write_file but calls erase() before writing.
 *
 * Return: 0 for success or negative error value
 */
int write_file_flash(const char *filename, const void *buf, size_t size)
{
	int fd, ret;

	fd = open(filename, O_WRONLY);
	if (fd < 0)
		return fd;

	ret = erase(fd, size, 0);
	if (ret < 0)
		goto out_close;

	ret = write_full(fd, buf, size);

out_close:
	close(fd);

	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(write_file_flash);

/**
 * copy_file - Copy a file
 * @src:	The source filename
 * @dst:	The destination filename
 * @verbose:	if true, show a progression bar
 *
 * Return: 0 for success or negative error code
 */
int copy_file(const char *src, const char *dst, int verbose)
{
	char *rw_buf = NULL;
	int srcfd = 0, dstfd = 0;
	int r;
	int ret = 1, err1 = 0;
	int mode;
	int total = 0;
	struct stat srcstat, dststat;

	rw_buf = xmalloc(RW_BUF_SIZE);

	srcfd = open(src, O_RDONLY);
	if (srcfd < 0) {
		printf("could not open %s: %s\n", src, errno_str());
		goto out;
	}

	mode = O_WRONLY | O_CREAT;

	ret = stat(dst, &dststat);
	if (ret && ret != -ENOENT)
		goto out;

	/* Set O_TRUNC only if file exist and is a regular file */
	if (!ret && S_ISREG(dststat.st_mode))
		mode |= O_TRUNC;

	dstfd = open(dst, mode);
	if (dstfd < 0) {
		printf("could not open %s: %s\n", dst, errno_str());
		goto out;
	}

	if (verbose) {
		if (stat(src, &srcstat) < 0)
			srcstat.st_size = 0;

		init_progression_bar(srcstat.st_size);
	}

	while (1) {
		r = read(srcfd, rw_buf, RW_BUF_SIZE);
		if (r < 0) {
			perror("read");
			goto out;
		}
		if (!r)
			break;

		if (write_full(dstfd, rw_buf, r) < 0) {
			perror("write");
			goto out;
		}

		total += r;

		if (verbose) {
			if (srcstat.st_size && srcstat.st_size != FILESIZE_MAX)
				show_progress(total);
			else
				show_progress(total / 16384);
		}
	}

	ret = 0;
out:
	if (verbose)
		putchar('\n');

	free(rw_buf);
	if (srcfd > 0)
		close(srcfd);
	if (dstfd > 0)
		err1 = close(dstfd);

	return ret ?: err1;
}
EXPORT_SYMBOL(copy_file);

int copy_recursive(const char *src, const char *dst)
{
	struct stat s;
	DIR *dir;
	struct dirent *d;
	int ret;

	ret = stat(src, &s);
	if (ret)
		return ret;

	if (!S_ISDIR(s.st_mode))
		return copy_file(src, dst, 0);

	ret = make_directory(dst);
	if (ret)
		return ret;

	dir = opendir(src);
	if (!dir)
		return -EIO;

	while ((d = readdir(dir))) {
		char *from, *to;
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		from = basprintf("%s/%s", src, d->d_name);
		to = basprintf("%s/%s", dst, d->d_name);
		ret = copy_recursive(from, to);
		if (ret)
			break;
		free(from);
		free(to);
	}
	closedir(dir);

	return ret;
}

/**
 * compare_file - Compare two files
 * @f1:		The first file
 * @f2:		The second file
 *
 * Return: 0 if both files are identical, 1 if they differ,
 *         a negative error code if some error occured
 */
int compare_file(const char *f1, const char *f2)
{
	int fd1, fd2, ret;
	struct stat s1, s2;
	void *buf1, *buf2;
	loff_t left;

	fd1 = open(f1, O_RDONLY);
	if (fd1 < 0)
		return -errno;

	fd2 = open(f2, O_RDONLY);
	if (fd2 < 0) {
		ret = -errno;
		goto err_out1;
	}

	ret = fstat(fd1, &s1);
	if (ret)
		goto err_out2;

	ret = fstat(fd2, &s2);
	if (ret)
		goto err_out2;

	if (s1.st_size != s2.st_size)
		return 1;

	buf1 = xmalloc(RW_BUF_SIZE);
	buf2 = xmalloc(RW_BUF_SIZE);

	left = s1.st_size;
	while (left) {
		loff_t now = min(left, (loff_t)RW_BUF_SIZE);

		ret = read_full(fd1, buf1, now);
		if (ret < 0)
			goto err_out3;

		ret = read_full(fd2, buf2, now);
		if (ret < 0)
			goto err_out3;

		if (memcmp(buf1, buf2, now)) {
			ret = 1;
			goto err_out3;
		}

		left -= now;
	}

	ret = 0;

err_out3:
	free(buf1);
	free(buf2);
err_out2:
	close(fd2);
err_out1:
	close(fd1);
	return ret;
}

/**
 * open_and_lseek - open file and lseek to position
 * @filename:	The file to open
 * @mode:	The file open mode
 * @pos:	The position to lseek to
 *
 * Return: If successful this function returns a positive
 *         filedescriptor number, otherwise -1 is returned
 */
int open_and_lseek(const char *filename, int mode, loff_t pos)
{
	int fd;

	fd = open(filename, mode);
	if (fd < 0) {
		perror("open");
		return fd;
	}

	if (!pos)
		return fd;

	if (mode & (O_WRONLY | O_RDWR)) {
		struct stat s;

		if (fstat(fd, &s)) {
			perror("fstat");
			goto out;
		}

		if (s.st_size < pos && ftruncate(fd, pos)) {
			perror("ftruncate");
			goto out;
		}
	}

	if (lseek(fd, pos, SEEK_SET) != pos) {
		perror("lseek");
		goto out;
	}

	return fd;
out:
	close(fd);
	return -1;
}

/**
 * make_temp - create a name for a temporary file
 * @template:	The filename prefix
 *
 * This function creates a name for a temporary file. @template is used as a
 * template for the name which gets appended a 8-digit hexadecimal number to
 * create a unique filename.
 *
 * Return: This function returns a filename which can be used as a temporary
 *         file later on. The returned filename must be freed by the caller.
 */
char *make_temp(const char *template)
{
	char *name = NULL;
	struct stat s;
	int ret;

	do {
		free(name);
		name = basprintf("/tmp/%s-%08x", template, random32());
		ret = stat(name, &s);
	} while (!ret);

	return name;
}

/**
 * cache_file - Cache a file in /tmp
 * @path:	The file to cache
 * @newpath:	The return path where the file is copied to
 *
 * This function copies a given file to /tmp and returns its name in @newpath.
 * @newpath is dynamically allocated and must be freed by the caller.
 *
 * Return: 0 for success, negative error code otherwise.
 */
int cache_file(const char *path, char **newpath)
{
	char *npath;
	int ret;

	npath = make_temp("filecache");

	ret = copy_file(path, npath, 0);
	if (ret) {
		free(npath);
		return ret;
	}

	*newpath = npath;

	return 0;
}
