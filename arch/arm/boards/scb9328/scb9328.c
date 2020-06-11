// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2004 Sascha Hauer, Synertronixx GmbH

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
	if (!of_machine_is_compatible("stx,scb9328"))
		return 0;

	/* CS3 becomes CS3 by clearing reset default bit 1 in FMCR */
	writel(0x1, MX1_SCM_BASE_ADDR + MX1_FMCR);

	armlinux_set_architecture(MACH_TYPE_SCB9328);

	return 0;
}

device_initcall(scb9328_devices_init);
