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
 */

/* Dummy functions, currently not in Use */

#include <common.h>
#include <command.h>
#include <image.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <asm/cpu.h>
#include <asm/blackfin.h>
#include <errno.h>
#include <init.h>
#include <boot.h>
#include <bootm.h>

#define CMD_LINE_ADDR 0xFF900000  /* L1 scratchpad */

static int do_bootm_linux(struct image_data *idata)
{
	int (*appl)(char *cmdline);
	const char *cmdline = linux_bootargs_get();
	char *cmdlinedest = (char *) CMD_LINE_ADDR;
	int ret;

	ret = bootm_load_os(idata, idata->os_address);
	if (ret)
		return ret;

	appl = (void *)(idata->os_address + idata->os_entry);
	printf("Starting Kernel at 0x%p\n", appl);

	if (idata->dryrun)
		return 0;

	icache_disable();

	strncpy(cmdlinedest, cmdline, 0x1000);
	cmdlinedest[0xfff] = 0;

	*(volatile unsigned long *) IMASK = 0x1f;

	(*appl)(cmdlinedest);

	return -1;
}

static struct image_handler handler = {
	.name = "Blackfin Linux",
	.bootm = do_bootm_linux,
	.filetype = filetype_uimage,
	.ih_os = IH_OS_LINUX,
};

static int bfinlinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(bfinlinux_register_image_handler);

