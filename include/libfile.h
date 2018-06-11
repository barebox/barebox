#ifndef __LIBFILE_H
#define __LIBFILE_H

int write_full(int fd, const void *buf, size_t size);
int read_full(int fd, void *buf, size_t size);

char *read_file_line(const char *fmt, ...);

void *read_file(const char *filename, size_t *size);

int read_file_2(const char *filename, size_t *size, void **outbuf,
		loff_t max_size);

int write_file(const char *filename, const void *buf, size_t size);
int write_file_flash(const char *filename, const void *buf, size_t size);

int copy_file(const char *src, const char *dst, int verbose);

int copy_recursive(const char *src, const char *dst);

int compare_file(const char *f1, const char *f2);

int open_and_lseek(const char *filename, int mode, loff_t pos);

/* Create a directory and its parents */
int make_directory(const char *pathname);

int unlink_recursive(const char *path, char **failedpath);

char *make_temp(const char *template);

int cache_file(const char *path, char **newpath);

#endif /* __LIBFILE_H */
