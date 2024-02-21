/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef GUNZIP_H
#define GUNZIP_H

int gunzip(unsigned char *inbuf, long len,
	   long(*fill)(void*, unsigned long),
	   long(*flush)(void*, unsigned long),
	   unsigned char *output,
	   long *pos,
	   void(*error_fn)(char *x));
#endif
