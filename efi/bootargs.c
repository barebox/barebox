// SPDX-License-Identifier: GPL-2.0-only
/*
 * bootargs.c - load options for EFI applications started via bootm
 *
 * Copyright (C) 2026 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */
#include <common.h>
#include <init.h>
#include <malloc.h>
#include <string.h>
#include <magicvar.h>
#include <globalvar.h>
#include <bootargs.h>
#include <efi/bootargs.h>

/**
 * efi_bootargs_get() - compute load options for an EFI application
 * @linux_image: whether the image is an EFI-stubbed Linux kernel
 *
 * All variables beginning with "global.efi.bootargs." are concatenated in
 * lexicographical order. For Linux kernels, the Linux bootargs are appended,
 * so the EFI stub sees them as the kernel command line.
 *
 * Return: newly allocated string to be freed by the caller or NULL
 */
char *efi_bootargs_get(bool linux_image)
{
	char *efi_args, *options = NULL;
	char *args[2];
	int nargs = 0;

	efi_args = globalvar_get_match("efi.bootargs.", " ");
	if (!isempty(efi_args))
		args[nargs++] = efi_args;

	if (linux_image) {
		const char *linux_args = linux_bootargs_get();

		if (!isempty(linux_args))
			args[nargs++] = (char *)linux_args;
	}

	if (nargs)
		options = strjoin(" ", args, nargs);

	free(efi_args);

	return options;
}

static int efi_bootargs_init(void)
{
	globalvar_add_simple("efi.bootargs.base", NULL);

	return 0;
}
late_initcall(efi_bootargs_init);

BAREBOX_MAGICVAR(global.efi.bootargs.*,
		 "Load options of EFI applications started via bootm. Linux bootargs are appended for EFI-stubbed kernels");
BAREBOX_MAGICVAR(global.efi.bootargs.dyn.*,
		 "Load options set by boot entries. Cleared after each boot entry");
