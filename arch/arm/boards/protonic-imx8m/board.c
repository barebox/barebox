// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2020 David Jander, Protonic Holland

#include <bootsource.h>
#include <common.h>
#include <envfs.h>
#include <environment.h>
#include <i2c/i2c.h>
#include <init.h>
#include <mach/imx/bbu.h>

static int prt_prt8mm_init_power(void)
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	int ret;
	char buf[2];

	client.addr = 0x60;
	adapter = i2c_get_adapter(1);
	if (!adapter) {
		printf("i2c bus not found\n");
		return -ENODEV;
	}
	client.adapter = adapter;

	buf[0] = 0xe3;
	ret = i2c_write_reg(&client, 0x00, buf, 1); // VSEL0 = 0.95V, force PWM
	if (ret < 0) {
		printf("i2c write error\n");
		return -ENODEV;
	}
	buf[0] = 0xe0;
	ret = i2c_write_reg(&client, 0x01, buf, 1); // VSEL1 = 0.92V, force PWM
	if (ret < 0) {
		printf("i2c write error\n");
		return -ENODEV;
	}
	return 0;
}

static int prt_prt8mm_probe(struct device *dev)
{
	int emmc_bbu_flag = 0;
	int sd_bbu_flag = 0;

	prt_prt8mm_init_power();

	barebox_set_hostname("prt8mm");

	if (bootsource_get() == BOOTSOURCE_MMC) {
		if (bootsource_get_instance() == 2) {
			of_device_enable_path("/chosen/environment-emmc");
			emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
		} else {
			of_device_enable_path("/chosen/environment-sd");
			sd_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
		}
	} else {
		of_device_enable_path("/chosen/environment-emmc");
		emmc_bbu_flag = BBU_HANDLER_FLAG_DEFAULT;
	}

	imx8m_bbu_internal_mmc_register_handler("SD", "/dev/mmc1.barebox",
						sd_bbu_flag);
	imx8m_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc2",
						    emmc_bbu_flag);

	defaultenv_append_directory(defaultenv_prt8m);

	return 0;
}

static const struct of_device_id prt_imx8mm_of_match[] = {
	{ .compatible = "prt,prt8mm", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, prt_imx8mm_of_match);

static struct driver prt_prt8mm_board_driver = {
	.name = "board-protonic-imx8mm",
	.probe = prt_prt8mm_probe,
	.of_compatible = DRV_OF_COMPAT(prt_imx8mm_of_match),
};
device_platform_driver(prt_prt8mm_board_driver);
