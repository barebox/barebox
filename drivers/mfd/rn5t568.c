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
#include <regmap.h>
#include <reset_source.h>
#include <restart.h>

#define RN5T568_LSIVER 0x00
#define RN5T568_OTPVER 0x01
#define RN5T568_PONHIS 0x09
# define RN5T568_PONHIS_ON_EXTINPON BIT(3)
# define RN5T568_PONHIS_ON_REPWRPON BIT(1)
# define RN5T568_PONHIS_ON_PWRONPON BIT(0)
#define RN5T568_POFFHIS 0x0a
# define RN5T568_POFFHIS_N_OEPOFF BIT(7)
# define RN5T568_POFFHIS_DCLIMPOFF BIT(6)
# define RN5T568_POFFHIS_WDGPOFF BIT(5)
# define RN5T568_POFFHIS_CPUPOFF BIT(4)
# define RN5T568_POFFHIS_IODETPOFF BIT(3)
# define RN5T568_POFFHIS_VINDETPOFF BIT(2)
# define RN5T568_POFFHIS_TSHUTPOFF BIT(1)
# define RN5T568_POFFHIS_PWRONPOFF BIT(0)
#define RN5T568_SLPCNT 0x0e
# define RN5T568_SLPCNT_SWPPWROFF BIT(0)
#define RN5T568_REPCNT 0x0f
# define RN5T568_REPCNT_OFF_RESETO_16MS 0x30
# define RN5T568_REPCNT_OFF_REPWRTIM_1000MS 0x06
# define RN5T568_REPCNT_OFF_REPWRON BIT(0)
#define RN5T568_MAX_REG 0xbc

struct rn5t568 {
	struct restart_handler restart;
	struct regmap *regmap;
};

static void rn5t568_restart(struct restart_handler *rst)
{
	struct rn5t568 *rn5t568 = container_of(rst, struct rn5t568, restart);

	regmap_write(rn5t568->regmap, RN5T568_SLPCNT, RN5T568_SLPCNT_SWPPWROFF);
}

static int rn5t568_reset_reason_detect(struct device *dev,
				       struct regmap *regmap)
{
	unsigned int reg;
	int ret;

	ret = regmap_read(regmap, RN5T568_PONHIS, &reg);
	if (ret)
		return ret;

	dev_dbg(dev, "Power-on history: %x\n", reg);

	if (reg == 0) {
		dev_info(dev, "No power-on reason available\n");
		return 0;
	}

	if (reg & RN5T568_PONHIS_ON_EXTINPON) {
		reset_source_set_device(dev, RESET_POR);
		return 0;
	} else if (reg & RN5T568_PONHIS_ON_PWRONPON) {
		reset_source_set_device(dev, RESET_POR);
		return 0;
	} else if (!(reg & RN5T568_PONHIS_ON_REPWRPON))
		return -EINVAL;

	ret = regmap_read(regmap, RN5T568_POFFHIS, &reg);
	if (ret)
		return ret;

	dev_dbg(dev, "Power-off history: %x\n", reg);

	if (reg & RN5T568_POFFHIS_PWRONPOFF)
		reset_source_set_device(dev, RESET_POR);
	else if (reg & RN5T568_POFFHIS_TSHUTPOFF)
		reset_source_set_device(dev, RESET_THERM);
	else if (reg & RN5T568_POFFHIS_VINDETPOFF)
		reset_source_set_device(dev, RESET_BROWNOUT);
	else if (reg & RN5T568_POFFHIS_IODETPOFF)
		reset_source_set_device(dev, RESET_UKWN);
	else if (reg & RN5T568_POFFHIS_CPUPOFF)
		reset_source_set_device(dev, RESET_RST);
	else if (reg & RN5T568_POFFHIS_WDGPOFF)
		reset_source_set_device(dev, RESET_WDG);
	else if (reg & RN5T568_POFFHIS_DCLIMPOFF)
		reset_source_set_device(dev, RESET_BROWNOUT);
	else if (reg & RN5T568_POFFHIS_N_OEPOFF)
		reset_source_set_device(dev, RESET_EXT);
	else
		return -EINVAL;

	return 0;
}

static const struct regmap_config rn5t568_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= RN5T568_MAX_REG,
};

static int __init rn5t568_i2c_probe(struct device *dev)
{
	struct rn5t568 *pmic_instance;
	unsigned char reg[2];
	int ret;

	pmic_instance = xzalloc(sizeof(struct rn5t568));
	pmic_instance->regmap = regmap_init_i2c(to_i2c_client(dev), &rn5t568_regmap_config);
	if (IS_ERR(pmic_instance->regmap))
		return PTR_ERR(pmic_instance->regmap);

	ret = regmap_register_cdev(pmic_instance->regmap, NULL);
	if (ret)
		return ret;

	ret = regmap_bulk_read(pmic_instance->regmap, RN5T568_LSIVER, &reg, 2);
	if (ret) {
		dev_err(dev, "Failed to read PMIC version via I2C\n");
		return ret;
	}

	dev_info(dev, "Found NMD RN5T568 LSI %x, OTP: %x\n", reg[0], reg[1]);

	/* Settings used to trigger software reset and by a watchdog trigger */
	regmap_write(pmic_instance->regmap, RN5T568_REPCNT, RN5T568_REPCNT_OFF_RESETO_16MS |
		     RN5T568_REPCNT_OFF_REPWRTIM_1000MS | RN5T568_REPCNT_OFF_REPWRON);

	pmic_instance->restart.of_node = dev->of_node;
	pmic_instance->restart.name = "RN5T568";
	pmic_instance->restart.restart = &rn5t568_restart;
	restart_handler_register(&pmic_instance->restart);
	dev_dbg(dev, "RN5t: Restart handler with priority %d registered\n", pmic_instance->restart.priority);

	ret = rn5t568_reset_reason_detect(dev, pmic_instance->regmap);
	if (ret)
		dev_warn(dev, "Failed to query reset reason\n");

	return of_platform_populate(dev->of_node, NULL, dev);
}

static __maybe_unused const struct of_device_id rn5t568_of_match[] = {
	{ .compatible = "ricoh,rn5t568", .data = NULL, },
	{ }
};
MODULE_DEVICE_TABLE(of, rn5t568_of_match);

static struct driver rn5t568_i2c_driver = {
	.name		= "rn5t568-i2c",
	.probe		= rn5t568_i2c_probe,
	.of_compatible	= DRV_OF_COMPAT(rn5t568_of_match),
};

coredevice_i2c_driver(rn5t568_i2c_driver);
