// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Antony Pavlov <antonynpavlov@gmail.com>

/* This file is part of barebox. */

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
