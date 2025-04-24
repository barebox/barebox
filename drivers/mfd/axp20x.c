// SPDX-License-Identifier: GPL-2.0-only
/*
 * MFD core driver for the X-Powers' Power Management ICs
 *
 * AXP20x typically comprises an adaptive USB-Compatible PWM charger, BUCK DC-DC
 * converters, LDOs, multiple 12-bit ADCs of voltage, current and temperature
 * as well as configurable GPIOs.
 *
 * This file contains the interface independent core functions.
 *
 * Copyright (C) 2014 Carlo Caione
 *
 * Author: Carlo Caione <carlo@caione.org>
 */

#include <common.h>
#include <linux/bitops.h>
#include <clock.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/axp20x.h>
#include <linux/mfd/core.h>
#include <module.h>
#include <of.h>
#include <of_device.h>
#include <linux/regmap.h>
#include <regulator.h>

#define AXP20X_OFF	BIT(7)

#define AXP806_REG_ADDR_EXT_ADDR_MASTER_MODE	0
#define AXP806_REG_ADDR_EXT_ADDR_SLAVE_MODE	BIT(4)

static const char * const axp20x_model_names[] = {
	"AXP152",
	"AXP202",
	"AXP209",
	"AXP221",
	"AXP223",
	"AXP288",
	"AXP313A",
	"AXP803",
	"AXP806",
	"AXP809",
	"AXP813",
};

static const struct regmap_config axp152_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= AXP152_PWM1_DUTY_CYCLE,
};

static const struct regmap_config axp20x_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= AXP20X_OCV(AXP20X_OCV_MAX),
};

static const struct regmap_config axp22x_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= AXP22X_BATLOW_THRES1,
};

static const struct regmap_config axp288_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= AXP288_FG_TUNE5,
};

static const struct regmap_config axp313a_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= AXP313A_POK_CONTROL,
};

static const struct regmap_config axp806_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= AXP806_REG_ADDR_EXT,
};

static const struct mfd_cell axp20x_cells[] = {
	{
		.name		= "axp20x-gpio",
		/* .of_compatible	= "x-powers,axp209-gpio", */
	}, {
		.name		= "axp20x-pek",
	}, {
		.name		= "axp20x-regulator",
	}, {
		.name		= "axp20x-adc",
		/* .of_compatible	= "x-powers,axp209-adc", */
	}, {
		.name		= "axp20x-battery-power-supply",
		/* .of_compatible	= "x-powers,axp209-battery-power-supply", */
	}, {
		.name		= "axp20x-ac-power-supply",
		/* .of_compatible	= "x-powers,axp202-ac-power-supply", */
	}, {
		.name		= "axp20x-usb-power-supply",
		/* .of_compatible	= "x-powers,axp202-usb-power-supply", */
	},
};

static const struct mfd_cell axp221_cells[] = {
	{
		.name		= "axp221-pek",
	}, {
		.name		= "axp20x-regulator",
	}, {
		.name		= "axp22x-adc",
		/* .of_compatible	= "x-powers,axp221-adc", */
	}, {
		.name		= "axp20x-ac-power-supply",
		/* .of_compatible	= "x-powers,axp221-ac-power-supply", */
	}, {
		.name		= "axp20x-battery-power-supply",
		/* .of_compatible	= "x-powers,axp221-battery-power-supply", */
	}, {
		.name		= "axp20x-usb-power-supply",
		/* .of_compatible	= "x-powers,axp221-usb-power-supply", */
	},
};

static const struct mfd_cell axp223_cells[] = {
	{
		.name		= "axp221-pek",
	}, {
		.name		= "axp22x-adc",
		/* .of_compatible	= "x-powers,axp221-adc", */
	}, {
		.name		= "axp20x-battery-power-supply",
		/* .of_compatible	= "x-powers,axp221-battery-power-supply", */
	}, {
		.name		= "axp20x-regulator",
	}, {
		.name		= "axp20x-ac-power-supply",
		/* .of_compatible	= "x-powers,axp221-ac-power-supply", */
	}, {
		.name		= "axp20x-usb-power-supply",
		/* .of_compatible	= "x-powers,axp223-usb-power-supply", */
	},
};

static const struct mfd_cell axp152_cells[] = {
	{
		.name		= "axp20x-pek",
	},
	{
		.name		= "axp20x-regulator",
	},
};

static const struct mfd_cell axp288_cells[] = {
	{
		.name		= "axp288_adc",
	}, {
		.name		= "axp288_extcon",
	}, {
		.name		= "axp288_charger",
	}, {
		.name		= "axp221-pek",
	}, {
		.name		= "axp288_pmic_acpi",
	},
};

static const struct mfd_cell axp313a_cells[] = {
	{
		.name		= "axp313a-regulator"
	},
};


static const struct mfd_cell axp803_cells[] = {
	{
		.name		= "axp221-pek",
	}, {
		.name		= "axp20x-gpio",
		/* .of_compatible	= "x-powers,axp813-gpio", */
	}, {
		.name		= "axp813-adc",
		/* .of_compatible	= "x-powers,axp813-adc", */
	}, {
		.name		= "axp20x-battery-power-supply",
		/* .of_compatible	= "x-powers,axp813-battery-power-supply", */
	}, {
		.name		= "axp20x-ac-power-supply",
		/* .of_compatible	= "x-powers,axp813-ac-power-supply", */
	}, {
		.name		= "axp20x-usb-power-supply",
		/* .of_compatible	= "x-powers,axp813-usb-power-supply", */
	},
	{	.name		= "axp20x-regulator" },
};

static const struct mfd_cell axp806_cells[] = {
	{
		.name		= "axp20x-regulator",
	},
};

static const struct mfd_cell axp809_cells[] = {
	{
		.name		= "axp221-pek",
	}, {
		.name		= "axp20x-regulator",
	},
};

static const struct mfd_cell axp813_cells[] = {
	{
		.name		= "axp221-pek",
	}, {
		.name		= "axp20x-regulator",
	}, {
		.name		= "axp20x-gpio",
		/* .of_compatible	= "x-powers,axp813-gpio", */
	}, {
		.name		= "axp813-adc",
		/* .of_compatible	= "x-powers,axp813-adc", */
	}, {
		.name		= "axp20x-battery-power-supply",
		/* .of_compatible	= "x-powers,axp813-battery-power-supply", */
	}, {
		.name		= "axp20x-ac-power-supply",
		/* .of_compatible	= "x-powers,axp813-ac-power-supply", */
	}, {
		.name		= "axp20x-usb-power-supply",
		/* .of_compatible	= "x-powers,axp813-usb-power-supply", */
	},
};

static void axp20x_power_off(struct poweroff_handler *handler,
			     unsigned long flags)
{
	struct axp20x_dev *axp20x = container_of(handler, struct axp20x_dev, poweroff);

	regmap_write(axp20x->regmap, AXP20X_OFF_CTRL, AXP20X_OFF);

	shutdown_barebox();

	/* Give capacitors etc. time to drain to avoid kernel panic msg. */
	mdelay(500);
	hang();
}

int axp20x_match_device(struct axp20x_dev *axp20x)
{
	struct device *dev = axp20x->dev;
	const struct of_device_id *of_id;

	of_id = of_match_device(dev->driver->of_compatible, dev);
	if (!of_id) {
		dev_err(dev, "Unable to match OF ID\n");
		return -ENODEV;
	}
	axp20x->variant = (long)of_id->data;

	switch (axp20x->variant) {
	case AXP152_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp152_cells);
		axp20x->cells = axp152_cells;
		axp20x->regmap_cfg = &axp152_regmap_config;
		break;
	case AXP202_ID:
	case AXP209_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp20x_cells);
		axp20x->cells = axp20x_cells;
		axp20x->regmap_cfg = &axp20x_regmap_config;
		break;
	case AXP221_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp221_cells);
		axp20x->cells = axp221_cells;
		axp20x->regmap_cfg = &axp22x_regmap_config;
		break;
	case AXP223_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp223_cells);
		axp20x->cells = axp223_cells;
		axp20x->regmap_cfg = &axp22x_regmap_config;
		break;
	case AXP288_ID:
		axp20x->cells = axp288_cells;
		axp20x->nr_cells = ARRAY_SIZE(axp288_cells);
		axp20x->regmap_cfg = &axp288_regmap_config;
		break;
	case AXP313A_ID:
		axp20x->cells = axp313a_cells;
		axp20x->nr_cells = ARRAY_SIZE(axp313a_cells);
		axp20x->regmap_cfg = &axp313a_regmap_config;
		break;
	case AXP803_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp803_cells);
		axp20x->cells = axp803_cells;
		axp20x->regmap_cfg = &axp288_regmap_config;
		break;
	case AXP806_ID:
		/*
		 * Don't register the power key part if in slave mode or
		 * if there is no interrupt line.
		 */
		axp20x->nr_cells = ARRAY_SIZE(axp806_cells);
		axp20x->cells = axp806_cells;
		axp20x->regmap_cfg = &axp806_regmap_config;
		break;
	case AXP809_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp809_cells);
		axp20x->cells = axp809_cells;
		axp20x->regmap_cfg = &axp22x_regmap_config;
		break;
	case AXP813_ID:
		axp20x->nr_cells = ARRAY_SIZE(axp813_cells);
		axp20x->cells = axp813_cells;
		axp20x->regmap_cfg = &axp288_regmap_config;
		break;
	default:
		dev_err(dev, "unsupported AXP20X ID %lu\n", axp20x->variant);
		return -EINVAL;
	}
	dev_info(dev, "AXP20x variant %s found\n",
		 axp20x_model_names[axp20x->variant]);

	return 0;
}
EXPORT_SYMBOL(axp20x_match_device);

int axp20x_device_probe(struct axp20x_dev *axp20x)
{
	int ret;

	/*
	 * The AXP806 supports either master/standalone or slave mode.
	 * Slave mode allows sharing the serial bus, even with multiple
	 * AXP806 which all have the same hardware address.
	 *
	 * This is done with extra "serial interface address extension",
	 * or AXP806_BUS_ADDR_EXT, and "register address extension", or
	 * AXP806_REG_ADDR_EXT, registers. The former is read-only, with
	 * 1 bit customizable at the factory, and 1 bit depending on the
	 * state of an external pin. The latter is writable. The device
	 * will only respond to operations to its other registers when
	 * the these device addressing bits (in the upper 4 bits of the
	 * registers) match.
	 *
	 * By default we support an AXP806 chained to an AXP809 in slave
	 * mode. Boards which use an AXP806 in master mode can set the
	 * property "x-powers,master-mode" to override the default.
	 */
	if (axp20x->variant == AXP806_ID) {
		if (of_property_read_bool(axp20x->dev->of_node,
					  "x-powers,master-mode") ||
		    of_property_read_bool(axp20x->dev->of_node,
					  "x-powers,self-working-mode"))
			regmap_write(axp20x->regmap, AXP806_REG_ADDR_EXT,
				     AXP806_REG_ADDR_EXT_ADDR_MASTER_MODE);
		else
			regmap_write(axp20x->regmap, AXP806_REG_ADDR_EXT,
				     AXP806_REG_ADDR_EXT_ADDR_SLAVE_MODE);
	}

	axp20x->dev->priv = axp20x;

	ret = mfd_add_devices(axp20x->dev, axp20x->cells, axp20x->nr_cells);
	if (ret)
		return dev_err_probe(axp20x->dev, ret, "failed to add MFD devices\n");


	axp20x->poweroff.name = "axp20x-poweroff";
	axp20x->poweroff.poweroff = axp20x_power_off;
	axp20x->poweroff.priority = 200;

	if (!(axp20x->variant == AXP288_ID) || (axp20x->variant == AXP313A_ID))
		poweroff_handler_register(&axp20x->poweroff);

	return 0;
}
EXPORT_SYMBOL(axp20x_device_probe);

MODULE_DESCRIPTION("PMIC MFD core driver for AXP20X");
MODULE_AUTHOR("Carlo Caione <carlo@caione.org>");
MODULE_LICENSE("GPL");
