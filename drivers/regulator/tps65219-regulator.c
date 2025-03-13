// SPDX-License-Identifier: GPL-2.0
//
// tps65219-regulator.c
//
// Regulator driver for TPS65219 PMIC
//
// Copyright (C) 2022 BayLibre Incorporated - https://www.baylibre.com/
//
// This implementation derived from tps65218 authored by
// "J Keerthy <j-keerthy@ti.com>"
//

#include <linux/kernel.h>
#include <linux/mfd/tps65219.h>
#include <of_device.h>
#include <linux/regmap.h>
#include <linux/regulator/of_regulator.h>
#include <regulator.h>
#include <linux/device.h>

struct tps65219_regulator_irq_type {
	const char *irq_name;
	const char *regulator_name;
	const char *event_name;
	unsigned long event;
};

#define TPS65219_REGULATOR(_name, _of, _id, _type, _ops, _n, _vr, _vm, _er, \
			   _em, _cr, _cm, _lr, _nlr, _delay, _fuv, \
			   _ct, _ncl, _bpm) \
	{								\
		.name			= _name,			\
		.of_match		= _of,				\
		.regulators_node	= of_match_ptr("regulators"),	\
		.supply_name		= _of,				\
		.id			= _id,				\
		.ops			= &(_ops),			\
		.n_voltages		= _n,				\
		.vsel_reg		= _vr,				\
		.vsel_mask		= _vm,				\
		.enable_reg		= _er,				\
		.enable_mask		= _em,				\
		.volt_table		= NULL,				\
		.linear_ranges		= _lr,				\
		.n_linear_ranges	= _nlr,				\
		.fixed_uV		= _fuv,				\
	}								\

static const struct regulator_linear_range bucks_ranges[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x0, 0x1f, 25000),
	REGULATOR_LINEAR_RANGE(1400000, 0x20, 0x33, 100000),
	REGULATOR_LINEAR_RANGE(3400000, 0x34, 0x3f, 0),
};

static const struct regulator_linear_range ldos_1_2_ranges[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x0, 0x37, 50000),
	REGULATOR_LINEAR_RANGE(3400000, 0x38, 0x3f, 0),
};

static const struct regulator_linear_range ldos_3_4_ranges[] = {
	REGULATOR_LINEAR_RANGE(1200000, 0x0, 0xC, 0),
	REGULATOR_LINEAR_RANGE(1250000, 0xD, 0x35, 50000),
	REGULATOR_LINEAR_RANGE(3300000, 0x36, 0x3F, 0),
};

/* Operations permitted on BUCK1/2/3 */
static const struct regulator_ops tps65219_bucks_ops = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
};

/* Operations permitted on LDO1/2 */
static const struct regulator_ops tps65219_ldos_1_2_ops = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
};

/* Operations permitted on LDO3/4 */
static const struct regulator_ops tps65219_ldos_3_4_ops = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
};

static const struct regulator_desc regulators[] = {
	TPS65219_REGULATOR("BUCK1", "buck1", TPS65219_BUCK_1,
			   REGULATOR_VOLTAGE, tps65219_bucks_ops, 64,
			   TPS65219_REG_BUCK1_VOUT,
			   TPS65219_BUCKS_LDOS_VOUT_VSET_MASK,
			   TPS65219_REG_ENABLE_CTRL,
			   TPS65219_ENABLE_BUCK1_EN_MASK, 0, 0, bucks_ranges,
			   3, 4000, 0, NULL, 0, 0),
	TPS65219_REGULATOR("BUCK2", "buck2", TPS65219_BUCK_2,
			   REGULATOR_VOLTAGE, tps65219_bucks_ops, 64,
			   TPS65219_REG_BUCK2_VOUT,
			   TPS65219_BUCKS_LDOS_VOUT_VSET_MASK,
			   TPS65219_REG_ENABLE_CTRL,
			   TPS65219_ENABLE_BUCK2_EN_MASK, 0, 0, bucks_ranges,
			   3, 4000, 0, NULL, 0, 0),
	TPS65219_REGULATOR("BUCK3", "buck3", TPS65219_BUCK_3,
			   REGULATOR_VOLTAGE, tps65219_bucks_ops, 64,
			   TPS65219_REG_BUCK3_VOUT,
			   TPS65219_BUCKS_LDOS_VOUT_VSET_MASK,
			   TPS65219_REG_ENABLE_CTRL,
			   TPS65219_ENABLE_BUCK3_EN_MASK, 0, 0, bucks_ranges,
			   3, 0, 0, NULL, 0, 0),
	TPS65219_REGULATOR("LDO1", "ldo1", TPS65219_LDO_1,
			   REGULATOR_VOLTAGE, tps65219_ldos_1_2_ops, 64,
			   TPS65219_REG_LDO1_VOUT,
			   TPS65219_BUCKS_LDOS_VOUT_VSET_MASK,
			   TPS65219_REG_ENABLE_CTRL,
			   TPS65219_ENABLE_LDO1_EN_MASK, 0, 0, ldos_1_2_ranges,
			   2, 0, 0, NULL, 0, TPS65219_LDOS_BYP_CONFIG_MASK),
	TPS65219_REGULATOR("LDO2", "ldo2", TPS65219_LDO_2,
			   REGULATOR_VOLTAGE, tps65219_ldos_1_2_ops, 64,
			   TPS65219_REG_LDO2_VOUT,
			   TPS65219_BUCKS_LDOS_VOUT_VSET_MASK,
			   TPS65219_REG_ENABLE_CTRL,
			   TPS65219_ENABLE_LDO2_EN_MASK, 0, 0, ldos_1_2_ranges,
			   2, 0, 0, NULL, 0, TPS65219_LDOS_BYP_CONFIG_MASK),
	TPS65219_REGULATOR("LDO3", "ldo3", TPS65219_LDO_3,
			   REGULATOR_VOLTAGE, tps65219_ldos_3_4_ops, 64,
			   TPS65219_REG_LDO3_VOUT,
			   TPS65219_BUCKS_LDOS_VOUT_VSET_MASK,
			   TPS65219_REG_ENABLE_CTRL,
			   TPS65219_ENABLE_LDO3_EN_MASK, 0, 0, ldos_3_4_ranges,
			   3, 0, 0, NULL, 0, 0),
	TPS65219_REGULATOR("LDO4", "ldo4", TPS65219_LDO_4,
			   REGULATOR_VOLTAGE, tps65219_ldos_3_4_ops, 64,
			   TPS65219_REG_LDO4_VOUT,
			   TPS65219_BUCKS_LDOS_VOUT_VSET_MASK,
			   TPS65219_REG_ENABLE_CTRL,
			   TPS65219_ENABLE_LDO4_EN_MASK, 0, 0, ldos_3_4_ranges,
			   3, 0, 0, NULL, 0, 0),
};

static int tps65219_regulator_probe(struct device *dev)
{
	struct tps65219 *tps = dev->parent->priv;
	struct regulator_dev *rdev;
	struct regulator_config config = { };
	int i;
	struct regulator_dev *rdevtbl[7];

	config.dev = tps->dev;
	config.driver_data = tps;
	config.regmap = tps->regmap;

	for (i = 0; i < ARRAY_SIZE(regulators); i++) {
		rdev = regulator_register(tps->dev, &regulators[i],
					       &config);
		if (IS_ERR(rdev)) {
			dev_err(tps->dev, "failed to register %s regulator\n",
				regulators[i].name);
			return PTR_ERR(rdev);
		}
		rdevtbl[i] = rdev;
	}

	return 0;
}

static struct driver tps65219_regulator_driver = {
	.name = "tps65219-regulator",
	.probe = tps65219_regulator_probe,
};
coredevice_platform_driver(tps65219_regulator_driver);

MODULE_AUTHOR("Jerome Neanne <j-neanne@baylibre.com>");
MODULE_DESCRIPTION("TPS65219 voltage regulator driver");
MODULE_ALIAS("platform:tps65219-pmic");
MODULE_LICENSE("GPL");
