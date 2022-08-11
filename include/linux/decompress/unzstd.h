/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_DECOMPRESS_UNZSTD_H
#define LINUX_DECOMPRESS_UNZSTD_H

int unzstd(unsigned char *inbuf, int len,
	   int (*fill)(void*, unsigned int),
	   int (*flush)(void*, unsigned int),
	   unsigned char *output,
	   int *pos,
	   void (*error_fn)(char *x));
#endif
