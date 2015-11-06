#ifndef __ASM_ARM_SEMIHOSTING_H
#define __ASM_ARM_SEMIHOSTING_H

int semihosting_open(const char *fname, int flags);
int semihosting_close(int fd);
int semihosting_writec(char c);
int semihosting_write0(const char *str);
ssize_t semihosting_write(int fd, const void *buf, size_t count);
ssize_t semihosting_read(int fd, void *buf, size_t count);
int semihosting_readc(void);
int semihosting_isatty(int fd);
int semihosting_seek(int fd, loff_t pos);
int semihosting_flen(int fd);
int semihosting_remove(const char *fname);
int semihosting_rename(const char *fname1, const char *fname2);
int semihosting_errno(void);
int semihosting_system(const char *command);

#endif
