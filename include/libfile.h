#ifndef __LIBFILE_H
#define __LIBFILE_H

int write_full(int fd, void *buf, size_t size);
int read_full(int fd, void *buf, size_t size);

char *read_file_line(const char *fmt, ...);

void *read_file(const char *filename, size_t *size);

int read_file_2(const char *filename, size_t *size, void **outbuf,
		loff_t max_size);

int write_file(const char *filename, void *buf, size_t size);

int copy_file(const char *src, const char *dst, int verbose);

#endif /* __LIBFILE_H */
