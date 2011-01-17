/*
 * barebox - blackfin_linux.c
 *
 * Copyright (c) 2005 blackfin.uclinux.org
 *
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* Dummy functions, currently not in Use */

#include <common.h>
#include <command.h>
#include <image.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <asm/cpu.h>
#include <asm/blackfin.h>
#include <init.h>
#include <boot.h>

#define CMD_LINE_ADDR 0xFF900000  /* L1 scratchpad */

static int do_bootm_linux(struct image_data *idata)
{
	int (*appl)(char *cmdline);
	const char *cmdline = getenv("bootargs");
	char *cmdlinedest = (char *) CMD_LINE_ADDR;
	struct image_handle *os_handle = idata->os;
	image_header_t *os_header = &os_handle->header;

	appl = (int (*)(char *))image_get_ep(os_header);
	printf("Starting Kernel at 0x%p\n", appl);

	if (relocate_image(os_handle, (void *)image_get_load(os_header)))
		return -1;

	icache_disable();

	strncpy(cmdlinedest, cmdline, 0x1000);
	cmdlinedest[0xfff] = 0;

	*(volatile unsigned long *) IMASK = 0x1f;

	(*appl)(cmdlinedest);

	return -1;
}

static struct image_handler handler = {
	.bootm = do_bootm_linux,
	.image_type = IH_OS_LINUX,
};

static int bfinlinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(bfinlinux_register_image_handler);

