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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <asm/armlinux.h>
#include <sizes.h>
#include <usb/ehci.h>
#include <mach/iomap.h>

static int ac100_mem_init(void)
{
	arm_add_mem_device("ram0", 0x0, SZ_512M);

	return 0;
}
mem_initcall(ac100_mem_init);

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
