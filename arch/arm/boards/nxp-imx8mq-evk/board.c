/*
 * Copyright (C) 2018 Sascha Hauer, Pengutronix
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

#include <common.h>
#include <init.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <linux/phy.h>
#include <mach/bbu.h>

#include <envfs.h>

#define PHY_ID_AR8031	0x004dd074
#define AR_PHY_ID_MASK	0xffffffff

static int ar8031_phy_fixup(struct phy_device *phydev)
{
	/*
	 * Enable 1.8V(SEL_1P5_1P8_POS_REG) on
	 * Phy control debug reg 0
	 */
	phy_write(phydev, 0x1d, 0x1f);
	phy_write(phydev, 0x1e, 0x8);

	/* rgmii tx clock delay enable */
	phy_write(phydev, 0x1d, 0x05);
	phy_write(phydev, 0x1e, 0x100);

	return 0;
}

static int imx8mq_evk_mem_init(void)
{
	if (!of_machine_is_compatible("fsl,imx8mq-evk"))
		return 0;

	request_sdram_region("ATF", 0x40000000, SZ_128K);

	return 0;
}
mem_initcall(imx8mq_evk_mem_init);

static int nxp_imx8mq_evk_init(void)
{
	if (!of_machine_is_compatible("fsl,imx8mq-evk"))
		return 0;

	barebox_set_hostname("imx8mq-evk");

	imx8mq_bbu_internal_mmc_register_handler("eMMC", "/dev/mmc0", 0);

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK,
				   ar8031_phy_fixup);
	return 0;
}
device_initcall(nxp_imx8mq_evk_init);
