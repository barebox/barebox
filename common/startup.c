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

/**
 * @file
 * @brief Main entry into the C part of barebox
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
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>

extern initcall_t __barebox_initcalls_start[], __barebox_early_initcalls_end[],
		  __barebox_initcalls_end[];

static void display_meminfo(void)
{
	ulong mstart = mem_malloc_start();
	ulong mend   = mem_malloc_end();
	ulong msize  = mend - mstart + 1;

	debug("barebox code: 0x%p -> 0x%p\n", _stext, _etext);
	debug("bss segment:  0x%p -> 0x%p\n", __bss_start, __bss_stop);
	printf("Malloc space: 0x%08lx -> 0x%08lx (size %s)\n",
		mstart, mend, size_human_readable(msize));
#ifdef CONFIG_ARM
	printf("Stack space : 0x%08x -> 0x%08x (size %s)\n",
		STACK_BASE, STACK_BASE + STACK_SIZE,
		size_human_readable(STACK_SIZE));
#endif
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
}

#endif /* CONFIG_HAS_EARLY_INIT */

#ifdef CONFIG_DEFAULT_ENVIRONMENT
#include <generated/barebox_default_env.h>

static struct memory_platform_data default_env_platform_data = {
	.name = "defaultenv",
};

static struct device_d default_env_dev = {
	.id		= -1,
	.name		= "mem",
	.platform_data	= &default_env_platform_data,
};

static int register_default_env(void)
{
	default_env_dev.map_base = (unsigned long)default_environment;
	default_env_dev.size = sizeof(default_environment);
	register_device(&default_env_dev);
	return 0;
}

device_initcall(register_default_env);
#endif

#if defined CONFIG_FS_RAMFS && defined CONFIG_FS_DEVFS
static int mount_root(void)
{
	mount("none", "ramfs", "/");
	mkdir("/dev", 0);
	mount("none", "devfs", "/dev");
	return 0;
}
fs_initcall(mount_root);
#endif

void start_barebox (void)
{
	initcall_t *initcall;
	int result;
#ifdef CONFIG_COMMAND_SUPPORT
	struct stat s;
#endif

#ifdef CONFIG_HAS_EARLY_INIT
	/* We are running from RAM now, copy early initdata from
	 * early RAM to RAM
	 */
	memcpy(&__early_init_data_begin, init_data_ptr,
			(ulong)&__early_init_data_end -
			(ulong)&__early_init_data_begin);
	init_data_ptr = &__early_init_data_begin;
#endif /* CONFIG_HAS_EARLY_INIT */

	for (initcall = __barebox_initcalls_start;
			initcall < __barebox_initcalls_end; initcall++) {
		PUTHEX_LL(*initcall);
		PUTC_LL('\n');
		result = (*initcall)();
		if (result)
			hang();
	}

	display_meminfo();

#ifdef CONFIG_ENV_HANDLING
	if (envfs_load(default_environment_path, "/env")) {
#ifdef CONFIG_DEFAULT_ENVIRONMENT
		printf("no valid environment found on %s. "
			"Using default environment\n",
			default_environment_path);
		envfs_load("/dev/defaultenv", "/env");
#endif
	}
#endif
#ifdef CONFIG_COMMAND_SUPPORT
	printf("running /env/bin/init...\n");

	if (!stat("/env/bin/init", &s)) {
		run_command("source /env/bin/init", 0);
	} else {
		printf("not found\n");
	}
#endif
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;)
		run_shell();

	/* NOTREACHED - no way out of command loop except booting */
}

void __noreturn hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}

/* Everything needed to cleanly shutdown barebox.
 * Should be called before starting an OS to get
 * the devices into a clean state
 */
void shutdown_barebox(void)
{
	devices_shutdown();
#ifdef ARCH_SHUTDOWN
	arch_shutdown();
#endif
}

