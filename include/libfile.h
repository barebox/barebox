/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __LIBFILE_H
#define __LIBFILE_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/bits.h>

struct resource;

int pread_full(int fd, void *buf, size_t size, loff_t offset);
int pwrite_full(int fd, const void *buf, size_t size, loff_t offset);
int write_full(int fd, const void *buf, size_t size);
int read_full(int fd, void *buf, size_t size);
int copy_fd(int in, int out);

ssize_t read_file_into_buf(const char *filename, void *buf, size_t size);

char *read_file_line(const char *fmt, ...) __printf(1, 2);

void *read_file(const char *filename, size_t *size);

void *read_fd(int fd, size_t *size);

int read_file_2(const char *filename, size_t *size, void **outbuf,
		loff_t max_size);

int write_file(const char *filename, const void *buf, size_t size);
int write_file_flash(const char *filename, const void *buf, size_t size);

#define COPY_FILE_VERBOSE	BIT(0)
#define COPY_FILE_NO_OVERWRITE	BIT(1)

int copy_file(const char *src, const char *dst, unsigned flags);

int copy_recursive(const char *src, const char *dst, unsigned flags);

int compare_file(const char *f1, const char *f2);

#if IN_PROPER
int open_and_lseek(const char *filename, int mode, loff_t pos);
#else
static inline int open_and_lseek(const char *filename, int mode, loff_t pos)
{
	return -ENOSYS;
}
#endif

/* Create a directory and its parents */
int make_directory(const char *pathname);

int unlink_recursive(const char *path, char **failedpath);

char *make_temp(const char *template);

int cache_file(const char *path, char **newpath);

struct resource *file_to_sdram(const char *filename, unsigned long adr);

#endif /* __LIBFILE_H */
