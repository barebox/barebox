// SPDX-License-Identifier: GPL-2.0-only
/*
 * Regulator driver for Rockchip RK808
 *
 * Copyright (c) 2014, Fuzhou Rockchip Electronics Co., Ltd
 *
 * Author: Chris Zhong <zyw@rock-chips.com>
 * Author: Zhang Qing <zhangqing@rock-chips.com>
 */

#include <common.h>
#include <driver.h>
#include <gpio.h>
#include <init.h>
#include <i2c/i2c.h>
#include <of_device.h>
#include <regmap.h>
#include <linux/regulator/of_regulator.h>
#include <regulator.h>
#include <linux/mfd/rk808.h>

/* Field Definitions */
#define RK808_BUCK_VSEL_MASK	0x3f
#define RK808_BUCK4_VSEL_MASK	0xf
#define RK808_LDO_VSEL_MASK	0x1f

#define RK809_BUCK5_VSEL_MASK		0x7

#define RK817_LDO_VSEL_MASK		0x7f
#define RK817_BOOST_VSEL_MASK		0x7
#define RK817_BUCK_VSEL_MASK		0x7f

#define RK818_BUCK_VSEL_MASK		0x3f
#define RK818_BUCK4_VSEL_MASK		0x1f
#define RK818_LDO_VSEL_MASK		0x1f
#define RK818_LDO3_ON_VSEL_MASK		0xf
#define RK818_BOOST_ON_VSEL_MASK	0xe0

#define ENABLE_MASK(id)			(BIT(id) | BIT(4 + (id)))
#define DISABLE_VAL(id)			(BIT(4 + (id)))

#define RK817_BOOST_DESC(_supply_name, _min, _max, _step, _vreg,\
	_vmask, _ereg, _emask, _enval, _disval, _etime)		\
	{{							\
		.supply_name	= (_supply_name),			\
		.n_voltages	= (((_max) - (_min)) / (_step) + 1),	\
		.min_uV		= (_min) * 1000,			\
		.uV_step	= (_step) * 1000,			\
		.vsel_reg	= (_vreg),				\
		.vsel_mask	= (_vmask),				\
		.enable_reg	= (_ereg),				\
		.enable_mask	= (_emask),				\
		.enable_val     = (_enval),				\
		.disable_val     = (_disval),				\
		.off_on_delay	= (_etime),				\
		.ops		= &rk817_boost_ops,			\
	}}

#define RK8XX_DESC_COM(_supply_name, _min, _max, _step, _vreg,	\
	_vmask, _ereg, _emask, _enval, _disval, _etime, _ops)		\
	{{								\
		.supply_name	= (_supply_name),			\
		.n_voltages	= (((_max) - (_min)) / (_step) + 1),	\
		.min_uV		= (_min) * 1000,			\
		.uV_step	= (_step) * 1000,			\
		.vsel_reg	= (_vreg),				\
		.vsel_mask	= (_vmask),				\
		.enable_reg	= (_ereg),				\
		.enable_mask	= (_emask),				\
		.enable_val     = (_enval),				\
		.disable_val     = (_disval),				\
		.off_on_delay	= (_etime),				\
		.ops		= _ops,			\
	}}

#define RK805_DESC(_supply_name, _min, _max, _step, _vreg,	\
	_vmask, _ereg, _emask, _etime)					\
	RK8XX_DESC_COM(_supply_name, _min, _max, _step, _vreg,	\
	_vmask, _ereg, _emask, 0, 0, _etime, &rk808_reg_ops)

#define RK8XX_DESC(_supply_name, _min, _max, _step, _vreg,	\
	_vmask, _ereg, _emask, _etime)					\
	RK8XX_DESC_COM(_supply_name, _min, _max, _step, _vreg,	\
	_vmask, _ereg, _emask, 0, 0, _etime, &rk808_reg_ops)

#define RK817_DESC(_supply_name, _min, _max, _step, _vreg,	\
	_vmask, _ereg, _emask, _disval, _etime)				\
	RK8XX_DESC_COM(_supply_name, _min, _max, _step, _vreg,	\
	_vmask, _ereg, _emask, _emask, _disval, _etime, &rk817_reg_ops)

#define RKXX_DESC_SWITCH_COM(_supply_name,_ereg, _emask,	\
	_enval, _disval, _ops)						\
	{{								\
		.supply_name	= (_supply_name),			\
		.enable_reg	= (_ereg),				\
		.enable_mask	= (_emask),				\
		.enable_val     = (_enval),				\
		.disable_val     = (_disval),				\
		.ops		= _ops					\
	}}

#define RK817_DESC_SWITCH(_supply_name, _ereg, _emask,		\
	_disval)							\
	RKXX_DESC_SWITCH_COM(_supply_name, _ereg, _emask,	\
	_emask, _disval, &rk817_switch_ops)

#define RK8XX_DESC_SWITCH(_supply_name, _ereg, _emask)		\
	RKXX_DESC_SWITCH_COM(_supply_name, _ereg, _emask,	\
	0, 0, &rk808_switch_ops)

static const struct regulator_linear_range rk808_ldo3_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(800000, 0, 13, 100000),
	REGULATOR_LINEAR_RANGE(2500000, 15, 15, 0),
};

#define RK809_BUCK5_SEL_CNT		(8)

static const struct regulator_linear_range rk809_buck5_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(1500000, 0, 0, 0),
	REGULATOR_LINEAR_RANGE(1800000, 1, 3, 200000),
	REGULATOR_LINEAR_RANGE(2800000, 4, 5, 200000),
	REGULATOR_LINEAR_RANGE(3300000, 6, 7, 300000),
};

#define RK817_BUCK1_MIN0 500000
#define RK817_BUCK1_MAX0 1500000

#define RK817_BUCK1_MIN1 1600000
#define RK817_BUCK1_MAX1 2400000

#define RK817_BUCK3_MAX1 3400000

#define RK817_BUCK1_STP0 12500
#define RK817_BUCK1_STP1 100000

#define RK817_BUCK1_SEL0 ((RK817_BUCK1_MAX0 - RK817_BUCK1_MIN0) /\
						  RK817_BUCK1_STP0)
#define RK817_BUCK1_SEL1 ((RK817_BUCK1_MAX1 - RK817_BUCK1_MIN1) /\
						  RK817_BUCK1_STP1)

#define RK817_BUCK3_SEL1 ((RK817_BUCK3_MAX1 - RK817_BUCK1_MIN1) /\
						  RK817_BUCK1_STP1)

#define RK817_BUCK1_SEL_CNT (RK817_BUCK1_SEL0 + RK817_BUCK1_SEL1 + 1)
#define RK817_BUCK3_SEL_CNT (RK817_BUCK1_SEL0 + RK817_BUCK3_SEL1 + 1)

static const struct regulator_linear_range rk817_buck1_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(RK817_BUCK1_MIN0, 0,
			       RK817_BUCK1_SEL0, RK817_BUCK1_STP0),
	REGULATOR_LINEAR_RANGE(RK817_BUCK1_MIN1, RK817_BUCK1_SEL0 + 1,
			       RK817_BUCK1_SEL_CNT, RK817_BUCK1_STP1),
};

static const struct regulator_linear_range rk817_buck3_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(RK817_BUCK1_MIN0, 0,
			       RK817_BUCK1_SEL0, RK817_BUCK1_STP0),
	REGULATOR_LINEAR_RANGE(RK817_BUCK1_MIN1, RK817_BUCK1_SEL0 + 1,
			       RK817_BUCK3_SEL_CNT, RK817_BUCK1_STP1),
};

struct rk_regulator_cfg {
	struct regulator_desc desc;
	struct regulator_dev rdev;
};

static int rk8xx_is_enabled_wmsk_regmap(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, rdev->desc->enable_reg, &val);
	if (ret != 0)
		return ret;

	/* add write mask bit */
	val |= (rdev->desc->enable_mask & 0xf0);
	val &= rdev->desc->enable_mask;

	if (rdev->desc->enable_is_inverted) {
		if (rdev->desc->enable_val)
			return val != rdev->desc->enable_val;
		return (val == 0);
	}
	if (rdev->desc->enable_val)
		return val == rdev->desc->enable_val;
	return val != 0;
}

static struct regulator_ops rk808_buck1_2_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};

static struct regulator_ops rk808_reg_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};

static struct regulator_ops rk808_reg_ops_ranges = {
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};

static struct regulator_ops rk808_switch_ops = {
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};

static const struct regulator_linear_range rk805_buck_1_2_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(712500, 0, 59, 12500),
	REGULATOR_LINEAR_RANGE(1800000, 60, 62, 200000),
	REGULATOR_LINEAR_RANGE(2300000, 63, 63, 0),
};

static const struct regulator_ops rk809_buck5_ops_range = {
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= rk8xx_is_enabled_wmsk_regmap,
};

static const struct regulator_ops rk817_reg_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= rk8xx_is_enabled_wmsk_regmap,
};

static const struct regulator_ops rk817_boost_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= rk8xx_is_enabled_wmsk_regmap,
};

static const struct regulator_ops rk817_buck_ops_range = {
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= rk8xx_is_enabled_wmsk_regmap,
};

static const struct regulator_ops rk817_switch_ops = {
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= rk8xx_is_enabled_wmsk_regmap,
};

static struct rk_regulator_cfg rk805_reg[] = {
	{{
		/* .name = "DCDC_REG1", */
		.supply_name = "vcc1",
		.ops = &rk808_reg_ops_ranges,
		.n_voltages = 64,
		.linear_ranges = rk805_buck_1_2_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk805_buck_1_2_voltage_ranges),
		.vsel_reg = RK805_BUCK1_ON_VSEL_REG,
		.vsel_mask = RK818_BUCK_VSEL_MASK,
		.enable_reg = RK805_DCDC_EN_REG,
		.enable_mask = BIT(0),
	}}, {{
		/* .name = "DCDC_REG2", */
		.supply_name = "vcc2",
		.ops = &rk808_reg_ops_ranges,
		.n_voltages = 64,
		.linear_ranges = rk805_buck_1_2_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk805_buck_1_2_voltage_ranges),
		.vsel_reg = RK805_BUCK2_ON_VSEL_REG,
		.vsel_mask = RK818_BUCK_VSEL_MASK,
		.enable_reg = RK805_DCDC_EN_REG,
		.enable_mask = BIT(1),
	}}, {{
		/* .name = "DCDC_REG3", */
		.supply_name = "vcc3",
		.ops = &rk808_switch_ops,
		.n_voltages = 1,
		.enable_reg = RK805_DCDC_EN_REG,
		.enable_mask = BIT(2),
	}},

	RK805_DESC(/* "DCDC_REG4", */ "vcc4", 800, 3400, 100,
		RK805_BUCK4_ON_VSEL_REG, RK818_BUCK4_VSEL_MASK,
		RK805_DCDC_EN_REG, BIT(3), 0),

	RK805_DESC(/* "LDO_REG1", */ "vcc5", 800, 3400, 100,
		RK805_LDO1_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK805_LDO_EN_REG,
		BIT(0), 400),
	RK805_DESC(/* "LDO_REG2", */ "vcc5", 800, 3400, 100,
		RK805_LDO2_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK805_LDO_EN_REG,
		BIT(1), 400),
	RK805_DESC(/* "LDO_REG3", */ "vcc6", 800, 3400, 100,
		RK805_LDO3_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK805_LDO_EN_REG,
		BIT(2), 400),
};
static_assert(ARRAY_SIZE(rk805_reg) == RK805_NUM_REGULATORS);

static struct rk_regulator_cfg rk808_reg[] = {
	{{
		/* .name = "DCDC_REG1", */
		.supply_name = "vcc1",
		.ops = &rk808_buck1_2_ops,
		.min_uV = 712500,
		.uV_step = 12500,
		.n_voltages = 64,
		.vsel_reg = RK808_BUCK1_ON_VSEL_REG,
		.vsel_mask = RK808_BUCK_VSEL_MASK,
		.enable_reg = RK808_DCDC_EN_REG,
		.enable_mask = BIT(0),
	}}, {{
		/* .name = "DCDC_REG2", */
		.supply_name = "vcc2",
		.ops = &rk808_buck1_2_ops,
		.min_uV = 712500,
		.uV_step = 12500,
		.n_voltages = 64,
		.vsel_reg = RK808_BUCK2_ON_VSEL_REG,
		.vsel_mask = RK808_BUCK_VSEL_MASK,
		.enable_reg = RK808_DCDC_EN_REG,
		.enable_mask = BIT(1),
	}}, {{
		/* .name = "DCDC_REG3", */
		.supply_name = "vcc3",
		.ops = &rk808_switch_ops,
		.n_voltages = 1,
		.enable_reg = RK808_DCDC_EN_REG,
		.enable_mask = BIT(2),
	}}, {{
		/* .name = "DCDC_REG4", */
		.supply_name = "vcc4",
		.ops = &rk808_reg_ops,
		.min_uV = 1800000,
		.uV_step = 100000,
		.n_voltages = 16,
		.vsel_reg = RK808_BUCK4_ON_VSEL_REG,
		.vsel_mask = RK808_BUCK4_VSEL_MASK,
		.enable_reg = RK808_DCDC_EN_REG,
		.enable_mask = BIT(3),
	}}, {{
		/* .name = "LDO_REG1", */
		.supply_name = "vcc6",
		.ops = &rk808_reg_ops,
		.min_uV = 1800000,
		.uV_step = 100000,
		.n_voltages = 17,
		.vsel_reg = RK808_LDO1_ON_VSEL_REG,
		.vsel_mask = RK808_LDO_VSEL_MASK,
		.enable_reg = RK808_LDO_EN_REG,
		.enable_mask = BIT(0),
		.off_on_delay = 400,
	}}, {{
		/* .name = "LDO_REG2", */
		.supply_name = "vcc6",
		.ops = &rk808_reg_ops,
		.min_uV = 1800000,
		.uV_step = 100000,
		.n_voltages = 17,
		.vsel_reg = RK808_LDO2_ON_VSEL_REG,
		.vsel_mask = RK808_LDO_VSEL_MASK,
		.enable_reg = RK808_LDO_EN_REG,
		.enable_mask = BIT(1),
		.off_on_delay = 400,
	}}, {{
		/* .name = "LDO_REG3", */
		.supply_name = "vcc7",
		.ops = &rk808_reg_ops_ranges,
		.n_voltages = 16,
		.linear_ranges = rk808_ldo3_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk808_ldo3_voltage_ranges),
		.vsel_reg = RK808_LDO3_ON_VSEL_REG,
		.vsel_mask = RK808_BUCK4_VSEL_MASK,
		.enable_reg = RK808_LDO_EN_REG,
		.enable_mask = BIT(2),
		.off_on_delay = 400,
	}}, {{
		/* .name = "LDO_REG4", */
		.supply_name = "vcc9",
		.ops = &rk808_reg_ops,
		.min_uV = 1800000,
		.uV_step = 100000,
		.n_voltages = 17,
		.vsel_reg = RK808_LDO4_ON_VSEL_REG,
		.vsel_mask = RK808_LDO_VSEL_MASK,
		.enable_reg = RK808_LDO_EN_REG,
		.enable_mask = BIT(3),
		.off_on_delay = 400,
	}}, {{
		/* .name = "LDO_REG5", */
		.supply_name = "vcc9",
		.ops = &rk808_reg_ops,
		.min_uV = 1800000,
		.uV_step = 100000,
		.n_voltages = 17,
		.vsel_reg = RK808_LDO5_ON_VSEL_REG,
		.vsel_mask = RK808_LDO_VSEL_MASK,
		.enable_reg = RK808_LDO_EN_REG,
		.enable_mask = BIT(4),
		.off_on_delay = 400,
	}}, {{
		/* .name = "LDO_REG6", */
		.supply_name = "vcc10",
		.ops = &rk808_reg_ops,
		.min_uV = 800000,
		.uV_step = 100000,
		.n_voltages = 18,
		.vsel_reg = RK808_LDO6_ON_VSEL_REG,
		.vsel_mask = RK808_LDO_VSEL_MASK,
		.enable_reg = RK808_LDO_EN_REG,
		.enable_mask = BIT(5),
		.off_on_delay = 400,
	}}, {{
		/* .name = "LDO_REG7", */
		.supply_name = "vcc7",
		.ops = &rk808_reg_ops,
		.min_uV = 800000,
		.uV_step = 100000,
		.n_voltages = 18,
		.vsel_reg = RK808_LDO7_ON_VSEL_REG,
		.vsel_mask = RK808_LDO_VSEL_MASK,
		.enable_reg = RK808_LDO_EN_REG,
		.enable_mask = BIT(6),
		.off_on_delay = 400,
	}}, {{
		/* .name = "LDO_REG8", */
		.supply_name = "vcc11",
		.ops = &rk808_reg_ops,
		.min_uV = 1800000,
		.uV_step = 100000,
		.n_voltages = 17,
		.vsel_reg = RK808_LDO8_ON_VSEL_REG,
		.vsel_mask = RK808_LDO_VSEL_MASK,
		.enable_reg = RK808_LDO_EN_REG,
		.enable_mask = BIT(7),
		.off_on_delay = 400,
	}}, {{
		/* .name = "SWITCH_REG1", */
		.supply_name = "vcc8",
		.ops = &rk808_switch_ops,
		.enable_reg = RK808_DCDC_EN_REG,
		.enable_mask = BIT(5),
	}}, {{
		/* .name = "SWITCH_REG2", */
		.supply_name = "vcc12",
		.ops = &rk808_switch_ops,
		.enable_reg = RK808_DCDC_EN_REG,
		.enable_mask = BIT(6),
	}},
};
static_assert(ARRAY_SIZE(rk808_reg) == RK808_NUM_REGULATORS);

static struct rk_regulator_cfg rk809_reg[] = {
	{{
		/* .name = "DCDC_REG1", */
		.supply_name = "vcc1",
		.ops = &rk817_buck_ops_range,
		.n_voltages = RK817_BUCK1_SEL_CNT + 1,
		.linear_ranges = rk817_buck1_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk817_buck1_voltage_ranges),
		.vsel_reg = RK817_BUCK1_ON_VSEL_REG,
		.vsel_mask = RK817_BUCK_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(0),
		.enable_mask = ENABLE_MASK(RK817_ID_DCDC1),
		.enable_val = ENABLE_MASK(RK817_ID_DCDC1),
		.disable_val = DISABLE_VAL(RK817_ID_DCDC1),
	}}, {{
		/* .name = "DCDC_REG2", */
		.supply_name = "vcc2",
		.ops = &rk817_buck_ops_range,
		.n_voltages = RK817_BUCK1_SEL_CNT + 1,
		.linear_ranges = rk817_buck1_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk817_buck1_voltage_ranges),
		.vsel_reg = RK817_BUCK2_ON_VSEL_REG,
		.vsel_mask = RK817_BUCK_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(0),
		.enable_mask = ENABLE_MASK(RK817_ID_DCDC2),
		.enable_val = ENABLE_MASK(RK817_ID_DCDC2),
		.disable_val = DISABLE_VAL(RK817_ID_DCDC2),
	}}, {{
		/* .name = "DCDC_REG3", */
		.supply_name = "vcc3",
		.ops = &rk817_buck_ops_range,
		.n_voltages = RK817_BUCK1_SEL_CNT + 1,
		.linear_ranges = rk817_buck1_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk817_buck1_voltage_ranges),
		.vsel_reg = RK817_BUCK3_ON_VSEL_REG,
		.vsel_mask = RK817_BUCK_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(0),
		.enable_mask = ENABLE_MASK(RK817_ID_DCDC3),
		.enable_val = ENABLE_MASK(RK817_ID_DCDC3),
		.disable_val = DISABLE_VAL(RK817_ID_DCDC3),
	}}, {{
		/* .name = "DCDC_REG4", */
		.supply_name = "vcc4",
		.ops = &rk817_buck_ops_range,
		.n_voltages = RK817_BUCK3_SEL_CNT + 1,
		.linear_ranges = rk817_buck3_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk817_buck3_voltage_ranges),
		.vsel_reg = RK817_BUCK4_ON_VSEL_REG,
		.vsel_mask = RK817_BUCK_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(0),
		.enable_mask = ENABLE_MASK(RK817_ID_DCDC4),
		.enable_val = ENABLE_MASK(RK817_ID_DCDC4),
		.disable_val = DISABLE_VAL(RK817_ID_DCDC4),
	}},
	RK817_DESC(/* "LDO_REG1", */ "vcc5", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(0), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(1), ENABLE_MASK(0),
		   DISABLE_VAL(0), 400),
	RK817_DESC(/* "LDO_REG2", */ "vcc5", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(1), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(1), ENABLE_MASK(1),
		   DISABLE_VAL(1), 400),
	RK817_DESC(/* "LDO_REG3", */ "vcc5", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(2), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(1), ENABLE_MASK(2),
		   DISABLE_VAL(2), 400),
	RK817_DESC(/* "LDO_REG4", */ "vcc6", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(3), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(1), ENABLE_MASK(3),
		   DISABLE_VAL(3), 400),
	RK817_DESC(/* "LDO_REG5", */ "vcc6", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(4), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(2), ENABLE_MASK(0),
		   DISABLE_VAL(0), 400),
	RK817_DESC(/* "LDO_REG6", */ "vcc6", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(5), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(2), ENABLE_MASK(1),
		   DISABLE_VAL(1), 400),
	RK817_DESC(/* "LDO_REG7", */ "vcc7", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(6), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(2), ENABLE_MASK(2),
		   DISABLE_VAL(2), 400),
	RK817_DESC(/* "LDO_REG8", */ "vcc7", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(7), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(2), ENABLE_MASK(3),
		   DISABLE_VAL(3), 400),
	RK817_DESC(/* "LDO_REG9", */ "vcc7", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(8), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(3), ENABLE_MASK(0),
		   DISABLE_VAL(0), 400),
	{{
		/* .name = "DCDC_REG5", */
		.supply_name = "vcc9",
		.ops = &rk809_buck5_ops_range,
		.n_voltages = RK809_BUCK5_SEL_CNT,
		.linear_ranges = rk809_buck5_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk809_buck5_voltage_ranges),
		.vsel_reg = RK809_BUCK5_CONFIG(0),
		.vsel_mask = RK809_BUCK5_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(3),
		.enable_mask = ENABLE_MASK(1),
		.enable_val = ENABLE_MASK(1),
		.disable_val = DISABLE_VAL(1),
	}},
	RK817_DESC_SWITCH(/* "SWITCH_REG1", */ "vcc9",
			  RK817_POWER_EN_REG(3), ENABLE_MASK(2), DISABLE_VAL(2)),
	RK817_DESC_SWITCH(/* "SWITCH_REG2", */ "vcc8",
			  RK817_POWER_EN_REG(3), ENABLE_MASK(3), DISABLE_VAL(3)),
};
static_assert(ARRAY_SIZE(rk809_reg) == RK809_NUM_REGULATORS);

static struct rk_regulator_cfg rk817_reg[] = {
	{{
		/* .name = "DCDC_REG1", */
		.supply_name = "vcc1",
		.ops = &rk817_buck_ops_range,
		.n_voltages = RK817_BUCK1_SEL_CNT + 1,
		.linear_ranges = rk817_buck1_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk817_buck1_voltage_ranges),
		.vsel_reg = RK817_BUCK1_ON_VSEL_REG,
		.vsel_mask = RK817_BUCK_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(0),
		.enable_mask = ENABLE_MASK(RK817_ID_DCDC1),
		.enable_val = ENABLE_MASK(RK817_ID_DCDC1),
		.disable_val = DISABLE_VAL(RK817_ID_DCDC1),
	 }}, {{
		/* .name = "DCDC_REG2", */
		.supply_name = "vcc2",
		.ops = &rk817_buck_ops_range,
		.n_voltages = RK817_BUCK1_SEL_CNT + 1,
		.linear_ranges = rk817_buck1_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk817_buck1_voltage_ranges),
		.vsel_reg = RK817_BUCK2_ON_VSEL_REG,
		.vsel_mask = RK817_BUCK_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(0),
		.enable_mask = ENABLE_MASK(RK817_ID_DCDC2),
		.enable_val = ENABLE_MASK(RK817_ID_DCDC2),
		.disable_val = DISABLE_VAL(RK817_ID_DCDC2),
	 }}, {{
		/* .name = "DCDC_REG3", */
		.supply_name = "vcc3",
		.ops = &rk817_buck_ops_range,
		.n_voltages = RK817_BUCK1_SEL_CNT + 1,
		.linear_ranges = rk817_buck1_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk817_buck1_voltage_ranges),
		.vsel_reg = RK817_BUCK3_ON_VSEL_REG,
		.vsel_mask = RK817_BUCK_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(0),
		.enable_mask = ENABLE_MASK(RK817_ID_DCDC3),
		.enable_val = ENABLE_MASK(RK817_ID_DCDC3),
		.disable_val = DISABLE_VAL(RK817_ID_DCDC3),
	 }}, {{
		/* .name = "DCDC_REG4", */
		.supply_name = "vcc4",
		.ops = &rk817_buck_ops_range,
		.n_voltages = RK817_BUCK3_SEL_CNT + 1,
		.linear_ranges = rk817_buck3_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk817_buck3_voltage_ranges),
		.vsel_reg = RK817_BUCK4_ON_VSEL_REG,
		.vsel_mask = RK817_BUCK_VSEL_MASK,
		.enable_reg = RK817_POWER_EN_REG(0),
		.enable_mask = ENABLE_MASK(RK817_ID_DCDC4),
		.enable_val = ENABLE_MASK(RK817_ID_DCDC4),
		.disable_val = DISABLE_VAL(RK817_ID_DCDC4),
	}},
	RK817_DESC(/* "LDO_REG1", */ "vcc5", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(0), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(1), ENABLE_MASK(0),
		   DISABLE_VAL(0), 400),
	RK817_DESC(/* "LDO_REG2", */ "vcc5", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(1), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(1), ENABLE_MASK(1),
		   DISABLE_VAL(1), 400),
	RK817_DESC(/* "LDO_REG3", */ "vcc5", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(2), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(1), ENABLE_MASK(2),
		   DISABLE_VAL(2), 400),
	RK817_DESC(/* "LDO_REG4", */ "vcc6", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(3), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(1), ENABLE_MASK(3),
		   DISABLE_VAL(3), 400),
	RK817_DESC(/* "LDO_REG5", */ "vcc6", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(4), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(2), ENABLE_MASK(0),
		   DISABLE_VAL(0), 400),
	RK817_DESC(/* "LDO_REG6", */ "vcc6", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(5), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(2), ENABLE_MASK(1),
		   DISABLE_VAL(1), 400),
	RK817_DESC(/* "LDO_REG7", */ "vcc7", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(6), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(2), ENABLE_MASK(2),
		   DISABLE_VAL(2), 400),
	RK817_DESC(/* "LDO_REG8", */ "vcc7", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(7), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(2), ENABLE_MASK(3),
		   DISABLE_VAL(3), 400),
	RK817_DESC(/* "LDO_REG9", */ "vcc7", 600, 3400, 25,
		   RK817_LDO_ON_VSEL_REG(8), RK817_LDO_VSEL_MASK,
		   RK817_POWER_EN_REG(3), ENABLE_MASK(0),
		   DISABLE_VAL(0), 400),
	RK817_BOOST_DESC(/* "BOOST", */ "vcc8", 4700, 5400, 100,
			 RK817_BOOST_OTG_CFG, RK817_BOOST_VSEL_MASK,
			 RK817_POWER_EN_REG(3), ENABLE_MASK(1), ENABLE_MASK(1),
		   DISABLE_VAL(1), 400),
	RK817_DESC_SWITCH(/* "OTG_SWITCH", */ "vcc9",
			 RK817_POWER_EN_REG(3), ENABLE_MASK(2), DISABLE_VAL(2)),
};
static_assert(ARRAY_SIZE(rk817_reg) == RK817_NUM_REGULATORS);

static struct rk_regulator_cfg rk818_reg[] = {
	{{
		/* .name = "DCDC_REG1", */
		.supply_name = "vcc1",
		.ops = &rk808_reg_ops,
		.min_uV = 712500,
		.uV_step = 12500,
		.n_voltages = 64,
		.vsel_reg = RK818_BUCK1_ON_VSEL_REG,
		.vsel_mask = RK818_BUCK_VSEL_MASK,
		.enable_reg = RK818_DCDC_EN_REG,
		.enable_mask = BIT(0),
	 }}, {{
		/* .name = "DCDC_REG2", */
		.supply_name = "vcc2",
		.ops = &rk808_reg_ops,
		.min_uV = 712500,
		.uV_step = 12500,
		.n_voltages = 64,
		.vsel_reg = RK818_BUCK2_ON_VSEL_REG,
		.vsel_mask = RK818_BUCK_VSEL_MASK,
		.enable_reg = RK818_DCDC_EN_REG,
		.enable_mask = BIT(1),
	 }}, {{
		/* .name = "DCDC_REG3", */
		.supply_name = "vcc3",
		.ops = &rk808_switch_ops,
		.n_voltages = 1,
		.enable_reg = RK818_DCDC_EN_REG,
		.enable_mask = BIT(2),
	}},
	RK8XX_DESC(/* "DCDC_REG4", */ "vcc4", 1800, 3600, 100,
		RK818_BUCK4_ON_VSEL_REG, RK818_BUCK4_VSEL_MASK,
		RK818_DCDC_EN_REG, BIT(3), 0),
	RK8XX_DESC(/* "DCDC_BOOST", */ "boost", 4700, 5400, 100,
		RK818_BOOST_LDO9_ON_VSEL_REG, RK818_BOOST_ON_VSEL_MASK,
		RK818_DCDC_EN_REG, BIT(4), 0),
	RK8XX_DESC(/* "LDO_REG1", */ "vcc6", 1800, 3400, 100,
		RK818_LDO1_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK818_LDO_EN_REG,
		BIT(0), 400),
	RK8XX_DESC(/* "LDO_REG2", */ "vcc6", 1800, 3400, 100,
		RK818_LDO2_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK818_LDO_EN_REG,
		BIT(1), 400),
	{{
		/* .name = "LDO_REG3", */
		.supply_name = "vcc7",
		.ops = &rk808_reg_ops_ranges,
		.n_voltages = 16,
		.linear_ranges = rk808_ldo3_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(rk808_ldo3_voltage_ranges),
		.vsel_reg = RK818_LDO3_ON_VSEL_REG,
		.vsel_mask = RK818_LDO3_ON_VSEL_MASK,
		.enable_reg = RK818_LDO_EN_REG,
		.enable_mask = BIT(2),
		.off_on_delay = 400,
	}},
	RK8XX_DESC(/* "LDO_REG4", */ "vcc8", 1800, 3400, 100,
		RK818_LDO4_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK818_LDO_EN_REG,
		BIT(3), 400),
	RK8XX_DESC(/* "LDO_REG5", */ "vcc7", 1800, 3400, 100,
		RK818_LDO5_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK818_LDO_EN_REG,
		BIT(4), 400),
	RK8XX_DESC(/* "LDO_REG6", */ "vcc8", 800, 2500, 100,
		RK818_LDO6_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK818_LDO_EN_REG,
		BIT(5), 400),
	RK8XX_DESC(/* "LDO_REG7", */ "vcc7", 800, 2500, 100,
		RK818_LDO7_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK818_LDO_EN_REG,
		BIT(6), 400),
	RK8XX_DESC(/* "LDO_REG8", */ "vcc8", 1800, 3400, 100,
		RK818_LDO8_ON_VSEL_REG, RK818_LDO_VSEL_MASK, RK818_LDO_EN_REG,
		BIT(7), 400),
	RK8XX_DESC(/* "LDO_REG9", */ "vcc9", 1800, 3400, 100,
		RK818_BOOST_LDO9_ON_VSEL_REG, RK818_LDO_VSEL_MASK,
		RK818_DCDC_EN_REG, BIT(5), 400),
	RK8XX_DESC_SWITCH(/* "SWITCH_REG", */ "vcc9",
			  RK818_DCDC_EN_REG, BIT(6)),
	RK8XX_DESC_SWITCH(/* "HDMI_SWITCH", */ "h_5v",
			  RK818_H5V_EN_REG, BIT(0)),
	RK8XX_DESC_SWITCH(/* "OTG_SWITCH", */ "usb",
			  RK818_DCDC_EN_REG, BIT(7)),
};
static_assert(ARRAY_SIZE(rk818_reg) == RK818_NUM_REGULATORS);

static int rk808_regulator_register(struct rk808 *rk808, int id,
				    struct of_regulator_match *match,
				    struct rk_regulator_cfg *cfg)
{
	struct device *dev = &rk808->i2c->dev;
	int ret;

	if (!match->of_node) {
		dev_dbg(dev, "Skip missing DTB regulator %s", match->name);
		return 0;
	}

	cfg->rdev.desc = &cfg->desc;
	cfg->rdev.dev = dev;
	cfg->rdev.regmap = rk808->regmap;

	ret = of_regulator_register(&cfg->rdev, match->of_node);
	if (ret)
		return dev_err_probe(dev, ret, "failed to register %s regulator\n",
				     match->name);

	dev_dbg(dev, "registered %s\n", match->name);

	return 0;
}

#define MATCH(variant, _name, _id) [RK##variant##_ID_##_id] = \
	{ .name = #_name, .desc = &rk##variant##_reg[RK##variant##_ID_##_id].desc }

static struct of_regulator_match rk805_reg_matches[] = {
	MATCH(805, DCDC_REG1, DCDC1),
	MATCH(805, DCDC_REG2, DCDC2),
	MATCH(805, DCDC_REG3, DCDC3),
	MATCH(805, DCDC_REG4, DCDC4),
	MATCH(805, LDO_REG1, LDO1),
	MATCH(805, LDO_REG2, LDO2),
	MATCH(805, LDO_REG3, LDO3),
};
static_assert(ARRAY_SIZE(rk805_reg_matches) == RK805_NUM_REGULATORS);

static struct of_regulator_match rk808_reg_matches[] = {
	MATCH(808, DCDC_REG1, DCDC1),
	MATCH(808, DCDC_REG2, DCDC2),
	MATCH(808, DCDC_REG3, DCDC3),
	MATCH(808, DCDC_REG4, DCDC4),
	MATCH(808, LDO_REG1, LDO1),
	MATCH(808, LDO_REG2, LDO2),
	MATCH(808, LDO_REG3, LDO3),
	MATCH(808, LDO_REG4, LDO4),
	MATCH(808, LDO_REG5, LDO5),
	MATCH(808, LDO_REG6, LDO6),
	MATCH(808, LDO_REG7, LDO7),
	MATCH(808, LDO_REG8, LDO8),
	MATCH(808, SWITCH_REG1, SWITCH1),
	MATCH(808, SWITCH_REG2, SWITCH2),
};
static_assert(ARRAY_SIZE(rk808_reg_matches) == RK808_NUM_REGULATORS);

static struct of_regulator_match rk809_reg_matches[] = {
	MATCH(809, DCDC_REG1, DCDC1),
	MATCH(809, DCDC_REG2, DCDC2),
	MATCH(809, DCDC_REG3, DCDC3),
	MATCH(809, DCDC_REG4, DCDC4),
	MATCH(809, LDO_REG1, LDO1),
	MATCH(809, LDO_REG2, LDO2),
	MATCH(809, LDO_REG3, LDO3),
	MATCH(809, LDO_REG4, LDO4),
	MATCH(809, LDO_REG5, LDO5),
	MATCH(809, LDO_REG6, LDO6),
	MATCH(809, LDO_REG7, LDO7),
	MATCH(809, LDO_REG8, LDO8),
	MATCH(809, LDO_REG9, LDO9),
	MATCH(809, DCDC_REG5, DCDC5),
	MATCH(809, SWITCH_REG1, SW1),
	MATCH(809, SWITCH_REG2, SW2),
};
static_assert(ARRAY_SIZE(rk809_reg_matches) == RK809_NUM_REGULATORS);

static struct of_regulator_match rk817_reg_matches[] = {
	MATCH(817, DCDC_REG1, DCDC1),
	MATCH(817, DCDC_REG2, DCDC2),
	MATCH(817, DCDC_REG3, DCDC3),
	MATCH(817, DCDC_REG4, DCDC4),
	MATCH(817, LDO_REG1, LDO1),
	MATCH(817, LDO_REG2, LDO2),
	MATCH(817, LDO_REG3, LDO3),
	MATCH(817, LDO_REG4, LDO4),
	MATCH(817, LDO_REG5, LDO5),
	MATCH(817, LDO_REG6, LDO6),
	MATCH(817, LDO_REG7, LDO7),
	MATCH(817, LDO_REG8, LDO8),
	MATCH(817, LDO_REG9, LDO9),
	MATCH(817, BOOST, BOOST),
	MATCH(817, OTG_SWITCH, BOOST_OTG_SW),
};
static_assert(ARRAY_SIZE(rk817_reg_matches) == RK817_NUM_REGULATORS);

static struct of_regulator_match rk818_reg_matches[] = {
	MATCH(818, DCDC_REG1, DCDC1),
	MATCH(818, DCDC_REG2, DCDC2),
	MATCH(818, DCDC_REG3, DCDC3),
	MATCH(818, DCDC_REG4, DCDC4),
	MATCH(818, DCDC_BOOST, BOOST),
	MATCH(818, LDO_REG1, LDO1),
	MATCH(818, LDO_REG2, LDO2),
	MATCH(818, LDO_REG3, LDO3),
	MATCH(818, LDO_REG4, LDO4),
	MATCH(818, LDO_REG5, LDO5),
	MATCH(818, LDO_REG6, LDO6),
	MATCH(818, LDO_REG7, LDO7),
	MATCH(818, LDO_REG8, LDO8),
	MATCH(818, LDO_REG9, LDO9),
	MATCH(818, SWITCH_REG, SWITCH),
	MATCH(818, HDMI_SWITCH, HDMI_SWITCH),
	MATCH(818, OTG_SWITCH, OTG_SWITCH),
};
static_assert(ARRAY_SIZE(rk818_reg_matches) == RK818_NUM_REGULATORS);

static int rk808_regulator_dt_parse(struct device *dev,
				    struct of_regulator_match *matches,
				    int nregulators)
{
	struct device_node *np = dev->of_node;

	np = of_get_child_by_name(np, "regulators");
	if (!np)
		return -ENOENT;

	return of_regulator_match(dev, np, matches, nregulators);
}

static int rk808_regulator_probe(struct device *dev)
{
	struct rk808 *rk808 = dev->parent->priv;
	struct rk_regulator_cfg *regulators;
	struct of_regulator_match *matches;
	int ret, i, nregulators;

	switch (rk808->variant) {
	case RK805_ID:
		regulators = rk805_reg;
		matches = rk805_reg_matches;
		nregulators = RK805_NUM_REGULATORS;
		break;
	case RK808_ID:
		regulators = rk808_reg;
		matches = rk808_reg_matches;
		nregulators = RK809_NUM_REGULATORS;
		break;
	case RK809_ID:
		regulators = rk809_reg;
		matches = rk809_reg_matches;
		nregulators = RK809_NUM_REGULATORS;
		break;
	case RK817_ID:
		regulators = rk817_reg;
		matches = rk817_reg_matches;
		nregulators = RK817_NUM_REGULATORS;
		break;
	case RK818_ID:
		regulators = rk818_reg;
		matches = rk818_reg_matches;
		nregulators = RK818_NUM_REGULATORS;
		break;
	default:
		dev_err(dev, "unsupported RK8XX ID %lu\n", rk808->variant);
		return -EINVAL;
	}

	ret = rk808_regulator_dt_parse(&rk808->i2c->dev, matches, nregulators);
	if (ret < 0)
		return ret;

	/* Instantiate the regulators */
	for (i = 0; i < nregulators; i++) {
		ret = rk808_regulator_register(rk808, i, &matches[i],
					       &regulators[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static struct driver rk808_regulator_driver = {
	.name = "rk808-regulator",
	.probe = rk808_regulator_probe,
};
device_platform_driver(rk808_regulator_driver);

MODULE_DESCRIPTION("regulator driver for the rk808 series PMICs");
MODULE_AUTHOR("Chris Zhong<zyw@rock-chips.com>");
MODULE_AUTHOR("Zhang Qing<zhangqing@rock-chips.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rk808-regulator");
