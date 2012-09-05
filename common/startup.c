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
#include <debug_ll.h>
#include <fs.h>
#include <linux/stat.h>
#include <environment.h>
#include <asm/sections.h>

extern initcall_t __barebox_initcalls_start[], __barebox_early_initcalls_end[],
		  __barebox_initcalls_end[];

#ifdef CONFIG_DEFAULT_ENVIRONMENT
#include <generated/barebox_default_env.h>

#ifdef CONFIG_DEFAULT_ENVIRONMENT_COMPRESSED
#include <uncompress.h>
void *defaultenv;
#else
#define defaultenv default_environment
#endif

static int register_default_env(void)
{
#ifdef CONFIG_DEFAULT_ENVIRONMENT_COMPRESSED
	int ret;
	void *tmp;

	tmp = xzalloc(default_environment_size);
	memcpy(tmp, default_environment, default_environment_size);

	defaultenv = xzalloc(default_environment_uncompress_size);

	ret = uncompress(tmp, default_environment_size, NULL, NULL,
			 defaultenv, NULL, uncompress_err_stdout);

	free(tmp);

	if (ret)
		return ret;
#endif

	add_mem_device("defaultenv", (unsigned long)defaultenv,
		       default_environment_uncompress_size,
		       IORESOURCE_MEM_WRITEABLE);
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

	for (initcall = __barebox_initcalls_start;
			initcall < __barebox_initcalls_end; initcall++) {
		debug("initcall-> %pS\n", *initcall);
		result = (*initcall)();
		debug("initcall<- %pS (%d)\n", *initcall, result);
	}

	debug("initcalls done\n");

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

