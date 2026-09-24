/*
 * Copyright (C) 2010 Marco Stornelli <marco.stornelli@gmail.com>
 * Copyright (C) 2011 Kees Cook <keescook@chromium.org>
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LINUX_PSTORE_RAM_H__
#define __LINUX_PSTORE_RAM_H__

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>

struct persistent_ram_buffer;
struct rs_control;

struct persistent_ram_ecc_info {
	int block_size;
	int ecc_size;
	int symsize;
	int poly;
	uint16_t *par;
};

/*
 * Ramoops platform data
 * @mem_size	memory size for ramoops
 * @mem_address	physical memory address to contain ramoops
 */

struct ramoops_platform_data {
	unsigned long	mem_size;
	unsigned long	mem_address;
	unsigned int	mem_type;
	unsigned long	record_size;
	unsigned long	console_size;
	unsigned long	ftrace_size;
	unsigned long	pmsg_size;
	int		dump_oops;
	struct persistent_ram_ecc_info ecc_info;
};

#endif
