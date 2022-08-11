/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __UNCOMPRESS_H
#define __UNCOMPRESS_H

int uncompress(unsigned char *inbuf, int len,
	   int(*fill)(void*, unsigned int),
	   int(*flush)(void*, unsigned int),
	   unsigned char *output,
	   int *pos,
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
