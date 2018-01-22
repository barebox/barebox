#ifndef __UNISTD_H
#define __UNISTD_H

#include <linux/types.h>

struct stat;

int unlink(const char *pathname);
int close(int fd);
int lstat(const char *filename, struct stat *s);
int stat(const char *filename, struct stat *s);
int fstat(int fd, struct stat *s);
ssize_t read(int fd, void *buf, size_t count);
ssize_t pread(int fd, void *buf, size_t count, loff_t offset);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t pwrite(int fd, const void *buf, size_t count, loff_t offset);
loff_t lseek(int fildes, loff_t offset, int whence);
int rmdir (const char *pathname);
int symlink(const char *pathname, const char *newpath);
int readlink(const char *path, char *buf, size_t bufsiz);
int chdir(const char *pathname);
const char *getcwd(void);
int ftruncate(int fd, loff_t length);

#endif /* __UNISTD_H */
