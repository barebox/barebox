// SPDX-License-Identifier: GPL-2.0-only
/*
 * MFD core driver for Ricoh RN5T618 PMIC
 * Note: Manufacturer is now Nisshinbo Micro Devices Inc.
 *
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 * Copyright (C) 2016 Toradex AG
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <init.h>
#include <of.h>
#include <linux/regmap.h>
#include <reset_source.h>
#include <restart.h>
#include <mfd/rn5t618.h>

struct rn5t618 {
	struct restart_handler restart;
	struct regmap *regmap;
};

static void rn5t618_restart(struct restart_handler *rst,
			    unsigned long flags)
{
	struct rn5t618 *rn5t618 = container_of(rst, struct rn5t618, restart);

	regmap_write(rn5t618->regmap, RN5T618_SLPCNT, RN5T618_SLPCNT_SWPPWROFF);
}

static int rn5t618_reset_reason_detect(struct device *dev,
				       struct regmap *regmap)
{
	enum reset_src_type type;
	unsigned int reg;
	int ret;

	ret = regmap_read(regmap, RN5T618_PONHIS, &reg);
	if (ret)
		return ret;

	dev_dbg(dev, "Power-on history: %x\n", reg);

	if (reg == 0) {
		dev_info(dev, "No power-on reason available\n");
		return 0;
	}

	if (reg & (RN5T618_PONHIS_ON_EXTINPON | RN5T618_PONHIS_ON_PWRONPON)) {
		type = RESET_POR;
	} else if (!(reg & RN5T618_PONHIS_ON_REPWRPON)) {
		return -EINVAL;
	} else {
		ret = regmap_read(regmap, RN5T618_POFFHIS, &reg);
		if (ret)
			return ret;

		dev_dbg(dev, "Power-off history: %x\n", reg);

		if (reg & RN5T618_POFFHIS_PWRONPOFF)
			type = RESET_POR;
		else if (reg & RN5T618_POFFHIS_TSHUTPOFF)
			type = RESET_THERM;
		else if (reg & RN5T618_POFFHIS_VINDETPOFF)
			type = RESET_BROWNOUT;
		else if (reg & RN5T618_POFFHIS_IODETPOFF)
			type = RESET_UKWN;
		else if (reg & RN5T618_POFFHIS_CPUPOFF)
			type = RESET_RST;
		else if (reg & RN5T618_POFFHIS_WDGPOFF)
			type = RESET_WDG;
		else if (reg & RN5T618_POFFHIS_DCLIMPOFF)
			type = RESET_BROWNOUT;
		else if (reg & RN5T618_POFFHIS_N_OEPOFF)
			type = RESET_EXT;
		else
			return -EINVAL;
	}

	reset_source_set_device(dev, type, 200);
	return 0;
}

static const struct regmap_config rn5t618_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= RN5T618_MAX_REG,
};

static int __init rn5t618_i2c_probe(struct device *dev)
{
	struct rn5t618 *pmic_instance;
	unsigned char reg[2];
	int ret;

	pmic_instance = xzalloc(sizeof(struct rn5t618));
	pmic_instance->regmap = regmap_init_i2c(to_i2c_client(dev), &rn5t618_regmap_config);
	if (IS_ERR(pmic_instance->regmap))
		return PTR_ERR(pmic_instance->regmap);

	ret = regmap_register_cdev(pmic_instance->regmap, NULL);
	if (ret)
		return ret;

	ret = regmap_bulk_read(pmic_instance->regmap, RN5T618_LSIVER, &reg, 2);
	if (ret) {
		dev_err(dev, "Failed to read PMIC version via I2C\n");
		return ret;
	}

	dev_dbg(dev, "Found NMD RN5T567/618 LSI %x, OTP: %x\n", reg[0], reg[1]);

	/* Settings used to trigger software reset and by a watchdog trigger */
	regmap_write(pmic_instance->regmap, RN5T618_REPCNT, RN5T618_REPCNT_OFF_RESETO_16MS |
		     RN5T618_REPCNT_OFF_REPWRTIM_1000MS | RN5T618_REPCNT_OFF_REPWRON);

	pmic_instance->restart.of_node = dev->of_node;
	pmic_instance->restart.name = "RN5T618";
	pmic_instance->restart.restart = &rn5t618_restart;
	restart_handler_register(&pmic_instance->restart);
	dev_dbg(dev, "RN5t: Restart handler with priority %d registered\n", pmic_instance->restart.priority);

	ret = rn5t618_reset_reason_detect(dev, pmic_instance->regmap);
	if (ret)
		dev_warn(dev, "Failed to query reset reason\n");

	return of_platform_populate(dev->of_node, NULL, dev);
}

static __maybe_unused const struct of_device_id rn5t618_of_match[] = {
	{ .compatible = "ricoh,rn5t567" },
	{ .compatible = "ricoh,rn5t618" },
	{ .compatible = "ricoh,rc5t619" },
	{ }
};
MODULE_DEVICE_TABLE(of, rn5t618_of_match);

static struct driver rn5t618_i2c_driver = {
	.name		= "rn5t618-i2c",
	.probe		= rn5t618_i2c_probe,
	.of_compatible	= DRV_OF_COMPAT(rn5t618_of_match),
};

coredevice_i2c_driver(rn5t618_i2c_driver);
