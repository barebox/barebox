// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2011 Antony Pavlov <antonynpavlov@gmail.com>

/* This file is part of barebox. */

#include <common.h>
#include <init.h>
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
