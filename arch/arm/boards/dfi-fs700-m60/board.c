/*
 * Copyright (C) 2013 Sascha Hauer, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 *
 */
#define pr_fmt(fmt)  "dfi-fs700-m60: " fmt

#include <generated/mach-types.h>
#include <common.h>
#include <sizes.h>
#include <envfs.h>
#include <init.h>
#include <gpio.h>
#include <net.h>
#include <linux/phy.h>

#include <asm/armlinux.h>
#include <asm/memory.h>
#include <asm/mmu.h>
#include <asm/io.h>

#include <mach/imx6-regs.h>
#include <mach/generic.h>
#include <mach/bbu.h>

static int ar8031_phy_fixup(struct phy_device *dev)
{
	u16 val;

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	phy_write(dev, 0xd, 0x7);
	phy_write(dev, 0xe, 0x8016);
	phy_write(dev, 0xd, 0x4007);

	val = phy_read(dev, 0xe);
	val &= 0xffe3;
	val |= 0x18;
	phy_write(dev, 0xe, val);

	/* introduce tx clock delay */
	phy_write(dev, 0x1d, 0x5);
	val = phy_read(dev, 0x1e);
	val |= 0x0100;
	phy_write(dev, 0x1e, val);

	return 0;
}

#define PHY_ID_AR8031	0x004dd074
#define AR_PHY_ID_MASK	0xffffffff

static int dfi_fs700_m60_init(void)
{
	if (!of_machine_is_compatible("dfi,fs700-m60"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK, ar8031_phy_fixup);

	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/mmc3.boot0",
		BBU_HANDLER_FLAG_DEFAULT, NULL, 0, 0);

	armlinux_set_bootparams((void *)0x10000100);
	armlinux_set_architecture(MACH_TYPE_MX6Q_SABRESD);

	return 0;
}
device_initcall(dfi_fs700_m60_init);
