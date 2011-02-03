#ifndef __ASM_ARCH_LINUX_H
#define __ASM_ARCH_LINUX_H

int linux_register_device(const char *name, void *start, void *end);
int tap_alloc(char *dev);
uint64_t linux_get_time(void);
int linux_read(int fd, void *buf, size_t count);
int linux_read_nonblock(int fd, void *buf, size_t count);
ssize_t linux_write(int fd, const void *buf, size_t count);
off_t linux_lseek(int fildes, off_t offset);
int linux_tstc(int fd);

int barebox_register_console(char *name_template, int stdinfd, int stdoutfd);

struct linux_console_data {
	int stdinfd;
	int stdoutfd;
	unsigned int flags;
};

#endif /* __ASM_ARCH_LINUX_H */
