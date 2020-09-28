// SPDX-License-Identifier: GPL-2.0
// Copyright (C) STMicroelectronics 2018
// Author: Pascal Paillet <p.paillet@st.com> for STMicroelectronics.

#include <common.h>
#include <init.h>
#include <of_device.h>
#include <regmap.h>
#include <linux/regulator/of_regulator.h>
#include <regulator.h>
#include <linux/mfd/stpmic1.h>

#include <dt-bindings/mfd/st,stpmic1.h>

/**
 * stpmic1 regulator description: this structure is used as driver data
 * @desc: regulator framework description
 * @mask_reset_reg: mask reset register address
 * @mask_reset_mask: mask rank and mask reset register mask
 * @icc_reg: icc register address
 * @icc_mask: icc register mask
 */
struct stpmic1_regulator_cfg {
	struct regulator_dev rdev;
	struct regulator_desc desc;
	u8 mask_reset_reg;
	u8 mask_reset_mask;
	u8 icc_reg;
	u8 icc_mask;
};

enum {
	STPMIC1_BUCK1 = 0,
	STPMIC1_BUCK2 = 1,
	STPMIC1_BUCK3 = 2,
	STPMIC1_BUCK4 = 3,
	STPMIC1_LDO1 = 4,
	STPMIC1_LDO2 = 5,
	STPMIC1_LDO3 = 6,
	STPMIC1_LDO4 = 7,
	STPMIC1_LDO5 = 8,
	STPMIC1_LDO6 = 9,
	STPMIC1_VREF_DDR = 10,
	STPMIC1_BOOST = 11,
	STPMIC1_VBUS_OTG = 12,
	STPMIC1_SW_OUT = 13,
};

/* Enable time worst case is 5000mV/(2250uV/uS) */
#define PMIC_ENABLE_TIME_US 2200

static const struct regulator_linear_range buck1_ranges[] = {
	REGULATOR_LINEAR_RANGE(725000, 0, 4, 0),
	REGULATOR_LINEAR_RANGE(725000, 5, 36, 25000),
	REGULATOR_LINEAR_RANGE(1500000, 37, 63, 0),
};

static const struct regulator_linear_range buck2_ranges[] = {
	REGULATOR_LINEAR_RANGE(1000000, 0, 17, 0),
	REGULATOR_LINEAR_RANGE(1050000, 18, 19, 0),
	REGULATOR_LINEAR_RANGE(1100000, 20, 21, 0),
	REGULATOR_LINEAR_RANGE(1150000, 22, 23, 0),
	REGULATOR_LINEAR_RANGE(1200000, 24, 25, 0),
	REGULATOR_LINEAR_RANGE(1250000, 26, 27, 0),
	REGULATOR_LINEAR_RANGE(1300000, 28, 29, 0),
	REGULATOR_LINEAR_RANGE(1350000, 30, 31, 0),
	REGULATOR_LINEAR_RANGE(1400000, 32, 33, 0),
	REGULATOR_LINEAR_RANGE(1450000, 34, 35, 0),
	REGULATOR_LINEAR_RANGE(1500000, 36, 63, 0),
};

static const struct regulator_linear_range buck3_ranges[] = {
	REGULATOR_LINEAR_RANGE(1000000, 0, 19, 0),
	REGULATOR_LINEAR_RANGE(1100000, 20, 23, 0),
	REGULATOR_LINEAR_RANGE(1200000, 24, 27, 0),
	REGULATOR_LINEAR_RANGE(1300000, 28, 31, 0),
	REGULATOR_LINEAR_RANGE(1400000, 32, 35, 0),
	REGULATOR_LINEAR_RANGE(1500000, 36, 55, 100000),
	REGULATOR_LINEAR_RANGE(3400000, 56, 63, 0),
};

static const struct regulator_linear_range buck4_ranges[] = {
	REGULATOR_LINEAR_RANGE(600000, 0, 27, 25000),
	REGULATOR_LINEAR_RANGE(1300000, 28, 29, 0),
	REGULATOR_LINEAR_RANGE(1350000, 30, 31, 0),
	REGULATOR_LINEAR_RANGE(1400000, 32, 33, 0),
	REGULATOR_LINEAR_RANGE(1450000, 34, 35, 0),
	REGULATOR_LINEAR_RANGE(1500000, 36, 60, 100000),
	REGULATOR_LINEAR_RANGE(3900000, 61, 63, 0),
};

static const struct regulator_linear_range ldo1_ranges[] = {
	REGULATOR_LINEAR_RANGE(1700000, 0, 7, 0),
	REGULATOR_LINEAR_RANGE(1700000, 8, 24, 100000),
	REGULATOR_LINEAR_RANGE(3300000, 25, 31, 0),
};

static const struct regulator_linear_range ldo2_ranges[] = {
	REGULATOR_LINEAR_RANGE(1700000, 0, 7, 0),
	REGULATOR_LINEAR_RANGE(1700000, 8, 24, 100000),
	REGULATOR_LINEAR_RANGE(3300000, 25, 30, 0),
};

static const struct regulator_linear_range ldo3_ranges[] = {
	REGULATOR_LINEAR_RANGE(1700000, 0, 7, 0),
	REGULATOR_LINEAR_RANGE(1700000, 8, 24, 100000),
	REGULATOR_LINEAR_RANGE(3300000, 25, 30, 0),
	/* with index 31 LDO3 is in DDR mode */
	REGULATOR_LINEAR_RANGE(500000, 31, 31, 0),
};

static const struct regulator_linear_range ldo5_ranges[] = {
	REGULATOR_LINEAR_RANGE(1700000, 0, 7, 0),
	REGULATOR_LINEAR_RANGE(1700000, 8, 30, 100000),
	REGULATOR_LINEAR_RANGE(3900000, 31, 31, 0),
};

static const struct regulator_linear_range ldo6_ranges[] = {
	REGULATOR_LINEAR_RANGE(900000, 0, 24, 100000),
	REGULATOR_LINEAR_RANGE(3300000, 25, 31, 0),
};

static const struct regulator_ops stpmic1_ldo_ops = {
	.list_voltage = regulator_list_voltage_linear_range,
	.map_voltage = regulator_map_voltage_linear_range,
	.is_enabled = regulator_is_enabled_regmap,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
};

static const struct regulator_ops stpmic1_ldo3_ops = {
	.list_voltage = regulator_list_voltage_linear_range,
	.map_voltage = regulator_map_voltage_iterate,
	.is_enabled = regulator_is_enabled_regmap,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
};

static const struct regulator_ops stpmic1_ldo4_fixed_regul_ops = {
	.is_enabled = regulator_is_enabled_regmap,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
};

static const struct regulator_ops stpmic1_buck_ops = {
	.list_voltage = regulator_list_voltage_linear_range,
	.map_voltage = regulator_map_voltage_linear_range,
	.is_enabled = regulator_is_enabled_regmap,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
};

static const struct regulator_ops stpmic1_vref_ddr_ops = {
	.is_enabled = regulator_is_enabled_regmap,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
};

static const struct regulator_ops stpmic1_boost_regul_ops = {
	.is_enabled = regulator_is_enabled_regmap,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
};

static const struct regulator_ops stpmic1_switch_regul_ops = {
	.is_enabled = regulator_is_enabled_regmap,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
};

#define REG_LDO(ids, base) { \
	.n_voltages = 32, \
	.ops = &stpmic1_ldo_ops, \
	.linear_ranges = base ## _ranges, \
	.n_linear_ranges = ARRAY_SIZE(base ## _ranges), \
	.vsel_reg = ids##_ACTIVE_CR, \
	.vsel_mask = LDO_VOLTAGE_MASK, \
	.enable_reg = ids##_ACTIVE_CR, \
	.enable_mask = LDO_ENABLE_MASK, \
	.enable_val = 1, \
	.disable_val = 0, \
}

#define REG_LDO3(ids, base) { \
	.n_voltages = 32, \
	.ops = &stpmic1_ldo3_ops, \
	.linear_ranges = ldo3_ranges, \
	.n_linear_ranges = ARRAY_SIZE(ldo3_ranges), \
	.vsel_reg = LDO3_ACTIVE_CR, \
	.vsel_mask = LDO_VOLTAGE_MASK, \
	.enable_reg = LDO3_ACTIVE_CR, \
	.enable_mask = LDO_ENABLE_MASK, \
	.enable_val = 1, \
	.disable_val = 0, \
}

#define REG_LDO4(ids, base) { \
	.n_voltages = 1, \
	.ops = &stpmic1_ldo4_fixed_regul_ops, \
	.min_uV = 3300000, \
	.enable_reg = LDO4_ACTIVE_CR, \
	.enable_mask = LDO_ENABLE_MASK, \
	.enable_val = 1, \
	.disable_val = 0, \
}

#define REG_BUCK(ids, base) { \
	.ops = &stpmic1_buck_ops, \
	.n_voltages = 64, \
	.linear_ranges = base ## _ranges, \
	.n_linear_ranges = ARRAY_SIZE(base ## _ranges), \
	.vsel_reg = ids##_ACTIVE_CR, \
	.vsel_mask = BUCK_VOLTAGE_MASK, \
	.enable_reg = ids##_ACTIVE_CR, \
	.enable_mask = BUCK_ENABLE_MASK, \
	.enable_val = 1, \
	.disable_val = 0, \
}

#define REG_VREF_DDR(ids, base) { \
	.n_voltages = 1, \
	.ops = &stpmic1_vref_ddr_ops, \
	.min_uV = 500000, \
	.enable_reg = VREF_DDR_ACTIVE_CR, \
	.enable_mask = BUCK_ENABLE_MASK, \
	.enable_val = 1, \
	.disable_val = 0, \
}

#define REG_BOOST(ids, base) { \
	.n_voltages = 1, \
	.ops = &stpmic1_boost_regul_ops, \
	.min_uV = 0, \
	.enable_reg = BST_SW_CR, \
	.enable_mask = BOOST_ENABLED, \
	.enable_val = BOOST_ENABLED, \
	.disable_val = 0, \
}

#define REG_VBUS_OTG(ids, base) { \
	.n_voltages = 1, \
	.ops = &stpmic1_switch_regul_ops, \
	.min_uV = 0, \
	.enable_reg = BST_SW_CR, \
	.enable_mask = USBSW_OTG_SWITCH_ENABLED, \
	.enable_val = USBSW_OTG_SWITCH_ENABLED, \
	.disable_val = 0, \
}

#define REG_SW_OUT(ids, base) { \
	.n_voltages = 1, \
	.ops = &stpmic1_switch_regul_ops, \
	.min_uV = 0, \
	.enable_reg = BST_SW_CR, \
	.enable_mask = SWIN_SWOUT_ENABLED, \
	.enable_val = SWIN_SWOUT_ENABLED, \
	.disable_val = 0, \
}

static struct stpmic1_regulator_cfg stpmic1_regulator_cfgs[] = {
	[STPMIC1_BUCK1] = {
		.desc = REG_BUCK(BUCK1, buck1),
		.icc_reg = BUCKS_ICCTO_CR,
		.icc_mask = BIT(0),
		.mask_reset_reg = BUCKS_MASK_RESET_CR,
		.mask_reset_mask = BIT(0),
	},
	[STPMIC1_BUCK2] = {
		.desc = REG_BUCK(BUCK2, buck2),
		.icc_reg = BUCKS_ICCTO_CR,
		.icc_mask = BIT(1),
		.mask_reset_reg = BUCKS_MASK_RESET_CR,
		.mask_reset_mask = BIT(1),
	},
	[STPMIC1_BUCK3] = {
		.desc = REG_BUCK(BUCK3, buck3),
		.icc_reg = BUCKS_ICCTO_CR,
		.icc_mask = BIT(2),
		.mask_reset_reg = BUCKS_MASK_RESET_CR,
		.mask_reset_mask = BIT(2),
	},
	[STPMIC1_BUCK4] = {
		.desc = REG_BUCK(BUCK4, buck4),
		.icc_reg = BUCKS_ICCTO_CR,
		.icc_mask = BIT(3),
		.mask_reset_reg = BUCKS_MASK_RESET_CR,
		.mask_reset_mask = BIT(3),
	},
	[STPMIC1_LDO1] = {
		.desc = REG_LDO(LDO1, ldo1),
		.icc_reg = LDOS_ICCTO_CR,
		.icc_mask = BIT(0),
		.mask_reset_reg = LDOS_MASK_RESET_CR,
		.mask_reset_mask = BIT(0),
	},
	[STPMIC1_LDO2] = {
		.desc = REG_LDO(LDO2, ldo2),
		.icc_reg = LDOS_ICCTO_CR,
		.icc_mask = BIT(1),
		.mask_reset_reg = LDOS_MASK_RESET_CR,
		.mask_reset_mask = BIT(1),
	},
	[STPMIC1_LDO3] = {
		.desc = REG_LDO3(LDO3, ldo3),
		.icc_reg = LDOS_ICCTO_CR,
		.icc_mask = BIT(2),
		.mask_reset_reg = LDOS_MASK_RESET_CR,
		.mask_reset_mask = BIT(2),
	},
	[STPMIC1_LDO4] = {
		.desc = REG_LDO4(LDO4, ldo4),
		.icc_reg = LDOS_ICCTO_CR,
		.icc_mask = BIT(3),
		.mask_reset_reg = LDOS_MASK_RESET_CR,
		.mask_reset_mask = BIT(3),
	},
	[STPMIC1_LDO5] = {
		.desc = REG_LDO(LDO5, ldo5),
		.icc_reg = LDOS_ICCTO_CR,
		.icc_mask = BIT(4),
		.mask_reset_reg = LDOS_MASK_RESET_CR,
		.mask_reset_mask = BIT(4),
	},
	[STPMIC1_LDO6] = {
		.desc = REG_LDO(LDO6, ldo6),
		.icc_reg = LDOS_ICCTO_CR,
		.icc_mask = BIT(5),
		.mask_reset_reg = LDOS_MASK_RESET_CR,
		.mask_reset_mask = BIT(5),
	},
	[STPMIC1_VREF_DDR] = {
		.desc = REG_VREF_DDR(VREF_DDR, vref_ddr),
		.mask_reset_reg = LDOS_MASK_RESET_CR,
		.mask_reset_mask = BIT(6),
	},
	[STPMIC1_BOOST] = {
		.desc = REG_BOOST(BOOST, boost),
		.icc_reg = BUCKS_ICCTO_CR,
		.icc_mask = BIT(6),
	},
	[STPMIC1_VBUS_OTG] = {
		.desc = REG_VBUS_OTG(VBUS_OTG, pwr_sw1),
		.icc_reg = BUCKS_ICCTO_CR,
		.icc_mask = BIT(4),
	},
	[STPMIC1_SW_OUT] = {
		.desc = REG_SW_OUT(SW_OUT, pwr_sw2),
		.icc_reg = BUCKS_ICCTO_CR,
		.icc_mask = BIT(5),
	},
};

#define MATCH(_name, _id) \
	[STPMIC1_##_id] = { \
		.name = #_name, \
		.desc = &stpmic1_regulator_cfgs[STPMIC1_##_id].desc, \
	}

static struct of_regulator_match stpmic1_matches[] = {
	MATCH(buck1, BUCK1),
	MATCH(buck2, BUCK2),
	MATCH(buck3, BUCK3),
	MATCH(buck4, BUCK4),
	MATCH(ldo1, LDO1),
	MATCH(ldo2, LDO2),
	MATCH(ldo3, LDO3),
	MATCH(ldo4, LDO4),
	MATCH(ldo5, LDO5),
	MATCH(ldo6, LDO6),
	MATCH(vref_ddr, VREF_DDR),
	MATCH(boost, BOOST),
	MATCH(pwr_sw1, VBUS_OTG),
	MATCH(pwr_sw2, SW_OUT),
};

static int stpmic1_regulator_register(struct device_d *dev, int id,
				      struct of_regulator_match *match,
				      struct stpmic1_regulator_cfg *cfg)
{
	int ret;

	if (!match->of_node) {
		dev_dbg(dev, "Skip missing DTB regulator %s", match->name);
		return 0;
	}

	cfg->rdev.desc = &cfg->desc;
	cfg->rdev.dev = dev;
	cfg->rdev.regmap = dev_get_regmap(dev->parent, NULL);
	if (IS_ERR(cfg->rdev.regmap))
		return PTR_ERR(cfg->rdev.regmap);

	ret = of_regulator_register(&cfg->rdev, match->of_node);
	if (ret) {
		dev_err(dev, "failed to register %s regulator\n", match->name);
		return ret;
	}

	dev_dbg(dev, "registered %s\n", match->name);

	return 0;
}

static int stpmic1_regulator_probe(struct device_d *dev)
{
	int i, ret;

	ret = of_regulator_match(dev, dev->device_node, stpmic1_matches,
				 ARRAY_SIZE(stpmic1_matches));
	if (ret < 0) {
		dev_err(dev, "Error in PMIC regulator device tree node");
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(stpmic1_regulator_cfgs); i++) {
		ret = stpmic1_regulator_register(dev, i, &stpmic1_matches[i],
						 &stpmic1_regulator_cfgs[i]);
		if (ret < 0)
			return ret;
	}

	dev_dbg(dev, "probed\n");

	return 0;
}

static __maybe_unused const struct of_device_id stpmic1_regulator_of_match[] = {
	{ .compatible = "st,stpmic1-regulators" },
	{ /* sentinel */ },
};

static struct driver_d stpmic1_regulator_driver = {
	.name = "stpmic1-regulator",
	.probe = stpmic1_regulator_probe,
	.of_compatible = DRV_OF_COMPAT(stpmic1_regulator_of_match),
};
coredevice_platform_driver(stpmic1_regulator_driver);
