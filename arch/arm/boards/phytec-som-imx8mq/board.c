// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Christian Hemp
 */

#include <asm/memory.h>
#include <bootsource.h>
#include <environment.h>
#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <mach/imx/bbu.h>
#include <mfd/pfuze.h>
#include <linux/regmap.h>

#include <envfs.h>

#define PFUZE100_DEVICEID	0x0
#define PFUZE100_REVID		0x3

#define PFUZE100_SW1ABMODE	0x23
#define PFUZE100_SW2MODE	0x38
#define PFUZE100_SW1CMODE	0x31
#define PFUZE100_SW3AVOL	0x3c

#define APS_PFM			0xc

static void imx8mq_setup_pmic_voltages(struct regmap *map)
{
	int offset = PFUZE100_SW1CMODE;
	int switch_num = 6;
	int val, i;

	regmap_read(map, PFUZE100_SW3AVOL, &val);

	/* ensure the correct VDD_DRAM_0V9 output voltage */
	regmap_write_bits(map, PFUZE100_SW3AVOL, 0x3f, 0x18);

	/* pfuze200 */
	regmap_read(map, PFUZE100_DEVICEID, &val);
	if (val & 0xf) {
		offset = PFUZE100_SW2MODE;
		switch_num = 4;
	}

	/* set all switches APS in normal and PFM mode in standby */
	regmap_write(map, PFUZE100_SW1ABMODE, APS_PFM);

	for (i = 0; i < switch_num - 1; i++)
		regmap_write(map, offset + i * 7, APS_PFM);
}

static int physom_imx8mq_devices_init(void)
{
	int flag_emmc = 0;
	int flag_sd = 0;

	if (!of_machine_is_compatible("phytec,imx8mq-pcl066"))
		return 0;

	barebox_set_hostname("phycore-imx8mq");

	pfuze_register_init_callback(imx8mq_setup_pmic_voltages);

	switch (bootsource_get_instance()) {
	case 0:
		flag_emmc = BBU_HANDLER_FLAG_DEFAULT;
		of_device_enable_path("/chosen/environment-emmc");
		break;
	case 1:
	default:
		flag_sd = BBU_HANDLER_FLAG_DEFAULT;
		of_device_enable_path("/chosen/environment-sd");
		break;
	}

	imx8m_bbu_internal_mmc_register_handler("eMMC", "/dev/mmc0.barebox", flag_emmc);
	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox", flag_sd);


	return 0;
}
device_initcall(physom_imx8mq_devices_init);
