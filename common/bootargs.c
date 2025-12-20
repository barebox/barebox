// SPDX-License-Identifier: GPL-2.0-only
/*
 * bootargs.c - concatenate Linux bootargs
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <boot.h>
#include <malloc.h>
#include <magicvar.h>
#include <globalvar.h>
#include <environment.h>

static char *linux_bootargs;
static int linux_bootargs_overwritten;

/*
 * This returns the Linux bootargs
 *
 * There are two ways to handle bootargs. The old legacy way is to use the
 * 'bootargs' environment variable. The new and more flexible way is to use
 * global variables beginning with "global.linux.bootargs." and
 * "global.linux.mtdparts.". These variables will be concatenated together to
 * the resulting bootargs. If there are no "global.linux.bootargs." variables
 * we fall back to "bootargs"
 */
const char *linux_bootargs_get(void)
{
	char *bootargs, *parts;

	if (linux_bootargs_overwritten)
		return linux_bootargs;

	free(linux_bootargs);

	bootargs = globalvar_get_match("linux.bootargs.", " ");
	if (!strlen(bootargs)) {
		free(bootargs);
		return getenv("bootargs");
	}

	linux_bootargs = bootargs;

	parts = globalvar_get_match("linux.mtdparts.", ";");
	if (strlen(parts)) {
		bootargs = basprintf("%s mtdparts=%s", linux_bootargs,
				       parts);
		free(linux_bootargs);
		linux_bootargs = bootargs;
	}

	free(parts);

	parts = globalvar_get_match("linux.blkdevparts.", ";");
	if (strlen(parts)) {
		bootargs = basprintf("%s blkdevparts=%s", linux_bootargs,
				       parts);
		free(linux_bootargs);
		linux_bootargs = bootargs;
	}

	free(parts);

	return linux_bootargs;
}

int linux_bootargs_overwrite(const char *bootargs)
{
	if (bootargs) {
		free(linux_bootargs);
		linux_bootargs = xstrdup(bootargs);
		linux_bootargs_overwritten = 1;
	} else {
		linux_bootargs_overwritten = 0;
	}

	return 0;
}

char *linux_bootargs_append_rootwait(char *rootarg_old)
{
	char *rootarg = xasprintf("%s rootwait=%d",
				  rootarg_old, linux_rootwait_secs);
	free(rootarg_old);
	return rootarg;
}


BAREBOX_MAGICVAR(global.linux.bootargs.*, "Linux bootargs variables");
BAREBOX_MAGICVAR(global.linux.mtdparts.*, "Linux mtdparts variables");
BAREBOX_MAGICVAR(global.linux.blkdevparts.*, "Linux blkdevparts variables");
#ifdef CONFIG_ROOTWAIT_BOOTARG
BAREBOX_MAGICVAR(global.linux.rootwait, "Argument (in seconds) for Linux rootwait= option if it's fixed up. A negative value waits indefinitely");
#endif
