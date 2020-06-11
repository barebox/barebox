// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017 Sascha Hauer, Pengutronix

#include <common.h>
#include <init.h>
#include <mach/bbu.h>
#include <linux/phy.h>

#include <linux/micrel_phy.h>

static int ksz8081_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x1f, 0x8190);
	phy_write(dev, 0x16, 0x202);

	return 0;
}

static int nxp_imx6ull_evk_init(void)
{
	if (!of_machine_is_compatible("fsl,imx6ull-14x14-evk"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_KSZ8081, MICREL_PHY_ID_MASK,
			ksz8081_phy_fixup);

	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/mmc1.barebox",
			BBU_HANDLER_FLAG_DEFAULT);

	barebox_set_hostname("imx6ull-evk");

	return 0;
}
device_initcall(nxp_imx6ull_evk_init);
