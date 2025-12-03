// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: Copyright (c) 2025, Sohaib Mohamed <sohaib.amhmd@gmail.com>
#include <filetype.h>
#include <common.h>
#include <init.h>
#include <asm/memory.h>
#include <mach/stm32mp/bbu.h>
#include <bootsource.h>
#include <of.h>

static int dhcor_stm32mp1_probe(struct device *dev)
{
	int emmc_bbu_flag = 0;
	int sd_bbu_flag = 0;
	int nor_bbu_flag = 0;

	if (bootsource_get() == BOOTSOURCE_MMC) {
		if (bootsource_get_instance() == 2)
			emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
		else
			sd_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	} else if (bootsource_get() == BOOTSOURCE_NOR) {
		nor_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	} else {
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	stm32mp_bbu_mmc_fip_register("sd", "/dev/mmc0", sd_bbu_flag);
	stm32mp_bbu_mmc_fip_register("emmc", "/dev/mmc1", emmc_bbu_flag);
	stm32mp_bbu_nor_fip_register("nor", "/dev/m25p0", nor_bbu_flag);

	return 0;
}

static const struct of_device_id dhcor_stm32mp1_of_match[] = {
	{ .compatible = "dh,stm32mp157a-dhcor-som" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, dhcor_stm32mp1_of_match);
BAREBOX_DEEP_PROBE_ENABLE(dhcor_stm32mp1_of_match);

static struct driver dhcor_stm32mp1_driver = {
	.name = "dhcor_stm32mp1",
	.probe = dhcor_stm32mp1_probe,
	.of_compatible = dhcor_stm32mp1_of_match,
};
device_platform_driver(dhcor_stm32mp1_driver);
