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
 */

#ifdef CONFIG_DEBUG_INITCALLS
#define DEBUG
#endif

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
#include <envfs.h>
#include <asm/sections.h>
#include <uncompress.h>

extern initcall_t __barebox_initcalls_start[], __barebox_early_initcalls_end[],
		  __barebox_initcalls_end[];

#ifdef CONFIG_DEFAULT_ENVIRONMENT
#include "barebox_default_env.h"

static int register_default_env(void)
{
	int ret;
	void *defaultenv;

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_COMPRESSED)) {
		void *tmp = malloc(default_environment_size);

		if (!tmp)
			return -ENOMEM;

		memcpy(tmp, default_environment, default_environment_size);

		defaultenv = xzalloc(default_environment_uncompress_size);

		ret = uncompress(tmp, default_environment_size,
				NULL, NULL,
				defaultenv, NULL, uncompress_err_stdout);

		free(tmp);

		if (ret) {
			free(defaultenv);
			return ret;
		}
	} else {
		defaultenv = (void *)default_environment;
	}


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

int (*barebox_main)(void);

void __noreturn start_barebox(void)
{
	initcall_t *initcall;
	int result;
	struct stat s;

	if (!IS_ENABLED(CONFIG_SHELL_NONE))
		barebox_main = run_shell;

	for (initcall = __barebox_initcalls_start;
			initcall < __barebox_initcalls_end; initcall++) {
		pr_debug("initcall-> %pS\n", *initcall);
		result = (*initcall)();
		if (result)
			pr_err("initcall %pS failed: %s\n", *initcall,
					strerror(-result));
	}

	pr_debug("initcalls done\n");

	if (IS_ENABLED(CONFIG_ENV_HANDLING)) {
		int ret;

		ret = envfs_load(default_environment_path, "/env", 0);

		if (ret && IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT)) {
			pr_err("no valid environment found on %s. "
				"Using default environment\n",
				default_environment_path);
			envfs_load("/dev/defaultenv", "/env", 0);
		}
	}

	if (IS_ENABLED(CONFIG_COMMAND_SUPPORT)) {
		pr_info("running /env/bin/init...\n");

		if (!stat("/env/bin/init", &s)) {
			run_command("source /env/bin/init", 0);
		} else {
			pr_err("/env/bin/init not found\n");
			if (IS_ENABLED(CONFIG_CMD_LOGIN))
				while(run_command("login -t 0", 0));
		}
	}

	if (!barebox_main) {
		pr_err("No main function! aborting.\n");
		hang();
	}

	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;)
		barebox_main();

	/* NOTREACHED - no way out of command loop except booting */
}

void __noreturn hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}

void (*board_shutdown)(void);

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
	if (board_shutdown)
		board_shutdown();
}
