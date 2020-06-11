// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017 Zodiac Inflight Innovation

/* Author: Andrey Smirnov <andrew.smirnov@gmail.com> */

#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/imx7-regs.h>
#include <linux/phy.h>
#include <mfd/imx7-iomuxc-gpr.h>

#include <phy-id-list.h>

static int bcm54220_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x1e, 0x21);
	phy_write(dev, 0x1f, 0x7ea8);
	phy_write(dev, 0x1e, 0x2f);
	phy_write(dev, 0x1f, 0x71b7);

	return 0;
}

static void mx7_sabresd_init_fec1(void)
{
	void __iomem *gpr = IOMEM(MX7_IOMUXC_GPR_BASE_ADDR);
	uint32_t gpr1;

	gpr1 = readl(gpr + IOMUXC_GPR1);
	gpr1 &= ~(IMX7D_GPR1_ENET1_TX_CLK_SEL_MASK |
		  IMX7D_GPR1_ENET1_CLK_DIR_MASK);
	writel(gpr1, gpr + IOMUXC_GPR1);
}

static int mx7_sabresd_coredevices_init(void)
{
	if (!of_machine_is_compatible("fsl,imx7d-sdb"))
		return 0;

	mx7_sabresd_init_fec1();

	phy_register_fixup_for_uid(PHY_ID_BCM54220, 0xffffffff,
				   bcm54220_phy_fixup);

	return 0;
}
coredevice_initcall(mx7_sabresd_coredevices_init);
