/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
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
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <usb/ehci.h>
#include <mach/hardware.h>

static int hostname_init(void)
{
	barebox_set_hostname("mb7707");

	return 0;
}
core_initcall(hostname_init);

static struct ehci_platform_data ehci_pdata = {
	.flags = 0,
};

static int mb7707_devices_init(void)
{
	add_usb_ehci_device(DEVICE_ID_DYNAMIC, UEMD_EHCI_BASE,
		UEMD_EHCI_BASE + 0x10, &ehci_pdata);

	return 0;
}
device_initcall(mb7707_devices_init);
