// SPDX-License-Identifier: GPL-2.0
//
// FAN53555 Fairchild Digitally Programmable TinyBuck Regulator Driver.
//
// Based on fan53555.c linux kernel driver:
// Copyright (c) 2012 Marvell Technology Ltd.
// Yunfan Zhang <yfzhang@marvell.com>

#include <common.h>
#include <driver.h>
#include <of_device.h>
#include <regulator.h>
#include <i2c/i2c.h>
#include <linux/regmap.h>

#define FAN53555_VSEL0		(0x00)
#define FAN53555_VSEL1		(0x01)
# define VSEL_MODE		BIT(6)
# define VSEL_BUCK_EN		BIT(7)
#define FAN53555_CONTROL	(0x02)
# define CTL_MODE_VSEL0_MODE	BIT(0)
# define CTL_MODE_VSEL1_MODE	BIT(1)
#define FAN53555_ID1		(0x03)
# define DIE_ID			(0x0f)
#define FAN53555_ID2		(0x04)
# define DIE_REV		(0x0f)

#define RK8602_VSEL0		(0x06)
#define RK8602_VSEL1		(0x07)

#define TCS4525_VSEL1		(0x10)
#define TCS4525_VSEL0		(0x11)
#define TCS4525_COMMAND		(0x14)
# define TCS_VSEL1_MODE		BIT(6)
# define TCS_VSEL0_MODE		BIT(7)

#define FAN53555_NVOLTAGES	64
#define FAN53526_NVOLTAGES	128
#define RK8602_NVOLTAGES	160

enum {
	FAN53555_VSEL_ID_0 = 0,
	FAN53555_VSEL_ID_1,
};

enum fan53555_vendor {
	FAN53526_VENDOR_FAIRCHILD = 0,
	FAN53555_VENDOR_FAIRCHILD,
	FAN53555_VENDOR_ROCKCHIP,	/* RK8600, RK8601 */
	RK8602_VENDOR_ROCKCHIP,		/* RK8602, RK8603 */
	FAN53555_VENDOR_SILERGY,
	FAN53526_VENDOR_TCS,
};

enum {
	FAN53526_CHIP_ID_01 = 1,
};

enum {
	FAN53526_CHIP_REV_08 = 8,
};

enum {
	FAN53555_CHIP_ID_00 = 0,
	FAN53555_CHIP_ID_01,
	FAN53555_CHIP_ID_02,
	FAN53555_CHIP_ID_03,
	FAN53555_CHIP_ID_04,
	FAN53555_CHIP_ID_05,
	FAN53555_CHIP_ID_08 = 8,
};

enum {
	RK8600_CHIP_ID_08 = 8,		/* RK8600, RK8601 */
};

enum {
	RK8602_CHIP_ID_10 = 10,		/* RK8602, RK8603 */
};

enum {
	TCS4525_CHIP_ID_12 = 12,
};

enum {
	TCS4526_CHIP_ID_00 = 0,
};

enum {
	FAN53555_CHIP_REV_00 = 0x3,
	FAN53555_CHIP_REV_13 = 0xf,
};

enum {
	SILERGY_SYR82X = 8,
	SILERGY_SYR83X = 9,
};

struct fan53555_device_info {
	enum fan53555_vendor vendor;
	struct device *dev;
	struct regulator_dev rdev;
	struct regulator_desc rdesc;
	int chip_id;
	int chip_rev;
	unsigned int sleep_vsel_id;
};

static int fan53526_voltages_setup_fairchild(struct fan53555_device_info *di)
{
	switch (di->chip_id) {
	case FAN53526_CHIP_ID_01:
		switch (di->chip_rev) {
		case FAN53526_CHIP_REV_08:
			di->rdesc.min_uV = 600000;
			di->rdesc.uV_step = 6250;
			di->rdesc.n_voltages = FAN53526_NVOLTAGES;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int fan53555_voltages_setup_fairchild(struct fan53555_device_info *di)
{
	switch (di->chip_id) {
	case FAN53555_CHIP_ID_00:
		switch (di->chip_rev) {
		case FAN53555_CHIP_REV_00:
			di->rdesc.min_uV = 600000;
			di->rdesc.uV_step = 10000;
			break;
		case FAN53555_CHIP_REV_13:
			di->rdesc.min_uV = 800000;
			di->rdesc.uV_step = 10000;
			break;
		default:
			return -EINVAL;
		}
		break;
	case FAN53555_CHIP_ID_01:
	case FAN53555_CHIP_ID_03:
	case FAN53555_CHIP_ID_05:
	case FAN53555_CHIP_ID_08:
		di->rdesc.min_uV = 600000;
		di->rdesc.uV_step = 10000;
		break;
	case FAN53555_CHIP_ID_04:
		di->rdesc.min_uV = 603000;
		di->rdesc.uV_step = 12826;
		break;
	default:
		return -EINVAL;
	}

	di->rdesc.n_voltages = FAN53555_NVOLTAGES;

	return 0;
}

static int fan53555_voltages_setup_rockchip(struct fan53555_device_info *di)
{
	switch (di->chip_id) {
	case RK8600_CHIP_ID_08:
		di->rdesc.min_uV = 712500;
		di->rdesc.uV_step = 12500;
		di->rdesc.n_voltages = FAN53555_NVOLTAGES;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rk8602_voltages_setup_rockchip(struct fan53555_device_info *di)
{
	switch (di->chip_id) {
	case RK8602_CHIP_ID_10:
		di->rdesc.min_uV = 500000;
		di->rdesc.uV_step = 6250;
		di->rdesc.n_voltages = RK8602_NVOLTAGES;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int fan53555_voltages_setup_silergy(struct fan53555_device_info *di)
{
	switch (di->chip_id) {
	case SILERGY_SYR82X:
	case SILERGY_SYR83X:
		di->rdesc.min_uV = 712500;
		di->rdesc.uV_step = 12500;
		di->rdesc.n_voltages = FAN53555_NVOLTAGES;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int fan53526_voltages_setup_tcs(struct fan53555_device_info *di)
{
	switch (di->chip_id) {
	case TCS4525_CHIP_ID_12:
	case TCS4526_CHIP_ID_00:
		di->rdesc.min_uV = 600000;
		di->rdesc.uV_step = 6250;
		di->rdesc.n_voltages = FAN53526_NVOLTAGES;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct regulator_ops fan53555_regulator_ops = {
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.map_voltage = regulator_map_voltage_linear,
	.list_voltage = regulator_list_voltage_linear,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
};

static int fan53555_device_setup(struct fan53555_device_info *di)
{
	unsigned int mode_reg, mode_mask;
	int ret = 0;

	/* Setup voltage control register */
	switch (di->vendor) {
	case FAN53526_VENDOR_FAIRCHILD:
	case FAN53555_VENDOR_FAIRCHILD:
	case FAN53555_VENDOR_ROCKCHIP:
	case FAN53555_VENDOR_SILERGY:
		switch (di->sleep_vsel_id) {
		case FAN53555_VSEL_ID_0:
			di->rdesc.vsel_reg = FAN53555_VSEL1;
			break;
		case FAN53555_VSEL_ID_1:
			di->rdesc.vsel_reg = FAN53555_VSEL0;
			break;
		}
		di->rdesc.enable_reg = di->rdesc.vsel_reg;
		break;
	case RK8602_VENDOR_ROCKCHIP:
		switch (di->sleep_vsel_id) {
		case FAN53555_VSEL_ID_0:
			di->rdesc.vsel_reg = RK8602_VSEL1;
			di->rdesc.enable_reg = FAN53555_VSEL1;
			break;
		case FAN53555_VSEL_ID_1:
			di->rdesc.vsel_reg = RK8602_VSEL0;
			di->rdesc.enable_reg = FAN53555_VSEL0;
			break;
		}
		break;
	case FAN53526_VENDOR_TCS:
		switch (di->sleep_vsel_id) {
		case FAN53555_VSEL_ID_0:
			di->rdesc.vsel_reg = TCS4525_VSEL1;
			break;
		case FAN53555_VSEL_ID_1:
			di->rdesc.vsel_reg = TCS4525_VSEL0;
			break;
		}
		di->rdesc.enable_reg = di->rdesc.vsel_reg;
		break;
	default:
		dev_err(di->dev, "Vendor %d not supported!\n", di->vendor);
		return -EINVAL;
	}

	/* Setup mode control register */
	switch (di->vendor) {
	case FAN53526_VENDOR_FAIRCHILD:
		mode_reg = FAN53555_CONTROL;

		switch (di->sleep_vsel_id) {
		case FAN53555_VSEL_ID_0:
			mode_mask = CTL_MODE_VSEL1_MODE;
			break;
		case FAN53555_VSEL_ID_1:
			mode_mask = CTL_MODE_VSEL0_MODE;
			break;
		}
		break;
	case FAN53555_VENDOR_FAIRCHILD:
	case FAN53555_VENDOR_ROCKCHIP:
	case FAN53555_VENDOR_SILERGY:
		mode_reg = di->rdesc.vsel_reg;
		mode_mask = VSEL_MODE;
		break;
	case RK8602_VENDOR_ROCKCHIP:
		mode_mask = VSEL_MODE;

		switch (di->sleep_vsel_id) {
		case FAN53555_VSEL_ID_0:
			mode_reg = FAN53555_VSEL1;
			break;
		case FAN53555_VSEL_ID_1:
			mode_reg = FAN53555_VSEL0;
			break;
		default:
			return -EINVAL;
		}
		break;
	case FAN53526_VENDOR_TCS:
		mode_reg = TCS4525_COMMAND;

		switch (di->sleep_vsel_id) {
		case FAN53555_VSEL_ID_0:
			mode_mask = TCS_VSEL1_MODE;
			break;
		case FAN53555_VSEL_ID_1:
			mode_mask = TCS_VSEL0_MODE;
			break;
		}
		break;
	}

	/* Setup voltage range */
	switch (di->vendor) {
	case FAN53526_VENDOR_FAIRCHILD:
		ret = fan53526_voltages_setup_fairchild(di);
		break;
	case FAN53555_VENDOR_FAIRCHILD:
		ret = fan53555_voltages_setup_fairchild(di);
		break;
	case FAN53555_VENDOR_ROCKCHIP:
		ret = fan53555_voltages_setup_rockchip(di);
		break;
	case RK8602_VENDOR_ROCKCHIP:
		ret = rk8602_voltages_setup_rockchip(di);
		break;
	case FAN53555_VENDOR_SILERGY:
		ret = fan53555_voltages_setup_silergy(di);
		break;
	case FAN53526_VENDOR_TCS:
		ret = fan53526_voltages_setup_tcs(di);
		break;
	}
	if (!ret) {
		di->rdesc.supply_name = "vin";
		di->rdesc.ops = &fan53555_regulator_ops;
		di->rdesc.vsel_mask = BIT(fls(di->rdesc.n_voltages - 1)) - 1;
		di->rdesc.enable_mask = VSEL_BUCK_EN;

		/* REGULATOR_MODE_NORMAL */
		regmap_update_bits(di->rdev.regmap, mode_reg, mode_mask, 0);
	} else
		dev_err(di->dev, "Chip ID %d with rev %d not supported!\n",
			di->chip_id, di->chip_rev);

	return ret;
}

static const struct regmap_config fan53555_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int fan53555_regulator_probe(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct device_node *np = client->dev.of_node;
	struct fan53555_device_info *di;
	unsigned int val;
	int ret;

	di = xzalloc(sizeof(*di));

	di->vendor = (enum fan53555_vendor)device_get_match_data(dev);

	di->rdev.desc = &di->rdesc;
	di->rdev.dev = dev;

	di->rdev.regmap = regmap_init_i2c(client, &fan53555_regmap_config);
	if (IS_ERR(di->rdev.regmap)) {
		ret = PTR_ERR(di->rdev.regmap);
		goto err;
	}

	di->dev = &client->dev;

	/* Get chip ID */
	ret = regmap_read(di->rdev.regmap, FAN53555_ID1, &val);
	if (ret < 0) {
		dev_err(di->dev, "Failed to get chip ID!\n");
		goto err;
	}

	di->chip_id = val & DIE_ID;

	/* Get chip revision */
	ret = regmap_read(di->rdev.regmap, FAN53555_ID2, &val);
	if (ret < 0) {
		dev_err(di->dev, "Failed to get chip Rev!\n");
		goto err;
	}

	di->chip_rev = val & DIE_REV;

	ret = of_property_read_u32(np, "fcs,suspend-voltage-selector", &val);
	if (!ret) {
		if (val > 1) {
			dev_err(di->dev, "Invalid VSEL ID=%u!\n", val);
			ret = -EINVAL;
			goto err;
		}

		di->sleep_vsel_id = val;
	}

	ret = fan53555_device_setup(di);
	if (ret < 0)
		goto err;

	ret = of_regulator_register(&di->rdev, np);
	if (ret) {
		dev_err(di->dev, "Failed to register regulator!\n");
		goto err;
	}

	dev_info(di->dev, "FAN53555 Option[%d] Rev[%d] Detected!\n",
		 di->chip_id, di->chip_rev);

	return 0;

err:
	free(di);

	return ret;
}

static const struct of_device_id __maybe_unused fan53555_dt_ids[] = {
	{
		.compatible = "fcs,fan53526",
		.data = (void *)FAN53526_VENDOR_FAIRCHILD,
	}, {
		.compatible = "fcs,fan53555",
		.data = (void *)FAN53555_VENDOR_FAIRCHILD
	}, {
		.compatible = "rockchip,rk8600",
		.data = (void *)FAN53555_VENDOR_ROCKCHIP
	}, {
		.compatible = "rockchip,rk8602",
		.data = (void *)RK8602_VENDOR_ROCKCHIP
	}, {
		.compatible = "silergy,syr827",
		.data = (void *)FAN53555_VENDOR_SILERGY,
	}, {
		.compatible = "silergy,syr828",
		.data = (void *)FAN53555_VENDOR_SILERGY,
	}, {
		.compatible = "tcs,tcs4525",
		.data = (void *)FAN53526_VENDOR_TCS
	}, {
		.compatible = "tcs,tcs4526",
		.data = (void *)FAN53526_VENDOR_TCS
	},
	{ }
};
MODULE_DEVICE_TABLE(of, fan53555_dt_ids);

static struct driver fan53555_regulator_driver = {
	.name		= "fan53555-regulator",
	.probe		= fan53555_regulator_probe,
	.of_compatible	= fan53555_dt_ids,
};
device_i2c_driver(fan53555_regulator_driver);
