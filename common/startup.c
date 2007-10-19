/*
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
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

#include <common.h>
#include <init.h>
#include <command.h>
#include <malloc.h>
#include <mem_malloc.h>
#include <debug_ll.h>
#include <fs.h>
#include <linux/stat.h>
#include <environment.h>
#include <reloc.h>

extern initcall_t __u_boot_initcalls_start[], __u_boot_early_initcalls_end[],
		  __u_boot_initcalls_end[];

static void display_meminfo(void)
{
	ulong mstart = mem_malloc_start();
	ulong mend   = mem_malloc_end();
	ulong msize  = mend - mstart + 1;

	debug ("U-Boot code: 0x%08lX -> 0x%08lX  BSS: -> 0x%08lX\n",
	       _u_boot_start, _bss_start, _bss_end);
	printf("Malloc Space: 0x%08lx -> 0x%08lx (size %s)\n",
		mstart, mend, size_human_readable(msize));
}

#ifdef CONFIG_HAS_EARLY_INIT

#define EARLY_INITDATA (CFG_INIT_RAM_ADDR + CFG_INIT_RAM_SIZE \
		- CONFIG_EARLY_INITDATA_SIZE)

void *init_data_ptr = (void *)EARLY_INITDATA;

void early_init (void)
{
	/* copy the early initdata segment to early init RAM */
	memcpy((void *)EARLY_INITDATA, RELOC(&__early_init_data_begin),
				(ulong)&__early_init_data_end -
				(ulong)&__early_init_data_begin);
	early_console_start(RELOC("psc3"), 115200);
}

#endif /* CONFIG_HAS_EARLY_INIT */

#ifdef CONFIG_DEFAULT_ENVIRONMENT
#include <uboot_default_env.h>

static struct device_d default_env_dev = {
	.name     = "rom",
	.id       = "defaultenv",
};

static void register_default_env(void)
{
	default_env_dev.map_base = (unsigned long)default_environment;
	default_env_dev.size = sizeof(default_environment);
	register_device(&default_env_dev);
}
#else
static void register_default_env(void)
{
}
#endif

void start_uboot (void)
{
	initcall_t *initcall;
	int result;
	struct stat s;

#ifdef CONFIG_HAS_EARLY_INIT
	/* We are running from RAM now, copy early initdata from
	 * early RAM to RAM
	 */
	memcpy(&__early_init_data_begin, init_data_ptr,
			(ulong)&__early_init_data_end -
			(ulong)&__early_init_data_begin);
	init_data_ptr = &__early_init_data_begin;
#endif /* CONFIG_HAS_EARLY_INIT */

	for (initcall = __u_boot_initcalls_start;
			initcall < __u_boot_initcalls_end; initcall++) {
		PUTHEX_LL(*initcall);
		PUTC_LL('\n');
		result = (*initcall)();
		if (result)
			hang();
	}

	display_meminfo();

	register_default_env();

	mount("none", "ramfs", "/");
	mkdir("/dev", 0);
	mount("none", "devfs", "/dev");

#ifdef CONFIG_CMD_ENVIRONMENT
	if (envfs_load("/dev/env0", "/env")) {
#ifdef CONFIG_DEFAULT_ENVIRONMENT
		printf("no valid environment found on /dev/env0. "
			"Using default environment\n");
		envfs_load("/dev/defaultenv", "/env");
#endif
	}
#endif
	printf("running /env/bin/init...\n");

	if (!stat("/env/bin/init", &s)) {
		run_command("source /env/bin/init", 0);
	} else {
		printf("not found\n");
	}

	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;)
		run_shell();

	/* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
