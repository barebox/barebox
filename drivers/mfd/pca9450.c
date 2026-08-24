// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023 Holger Assmann, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <init.h>
#include <linux/mfd/core.h>
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

static const struct mfd_cell pca9450_cells[] = {
	{ .name = "pca9450-regulator", },
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

	reset_source_set_device(dev, type, 200);

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
	struct pca9450 *pca9450;
	struct regmap *regmap;
	int reg;
	int ret;

	pca9450 = devm_kzalloc(dev, sizeof(*pca9450), GFP_KERNEL);
	if (!pca9450)
		return -ENOMEM;

	pca9450->dev = dev;
	pca9450->type = (enum pca9450_chip_type)(uintptr_t)device_get_match_data(dev);

	regmap = regmap_init_i2c(to_i2c_client(dev), &pca9450_regmap_i2c_config);
	if (IS_ERR(regmap)) {
		ret = PTR_ERR(regmap);
		goto free;
	}

	pca9450->regmap = regmap;

	ret = regmap_register_cdev(regmap, NULL);
	if (ret)
		goto free;

	ret = regmap_read(regmap, PCA9450_REG_DEV_ID, &reg);
	if (ret) {
		dev_err(dev, "Unable to read PMIC Chip ID\n");
		goto free;
	}

	/* Chip ID defined in bits [7:4] */
	dev_info(dev, "PMIC Chip ID: 0x%x\n", (reg >> 4));

	/* Enable WDOG_B, for cold reset by default */
	if (of_property_read_bool(dev->of_node, "nxp,wdog_b-warm-reset"))
		regmap_update_bits(regmap, PCA9450_RESET_CTRL,
				   PCA9450_PMIC_RESET_WDOG_B_CFG_MASK,
				   PCA9450_PMIC_RESET_WDOG_B_CFG_WARM);
	else
		regmap_update_bits(regmap, PCA9450_RESET_CTRL,
				   PCA9450_PMIC_RESET_WDOG_B_CFG_MASK,
				   PCA9450_PMIC_RESET_WDOG_B_CFG_COLD_LDO12);

	if (pca9450_init_callback)
		pca9450_init_callback(regmap);
	pca9450_map = regmap;

	dev->priv = pca9450;

	pca9450_get_reset_source(dev, regmap);

	ret = mfd_add_devices(dev, pca9450_cells, ARRAY_SIZE(pca9450_cells));
	if (ret)
		goto free;

	return 0;

free:
	free(pca9450);
	return ret;
}

static __maybe_unused struct of_device_id pca9450_dt_ids[] = {
	{ .compatible = "nxp,pca9450a", .data = (void *)PCA9450_TYPE_PCA9450A, },
	{ .compatible = "nxp,pca9450b", .data = (void *)PCA9450_TYPE_PCA9450BC, },
	{ .compatible = "nxp,pca9450c", .data = (void *)PCA9450_TYPE_PCA9450BC, },
	{ .compatible = "nxp,pca9451a", .data = (void *)PCA9450_TYPE_PCA9451A, },
	{ .compatible = "nxp,pca9452", .data = (void *)PCA9450_TYPE_PCA9452, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pca9450_dt_ids);

static struct driver pca9450_i2c_driver = {
	.name		= "pca9450-i2c",
	.probe		= pca9450_probe,
	.of_compatible	= DRV_OF_COMPAT(pca9450_dt_ids),
};

coredevice_i2c_driver(pca9450_i2c_driver);
