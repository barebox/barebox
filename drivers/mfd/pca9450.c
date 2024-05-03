// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023 Holger Assmann, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <init.h>
#include <mfd/pca9450.h>
#include <of.h>
#include <regmap.h>
#include <reset_source.h>

#define REASON_PMIC_RST		0x10
#define REASON_SW_RST		0x20
#define REASON_WDOG		0x40
#define REASON_PWON		0x80

static const struct regmap_config pca9450_regmap_i2c_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x2E,
};

static int pca9450_get_reset_source(struct device *dev, struct regmap *map)
{
	enum reset_src_type type;
	int reg;
	int ret;

	ret = regmap_read(map, PCA9450_PWRON_STAT, &reg);
	if (ret)
		return ret;

	switch (reg) {
	case REASON_PWON:
		dev_dbg(dev, "Power ON triggered by PMIC_ON_REQ.\n");
		type = RESET_POR;
		break;
	case REASON_WDOG:
		dev_dbg(dev, "Detected cold reset by WDOGB pin\n");
		type = RESET_WDG;
		break;
	case REASON_SW_RST:
		dev_dbg(dev, "Detected cold reset by SW_RST\n");
		type = RESET_RST;
		break;
	case REASON_PMIC_RST:
		dev_dbg(dev, "Detected cold reset by PMIC_RST_B\n");
		type = RESET_EXT;
		break;
	default:
		dev_warn(dev, "Unknown reset reason: 0x%02x\n", reg);
		fallthrough;
	case 0:
		type = RESET_UKWN;
	}

	reset_source_set_device(dev, type);

	return 0;
};

static struct regmap *pca9450_map;

static void (*pca9450_init_callback)(struct regmap *map);

int pca9450_register_init_callback(void(*callback)(struct regmap *map))
{
	if (pca9450_init_callback)
		return -EBUSY;

	pca9450_init_callback = callback;

	if (pca9450_map)
		pca9450_init_callback(pca9450_map);

	return 0;
}

static int __init pca9450_probe(struct device *dev)
{
	struct regmap *regmap;
	int reg;
	int ret;

	regmap = regmap_init_i2c(to_i2c_client(dev), &pca9450_regmap_i2c_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	ret = regmap_register_cdev(regmap, NULL);
	if (ret)
		return ret;

	ret = regmap_read(regmap, PCA9450_REG_DEV_ID, &reg);
	if (ret) {
		dev_err(dev, "Unable to read PMIC Chip ID\n");
		return ret;
	}

	/* Chip ID defined in bits [7:4] */
	dev_info(dev, "PMIC Chip ID: 0x%x\n", (reg >> 4));

	if (pca9450_init_callback)
		pca9450_init_callback(regmap);
	pca9450_map = regmap;

	pca9450_get_reset_source(dev,regmap);

	return of_platform_populate(dev->of_node, NULL, dev);
}

static __maybe_unused struct of_device_id pca9450_dt_ids[] = {
	{ .compatible = "nxp,pca9450a" },
	{ .compatible = "nxp,pca9450c" },
	{ .compatible = "nxp,pca9451a" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pca9450_dt_ids);

static struct driver pca9450_i2c_driver = {
	.name		= "pca9450-i2c",
	.probe		= pca9450_probe,
	.of_compatible	= DRV_OF_COMPAT(pca9450_dt_ids),
};

coredevice_i2c_driver(pca9450_i2c_driver);
