// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Michael Kopfensteiner, VAHLE Automation GmbH
 */

#include <asm/memory.h>
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/sizes.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iomux-mx8mp.h>
#include <gpio.h>
#include <envfs.h>

#define PHY_ID_ADIN1300		0x0283bc30
#define PHY_ID_MODEL_MASK	0xfffffff0

/*
 * This fixup is necessary to properly configure the ADIN1300
 * PHY on the SOM to properly communicate using RGMII.
 * This fixup disables the PHY's internal 2ns RGMII receive clock
 * delay. Without this configuration change, the system will
 * be able to send Ethernet packages, but the MAC won't receive
 * any response packages.
 *
 * This fixup is specific to the ADIN1300 PHY. This implementation
 * was ported from Variscite's U-Boot sources.
 */
static int phy_fixup_adin1300(struct phy_device *dev) {
	int ret;

	pr_debug("BOARD: applying PHY fixup for ADIN1300\n");

	ret = mdiobus_write(dev->bus, dev->addr, 0x0010, 0xFF23);
	if (ret) {
		pr_warn("ADIN1300 PHY fixup: failed to write EXT_REG_PTR\n");
		return ret;
	}

	ret = mdiobus_write(dev->bus, dev->addr, 0x0011, 0x0E01);
	if (ret)  {
		pr_warn("ADIN1300 PHY fixup: failed to write EXT_REG_DATA\n");
		return ret;
	}

	return 0;
}

static int var_imx8mp_dart_cb_probe(struct device *dev)
{
	int emmc_bbu_flag = 0;
	int sd_bbu_flag = 0;

	phy_register_fixup_for_uid(PHY_ID_ADIN1300, PHY_ID_MODEL_MASK, phy_fixup_adin1300);

	if (bootsource_get() == BOOTSOURCE_MMC && bootsource_get_instance() == 1) {
		of_device_enable_path("/chosen/environment-sd");
		sd_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", sd_bbu_flag);
	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2", emmc_bbu_flag);

	return 0;
}

static const struct of_device_id var_imx8mp_dart_cb_of_match[] = {
	{ .compatible = "variscite,imx8mp-var-dart" },
	{ /* Sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(var_imx8mp_dart_cb_of_match);

static struct driver var_imx8mp_dart_cb_board_driver = {
	.name = "board-var-imx8mp-dart-cb",
	.probe = var_imx8mp_dart_cb_probe,
	.of_compatible = var_imx8mp_dart_cb_of_match,
};
coredevice_platform_driver(var_imx8mp_dart_cb_board_driver);
