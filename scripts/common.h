#ifndef __COMMON_H
#define __COMMON_H

int read_file_2(const char *filename, size_t *size, void **outbuf, size_t max_size);
void *read_file(const char *filename, size_t *size);
int write_file(const char *filename, const void *buf, size_t size);
int read_full(int fd, void *buf, size_t size);
int write_full(int fd, void *buf, size_t size);

#endif /* __COMMON_H */
