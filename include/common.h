/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __COMMON_H_
#define __COMMON_H_	1

#include <stdio.h>
#include <barebox.h>
#include <module.h>
#include <config.h>
#include <clock.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/pagemap.h>
#include <asm/common.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <barebox-info.h>

/*
 * Function Prototypes
 */
void reginfo(void);

char *size_human_readable(unsigned long long size);

int	readline	(const char *prompt, char *buf, int len);

/* common/memsize.c */
long	get_ram_size  (volatile long *, long);

int mem_parse_options(int argc, char *argv[], char *optstr, int *mode,
		char **sourcefile, char **destfile, int *swab);
int memcpy_parse_options(int argc, char *argv[], int *sourcefd,
			 int *destfd, loff_t *count,
			 int rwsize, int destmode);
#define RW_BUF_SIZE	(unsigned)4096

#endif	/* __COMMON_H_ */
