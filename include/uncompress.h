/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __UNCOMPRESS_H
#define __UNCOMPRESS_H

int uncompress(unsigned char *inbuf, long len,
	   long(*fill)(void*, unsigned long),
	   long(*flush)(void*, unsigned long),
	   unsigned char *output,
	   long *pos,
	   void(*error_fn)(char *x));

int uncompress_fd_to_fd(int infd, int outfd,
	   void(*error_fn)(char *x));

int uncompress_fd_to_buf(int infd, void *output,
	   void(*error_fn)(char *x));

int uncompress_buf_to_fd(const void *input, size_t input_len,
			 int outfd, void(*error_fn)(char *x));

ssize_t uncompress_buf_to_buf(const void *input, size_t input_len,
			      void **buf, void(*error_fn)(char *x));

void uncompress_err_stdout(char *);

#endif /* __UNCOMPRESS_H */
