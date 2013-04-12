/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <asm/armlinux.h>
#include <sizes.h>
#include <usb/ehci.h>
#include <mach/iomap.h>

static struct ehci_platform_data ehci_pdata = {
	.flags = EHCI_HAS_TT,
};

static int ac100_devices_init(void)
{
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, TEGRA_USB3_BASE,
			&ehci_pdata);

	return 0;
}
device_initcall(ac100_devices_init);
