// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "TQMa93xx: " fmt

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <i2c/i2c.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <environment.h>
#include <mfd/pca9450.h>
#include <deep-probe.h>
#include <mach/imx/bbu.h>

static void tqma93xx_init_pmic(struct regmap *map)
{
	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	regmap_write(map, PCA9450_BUCK123_DVS, 0x29);
	/* enable DVS control through PMIC_STBY_REQ */
	regmap_write(map, PCA9450_BUCK1CTRL, 0x59);
	/* 0.9 V */
	regmap_write(map, PCA9450_BUCK1OUT_DVS0, 0x18);
	regmap_write(map, PCA9450_BUCK3OUT_DVS0, 0x18);
	/* set standby voltage to 0.65v */
	regmap_write(map, PCA9450_BUCK1OUT_DVS1, 0x4);

	/* I2C_LT_EN*/
	regmap_write(map, 0xa, 0x3);

	/* set WDOG_B_CFG to cold reset */
	regmap_write(map, PCA9450_RESET_CTRL, 0xA1);
}

static int tqma93xx_probe(struct device *dev)
{
	pca9450_register_init_callback(tqma93xx_init_pmic);

	imx9_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0", 0);

	return 0;
}

static const struct of_device_id tqma93xx_of_match[] = {
        {
                .compatible = "tq,imx93-tqma9352",
        },
        { /* sentinel */ },
};

static struct driver tqma93xx_board_driver = {
        .name = "board-tqma93xx",
        .probe = tqma93xx_probe,
        .of_compatible = tqma93xx_of_match,
};
coredevice_platform_driver(tqma93xx_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(tqma93xx_of_match);
