/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __UNISTD_H
#define __UNISTD_H

#include <linux/types.h>
#include <fcntl.h>
#include <errno.h>

#define RW_BUF_SIZE	(unsigned)4096

struct stat;

#if IN_PROPER
int unlinkat(int dirfd, const char *pathname, int flags);
int close(int fd);
int lstatat(int dirfd, const char *filename, struct stat *s);
int statat(int dirfd, const char *filename, struct stat *s);
int fstat(int fd, struct stat *s);
ssize_t read(int fd, void *buf, size_t count);
ssize_t pread(int fd, void *buf, size_t count, loff_t offset);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t pwrite(int fd, const void *buf, size_t count, loff_t offset);
loff_t lseek(int fildes, loff_t offset, int whence);
int symlink(const char *pathname, const char *newpath);
int readlinkat(int dirfd, const char *path, char *buf, size_t bufsiz);
int chdir(const char *pathname);
char *pushd(const char *dir);
int popd(char *dir);
const char *getcwd(void);
int ftruncate(int fd, loff_t length);
#else
static inline int unlinkat(int dirfd, const char *pathname, int flags)
{
	return -ENOSYS;
}
static inline int close(int fd)
{
	return -ENOSYS;
}
static inline int lstatat(int dirfd, const char *filename, struct stat *s)
{
	return -ENOSYS;
}
static inline int statat(int dirfd, const char *filename, struct stat *s)
{
	return -ENOSYS;
}
static inline int fstat(int fd, struct stat *s)
{
	return -ENOSYS;
}
static inline ssize_t read(int fd, void *buf, size_t count)
{
	return -ENOSYS;
}
static inline ssize_t pread(int fd, void *buf, size_t count, loff_t offset)
{
	return -ENOSYS;
}
static inline ssize_t write(int fd, const void *buf, size_t count)
{
	return -ENOSYS;
}
static inline ssize_t pwrite(int fd, const void *buf, size_t count, loff_t offset)
{
	return -ENOSYS;
}
static inline loff_t lseek(int fildes, loff_t offset, int whence)
{
	return -ENOSYS;
}
static inline int symlink(const char *pathname, const char *newpath)
{
	return -ENOSYS;
}
static inline int readlinkat(int dirfd, const char *path, char *buf, size_t bufsiz)
{
	return -ENOSYS;
}
static inline int chdir(const char *pathname)
{
	return -ENOSYS;
}
static inline char *pushd(const char *dir)
{
	return NULL;
}
static inline int popd(char *dir)
{
	return -ENOSYS;
}
static inline const char *getcwd(void)
{
	return NULL;
}
static inline int ftruncate(int fd, loff_t length)
{
	return -ENOSYS;
}
#endif

static inline int unlink(const char *pathname)
{
	return unlinkat(AT_FDCWD, pathname, 0);
}

static inline int lstat(const char *filename, struct stat *s)
{
	return lstatat(AT_FDCWD, filename, s);
}

static inline int stat(const char *filename, struct stat *s)
{
	return statat(AT_FDCWD, filename, s);
}

static inline int rmdir(const char *pathname)
{
	return unlinkat(AT_FDCWD, pathname, AT_REMOVEDIR);
}

static inline int readlink(const char *path, char *buf, size_t bufsiz)
{
	return readlinkat(AT_FDCWD, path, buf, bufsiz);
}

#endif /* __UNISTD_H */
