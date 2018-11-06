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

#include <asm/memory.h>
#include <bootsource.h>
#include <common.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/sizes.h>
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

static int nxp_imx8mq_evk_init(void)
{
	int flags;

	if (!of_machine_is_compatible("fsl,imx8mq-evk"))
		return 0;

	barebox_set_hostname("imx8mq-evk");

	flags = bootsource_get_instance() == 0 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	imx8mq_bbu_internal_mmc_register_handler("eMMC",
						 "/dev/mmc0.barebox", flags);

	flags = bootsource_get_instance() == 1 ? BBU_HANDLER_FLAG_DEFAULT : 0;
	imx8mq_bbu_internal_mmc_register_handler("SD",
						 "/dev/mmc1.barebox", flags);

	if (bootsource_get_instance() == 0)
		of_device_enable_path("/chosen/environment-emmc");
	else
		of_device_enable_path("/chosen/environment-sd");

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK,
				   ar8031_phy_fixup);
	return 0;
}
device_initcall(nxp_imx8mq_evk_init);
