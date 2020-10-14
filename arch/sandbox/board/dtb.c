/*
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2015 Marc Kleine-Budde <mkl@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <of.h>

#include <mach/linux.h>
#include <linux/err.h>

static const void *dtb;

int barebox_register_dtb(const void *new_dtb)
{
	if (dtb)
		return -EBUSY;

	dtb = new_dtb;

	return 0;
}

extern char __dtb_sandbox_start[];

static int of_sandbox_init(void)
{
	if (!dtb)
		dtb = __dtb_sandbox_start;

	barebox_register_fdt(dtb);

	return 0;
}
core_initcall(of_sandbox_init);
