/*
 * barebox - bootm.c
 *
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
 *
 * (C) Copyright 2003, Psyent Corporation <www.psyent.com>
 * Scott McNutt <smcnutt@psyent.com>
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

#include <common.h>
#include <command.h>
#include <image.h>
#include <environment.h>
#include <init.h>
#include <boot.h>
#include <bootm.h>
#include <errno.h>
#include <asm/cache.h>

#define NIOS_MAGIC 0x534f494e /* enable command line and initrd passing */

static int do_bootm_linux(struct image_data *idata)
{
	void (*kernel)(int, int, int, const char *);
	const char *commandline = linux_bootargs_get();
	int ret;

	ret = bootm_load_os(idata, idata->os_address);
	if (ret)
		return ret;

	if (idata->dryrun)
		return 0;

	kernel = (void *)(idata->os_address + idata->os_entry);

	/* kernel parameters passing
	 * r4 : NIOS magic
	 * r5 : initrd start
	 * r6 : initrd end or fdt
	 * r7 : kernel command line
	 * fdt is passed to kernel via r6, the same as initrd_end. fdt will be
	 * verified with fdt magic. when both initrd and fdt are used at the
	 * same time, fdt must follow immediately after initrd.
	 */

	/* flushes data and instruction caches before calling the kernel */
	flush_cache_all();

	kernel(NIOS_MAGIC, 0, 0, commandline);
	/* does not return */

	return 1;
}

static struct image_handler handler = {
	.name = "NIOS2 Linux",
	.bootm = do_bootm_linux,
	.filetype = filetype_uimage,
	.ih_os = IH_OS_LINUX,
};

int nios2_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(nios2_register_image_handler);

