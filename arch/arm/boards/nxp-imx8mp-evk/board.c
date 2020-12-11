// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Oleksij Rempel, Pengutronix
 */

#include <asm/memory.h>
#include <bootsource.h>
#include <common.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/sizes.h>
#include <mach/bbu.h>
#include <mach/iomux-mx8mp.h>
#include <gpio.h>
#include <envfs.h>

static int nxp_imx8mp_evk_init(void)
{
	int emmc_bbu_flag = 0;
	int emmc_sd_flag = 0;
	u32 val;

	if (!of_machine_is_compatible("fsl,imx8mp-evk"))
		return 0;

	if (bootsource_get() == BOOTSOURCE_MMC) {
		if (bootsource_get_instance() == 2) {
			of_device_enable_path("/chosen/environment-emmc");
			emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
		} else {
			of_device_enable_path("/chosen/environment-sd");
			emmc_sd_flag = BBU_HANDLER_FLAG_DEFAULT;
		}
	} else {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", emmc_sd_flag);
	imx8m_bbu_internal_mmc_register_handler("eMMC", "/dev/mmc2", emmc_bbu_flag);

	val = readl(MX8MP_IOMUXC_GPR_BASE_ADDR + MX8MP_IOMUXC_GPR1);
	val |= MX8MP_IOMUXC_GPR1_ENET1_RGMII_EN;
	writel(val, MX8MP_IOMUXC_GPR_BASE_ADDR + MX8MP_IOMUXC_GPR1);

	return 0;
}
coredevice_initcall(nxp_imx8mp_evk_init);
