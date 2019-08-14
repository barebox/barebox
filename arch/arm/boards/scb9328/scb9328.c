/*
 * Copyright (C) 2004 Sascha Hauer, Synertronixx GmbH
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
 *
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <generated/mach-types.h>
#include <mach/imx1-regs.h>
#include <asm/armlinux.h>
#include <mach/weim.h>
#include <io.h>
#include <partition.h>
#include <fs.h>
#include <envfs.h>
#include <mach/iomux-mx1.h>
#include <mach/devices-imx1.h>

static int scb9328_devices_init(void)
{
	/* CS3 becomes CS3 by clearing reset default bit 1 in FMCR */
	writel(0x1, MX1_SCM_BASE_ADDR + MX1_FMCR);

	armlinux_set_architecture(MACH_TYPE_SCB9328);

	return 0;
}

device_initcall(scb9328_devices_init);
