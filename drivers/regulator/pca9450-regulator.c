// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/regulator/pca9450-regulator.c?id=fbef4191b4961c125585c715407e693f7d0024a9

/*
 * Copyright 2020 NXP.
 * NXP PCA9450 pmic driver
 */

#include <i2c/i2c.h>
#include <mfd/pca9450.h>
#include <regulator.h>

static const struct regulator_ops pca9450_dvs_buck_regulator_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.list_voltage = regulator_list_voltage_linear_range,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
};

static const struct regulator_ops pca9450_buck_regulator_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.list_voltage = regulator_list_voltage_linear_range,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
};

static const struct regulator_ops pca9450_ldo_regulator_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.list_voltage = regulator_list_voltage_linear_range,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
};

static unsigned int pca9450_ldo5_get_reg_voltage_sel(struct regulator_dev *rdev)
{
	struct pca9450 *pca9450 = dev_get_drvdata(rdev->dev);

	if (pca9450->sd_vsel_fixed_low)
		return PCA9450_LDO5CTRL_L;

	if (pca9450->sd_vsel_gpio && !gpiod_get_value(pca9450->sd_vsel_gpio))
		return PCA9450_LDO5CTRL_L;

	return rdev->desc->vsel_reg;
}

static int pca9450_ldo5_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, pca9450_ldo5_get_reg_voltage_sel(rdev), &val);
	if (ret != 0)
		return ret;

	val &= rdev->desc->vsel_mask;
	val >>= ffs(rdev->desc->vsel_mask) - 1;

	return val;
}

static int pca9450_ldo5_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned int sel)
{
	int ret;

	sel <<= ffs(rdev->desc->vsel_mask) - 1;

	ret = regmap_update_bits(rdev->regmap, pca9450_ldo5_get_reg_voltage_sel(rdev),
				  rdev->desc->vsel_mask, sel);
	if (ret)
		return ret;

	if (rdev->desc->apply_bit)
		ret = regmap_update_bits(rdev->regmap, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
}

static const struct regulator_ops pca9450_ldo5_regulator_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.list_voltage = regulator_list_voltage_linear_range,
	.set_voltage_sel = pca9450_ldo5_set_voltage_sel_regmap,
	.get_voltage_sel = pca9450_ldo5_get_voltage_sel_regmap,
};

/*
 * BUCK1/2/3
 * 0.60 to 2.1875V (12.5mV step)
 */
static const struct regulator_linear_range pca9450_dvs_buck_volts[] = {
	REGULATOR_LINEAR_RANGE(600000,  0x00, 0x7F, 12500),
};

/*
 * BUCK1/3
 * 0.65 to 2.2375V (12.5mV step)
 */
static const struct regulator_linear_range pca9451a_dvs_buck_volts[] = {
	REGULATOR_LINEAR_RANGE(650000, 0x00, 0x7F, 12500),
};

/*
 * BUCK4/5/6
 * 0.6V to 3.4V (25mV step)
 */
static const struct regulator_linear_range pca9450_buck_volts[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x00, 0x70, 25000),
	REGULATOR_LINEAR_RANGE(3400000, 0x71, 0x7F, 0),
};

/*
 * LDO1
 * 1.6 to 3.3V ()
 */
static const struct regulator_linear_range pca9450_ldo1_volts[] = {
	REGULATOR_LINEAR_RANGE(1600000, 0x00, 0x03, 100000),
	REGULATOR_LINEAR_RANGE(3000000, 0x04, 0x07, 100000),
};

/*
 * LDO2
 * 0.8 to 1.15V (50mV step)
 */
static const struct regulator_linear_range pca9450_ldo2_volts[] = {
	REGULATOR_LINEAR_RANGE(800000, 0x00, 0x07, 50000),
};

/*
 * LDO3/4
 * 0.8 to 3.3V (100mV step)
 */
static const struct regulator_linear_range pca9450_ldo34_volts[] = {
	REGULATOR_LINEAR_RANGE(800000, 0x00, 0x19, 100000),
	REGULATOR_LINEAR_RANGE(3300000, 0x1A, 0x1F, 0),
};

/*
 * LDO5
 * 1.8 to 3.3V (100mV step)
 */
static const struct regulator_linear_range pca9450_ldo5_volts[] = {
	REGULATOR_LINEAR_RANGE(1800000,  0x00, 0x0F, 100000),
};

static struct regulator_desc pca9450a_regulators[] = {
	{
		.name = "buck1",
		.supply_name = "inb13",
		.of_match = of_match_ptr("BUCK1"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK1,
		.ops = &pca9450_dvs_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK1_VOLTAGE_NUM,
		.linear_ranges = pca9450_dvs_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_dvs_buck_volts),
		.vsel_reg = PCA9450_BUCK1OUT_DVS0,
		.vsel_mask = BUCK1OUT_DVS0_MASK,
		.enable_reg = PCA9450_BUCK1CTRL,
		.enable_mask = BUCK1_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck2",
		.supply_name = "inb26",
		.of_match = of_match_ptr("BUCK2"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK2,
		.ops = &pca9450_dvs_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK2_VOLTAGE_NUM,
		.linear_ranges = pca9450_dvs_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_dvs_buck_volts),
		.vsel_reg = PCA9450_BUCK2OUT_DVS0,
		.vsel_mask = BUCK2OUT_DVS0_MASK,
		.enable_reg = PCA9450_BUCK2CTRL,
		.enable_mask = BUCK2_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ_STBYREQ,
	},
	{
		.name = "buck3",
		.supply_name = "inb13",
		.of_match = of_match_ptr("BUCK3"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK3,
		.ops = &pca9450_dvs_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK3_VOLTAGE_NUM,
		.linear_ranges = pca9450_dvs_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_dvs_buck_volts),
		.vsel_reg = PCA9450_BUCK3OUT_DVS0,
		.vsel_mask = BUCK3OUT_DVS0_MASK,
		.enable_reg = PCA9450_BUCK3CTRL,
		.enable_mask = BUCK3_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck4",
		.supply_name = "inb45",
		.of_match = of_match_ptr("BUCK4"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK4,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK4_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK4OUT,
		.vsel_mask = BUCK4OUT_MASK,
		.enable_reg = PCA9450_BUCK4CTRL,
		.enable_mask = BUCK4_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck5",
		.supply_name = "inb45",
		.of_match = of_match_ptr("BUCK5"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK5,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK5_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK5OUT,
		.vsel_mask = BUCK5OUT_MASK,
		.enable_reg = PCA9450_BUCK5CTRL,
		.enable_mask = BUCK5_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck6",
		.supply_name = "inb26",
		.of_match = of_match_ptr("BUCK6"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK6,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK6_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK6OUT,
		.vsel_mask = BUCK6OUT_MASK,
		.enable_reg = PCA9450_BUCK6CTRL,
		.enable_mask = BUCK6_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "ldo1",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO1"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO1,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO1_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo1_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo1_volts),
		.vsel_reg = PCA9450_LDO1CTRL,
		.vsel_mask = LDO1OUT_MASK,
		.enable_reg = PCA9450_LDO1CTRL,
		.enable_mask = LDO1_EN_MASK,
	},
	{
		.name = "ldo2",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO2"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO2,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO2_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo2_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo2_volts),
		.vsel_reg = PCA9450_LDO2CTRL,
		.vsel_mask = LDO2OUT_MASK,
		.enable_reg = PCA9450_LDO2CTRL,
		.enable_mask = LDO2_EN_MASK,
	},
	{
		.name = "ldo3",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO3"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO3,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO3_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo34_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo34_volts),
		.vsel_reg = PCA9450_LDO3CTRL,
		.vsel_mask = LDO3OUT_MASK,
		.enable_reg = PCA9450_LDO3CTRL,
		.enable_mask = LDO3_EN_MASK,
	},
	{
		.name = "ldo4",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO4"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO4,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO4_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo34_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo34_volts),
		.vsel_reg = PCA9450_LDO4CTRL,
		.vsel_mask = LDO4OUT_MASK,
		.enable_reg = PCA9450_LDO4CTRL,
		.enable_mask = LDO4_EN_MASK,
	},
	{
		.name = "ldo5",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO5"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO5,
		.ops = &pca9450_ldo5_regulator_ops,
		.n_voltages = PCA9450_LDO5_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo5_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo5_volts),
		.vsel_reg = PCA9450_LDO5CTRL_H,
		.vsel_mask = LDO5HOUT_MASK,
		.enable_reg = PCA9450_LDO5CTRL_L,
		.enable_mask = LDO5H_EN_MASK,
	},
};

/*
 * Buck3 removed on PCA9450B and connected with Buck1 internal for dual phase
 * on PCA9450C as no Buck3.
 */
static struct regulator_desc pca9450bc_regulators[] = {
	{
		.name = "buck1",
		.supply_name = "inb13",
		.of_match = of_match_ptr("BUCK1"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK1,
		.ops = &pca9450_dvs_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK1_VOLTAGE_NUM,
		.linear_ranges = pca9450_dvs_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_dvs_buck_volts),
		.vsel_reg = PCA9450_BUCK1OUT_DVS0,
		.vsel_mask = BUCK1OUT_DVS0_MASK,
		.enable_reg = PCA9450_BUCK1CTRL,
		.enable_mask = BUCK1_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck2",
		.supply_name = "inb26",
		.of_match = of_match_ptr("BUCK2"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK2,
		.ops = &pca9450_dvs_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK2_VOLTAGE_NUM,
		.linear_ranges = pca9450_dvs_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_dvs_buck_volts),
		.vsel_reg = PCA9450_BUCK2OUT_DVS0,
		.vsel_mask = BUCK2OUT_DVS0_MASK,
		.enable_reg = PCA9450_BUCK2CTRL,
		.enable_mask = BUCK2_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ_STBYREQ,
	},
	{
		.name = "buck4",
		.supply_name = "inb45",
		.of_match = of_match_ptr("BUCK4"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK4,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK4_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK4OUT,
		.vsel_mask = BUCK4OUT_MASK,
		.enable_reg = PCA9450_BUCK4CTRL,
		.enable_mask = BUCK4_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck5",
		.supply_name = "inb45",
		.of_match = of_match_ptr("BUCK5"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK5,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK5_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK5OUT,
		.vsel_mask = BUCK5OUT_MASK,
		.enable_reg = PCA9450_BUCK5CTRL,
		.enable_mask = BUCK5_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck6",
		.supply_name = "inb26",
		.of_match = of_match_ptr("BUCK6"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK6,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK6_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK6OUT,
		.vsel_mask = BUCK6OUT_MASK,
		.enable_reg = PCA9450_BUCK6CTRL,
		.enable_mask = BUCK6_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "ldo1",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO1"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO1,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO1_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo1_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo1_volts),
		.vsel_reg = PCA9450_LDO1CTRL,
		.vsel_mask = LDO1OUT_MASK,
		.enable_reg = PCA9450_LDO1CTRL,
		.enable_mask = LDO1_EN_MASK,
	},
	{
		.name = "ldo2",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO2"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO2,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO2_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo2_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo2_volts),
		.vsel_reg = PCA9450_LDO2CTRL,
		.vsel_mask = LDO2OUT_MASK,
		.enable_reg = PCA9450_LDO2CTRL,
		.enable_mask = LDO2_EN_MASK,
	},
	{
		.name = "ldo3",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO3"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO3,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO3_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo34_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo34_volts),
		.vsel_reg = PCA9450_LDO3CTRL,
		.vsel_mask = LDO3OUT_MASK,
		.enable_reg = PCA9450_LDO3CTRL,
		.enable_mask = LDO3_EN_MASK,
	},
	{
		.name = "ldo4",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO4"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO4,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO4_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo34_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo34_volts),
		.vsel_reg = PCA9450_LDO4CTRL,
		.vsel_mask = LDO4OUT_MASK,
		.enable_reg = PCA9450_LDO4CTRL,
		.enable_mask = LDO4_EN_MASK,
	},
	{
		.name = "ldo5",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO5"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO5,
		.ops = &pca9450_ldo5_regulator_ops,
		.n_voltages = PCA9450_LDO5_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo5_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo5_volts),
		.vsel_reg = PCA9450_LDO5CTRL_H,
		.vsel_mask = LDO5HOUT_MASK,
		.enable_reg = PCA9450_LDO5CTRL_L,
		.enable_mask = LDO5H_EN_MASK,
	},
};

static struct regulator_desc pca9451a_regulators[] = {
	{
		.name = "buck1",
		.supply_name = "inb13",
		.of_match = of_match_ptr("BUCK1"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK1,
		.ops = &pca9450_dvs_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK1_VOLTAGE_NUM,
		.linear_ranges = pca9451a_dvs_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9451a_dvs_buck_volts),
		.vsel_reg = PCA9450_BUCK1OUT_DVS0,
		.vsel_mask = BUCK1OUT_DVS0_MASK,
		.enable_reg = PCA9450_BUCK1CTRL,
		.enable_mask = BUCK1_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck2",
		.supply_name = "inb26",
		.of_match = of_match_ptr("BUCK2"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK2,
		.ops = &pca9450_dvs_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK2_VOLTAGE_NUM,
		.linear_ranges = pca9450_dvs_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_dvs_buck_volts),
		.vsel_reg = PCA9450_BUCK2OUT_DVS0,
		.vsel_mask = BUCK2OUT_DVS0_MASK,
		.enable_reg = PCA9450_BUCK2CTRL,
		.enable_mask = BUCK2_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ_STBYREQ,
	},
	{
		.name = "buck4",
		.supply_name = "inb45",
		.of_match = of_match_ptr("BUCK4"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK4,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK4_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK4OUT,
		.vsel_mask = BUCK4OUT_MASK,
		.enable_reg = PCA9450_BUCK4CTRL,
		.enable_mask = BUCK4_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck5",
		.supply_name = "inb45",
		.of_match = of_match_ptr("BUCK5"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK5,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK5_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK5OUT,
		.vsel_mask = BUCK5OUT_MASK,
		.enable_reg = PCA9450_BUCK5CTRL,
		.enable_mask = BUCK5_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "buck6",
		.supply_name = "inb26",
		.of_match = of_match_ptr("BUCK6"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_BUCK6,
		.ops = &pca9450_buck_regulator_ops,
		.n_voltages = PCA9450_BUCK6_VOLTAGE_NUM,
		.linear_ranges = pca9450_buck_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_buck_volts),
		.vsel_reg = PCA9450_BUCK6OUT,
		.vsel_mask = BUCK6OUT_MASK,
		.enable_reg = PCA9450_BUCK6CTRL,
		.enable_mask = BUCK6_ENMODE_MASK,
		.enable_val = BUCK_ENMODE_ONREQ,
	},
	{
		.name = "ldo1",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO1"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO1,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO1_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo1_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo1_volts),
		.vsel_reg = PCA9450_LDO1CTRL,
		.vsel_mask = LDO1OUT_MASK,
		.enable_reg = PCA9450_LDO1CTRL,
		.enable_mask = LDO1_EN_MASK,
	},
	{
		.name = "ldo3",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO3"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO3,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO3_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo34_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo34_volts),
		.vsel_reg = PCA9450_LDO3CTRL,
		.vsel_mask = LDO3OUT_MASK,
		.enable_reg = PCA9450_LDO3CTRL,
		.enable_mask = LDO3_EN_MASK,
	},
	{
		.name = "ldo4",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO4"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO4,
		.ops = &pca9450_ldo_regulator_ops,
		.n_voltages = PCA9450_LDO4_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo34_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo34_volts),
		.vsel_reg = PCA9450_LDO4CTRL,
		.vsel_mask = LDO4OUT_MASK,
		.enable_reg = PCA9450_LDO4CTRL,
		.enable_mask = LDO4_EN_MASK,
	},
	{
		.name = "ldo5",
		.supply_name = "inl1",
		.of_match = of_match_ptr("LDO5"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PCA9450_LDO5,
		.ops = &pca9450_ldo5_regulator_ops,
		.n_voltages = PCA9450_LDO5_VOLTAGE_NUM,
		.linear_ranges = pca9450_ldo5_volts,
		.n_linear_ranges = ARRAY_SIZE(pca9450_ldo5_volts),
		.vsel_reg = PCA9450_LDO5CTRL_H,
		.vsel_mask = LDO5HOUT_MASK,
		.enable_reg = PCA9450_LDO5CTRL_L,
		.enable_mask = LDO5H_EN_MASK,
	},
};

static int pca9450_of_init(struct pca9450 *pca9450)
{
	struct i2c_client *i2c = container_of(pca9450->dev, struct i2c_client, dev);
	int ret;
	unsigned int val;
	unsigned int rstb_deb_ctrl;
	unsigned int t_on_deb, t_off_deb;
	unsigned int t_on_step, t_off_step;
	unsigned int t_restart;

	ret = of_property_read_u32(i2c->dev.of_node, "npx,pmic-rst-b-debounce-ms", &val);
	if (ret == -EINVAL)
		rstb_deb_ctrl = T_PMIC_RST_DEB_50MS;
	else if (ret)
		return ret;
	else {
		switch (val) {
		case 10: rstb_deb_ctrl = T_PMIC_RST_DEB_10MS; break;
		case 50: rstb_deb_ctrl = T_PMIC_RST_DEB_50MS; break;
		case 100: rstb_deb_ctrl = T_PMIC_RST_DEB_100MS; break;
		case 500: rstb_deb_ctrl = T_PMIC_RST_DEB_500MS; break;
		case 1000: rstb_deb_ctrl = T_PMIC_RST_DEB_1S; break;
		case 2000: rstb_deb_ctrl = T_PMIC_RST_DEB_2S; break;
		case 4000: rstb_deb_ctrl = T_PMIC_RST_DEB_4S; break;
		case 8000: rstb_deb_ctrl = T_PMIC_RST_DEB_8S; break;
		default: return -EINVAL;
		}
	}
	ret = regmap_update_bits(pca9450->regmap, PCA9450_RESET_CTRL,
				 T_PMIC_RST_DEB_MASK, rstb_deb_ctrl);
	if (ret)
		return dev_err_probe(&i2c->dev, ret, "Failed to set PMIC_RST_B debounce time\n");

	ret = of_property_read_u32(i2c->dev.of_node, "nxp,pmic-on-req-on-debounce-us", &val);
	if (ret == -EINVAL)
		t_on_deb = T_ON_DEB_20MS;
	else if (ret)
		return ret;
	else {
		switch (val) {
		case 120: t_on_deb = T_ON_DEB_120US; break;
		case 20000: t_on_deb = T_ON_DEB_20MS; break;
		case 100000: t_on_deb = T_ON_DEB_100MS; break;
		case 750000: t_on_deb = T_ON_DEB_750MS; break;
		default: return -EINVAL;
		}
	}

	ret = of_property_read_u32(i2c->dev.of_node, "nxp,pmic-on-req-off-debounce-us", &val);
	if (ret == -EINVAL)
		t_off_deb = pca9450->default_t_off_deb;
	else if (ret)
		return ret;
	else {
		switch (val) {
		case 120: t_off_deb = T_OFF_DEB_120US; break;
		case 2000: t_off_deb = T_OFF_DEB_2MS; break;
		default: return -EINVAL;
		}
	}

	ret = of_property_read_u32(i2c->dev.of_node, "nxp,power-on-step-ms", &val);
	if (ret == -EINVAL)
		t_on_step = T_ON_STEP_2MS;
	else if (ret)
		return ret;
	else {
		switch (val) {
		case 1: t_on_step = T_ON_STEP_1MS; break;
		case 2: t_on_step = T_ON_STEP_2MS; break;
		case 4: t_on_step = T_ON_STEP_4MS; break;
		case 8: t_on_step = T_ON_STEP_8MS; break;
		default: return -EINVAL;
		}
	}

	ret = of_property_read_u32(i2c->dev.of_node, "nxp,power-down-step-ms", &val);
	if (ret == -EINVAL)
		t_off_step = T_OFF_STEP_8MS;
	else if (ret)
		return ret;
	else {
		switch (val) {
		case 2: t_off_step = T_OFF_STEP_2MS; break;
		case 4: t_off_step = T_OFF_STEP_4MS; break;
		case 8: t_off_step = T_OFF_STEP_8MS; break;
		case 16: t_off_step = T_OFF_STEP_16MS; break;
		default: return -EINVAL;
		}
	}

	ret = of_property_read_u32(i2c->dev.of_node, "nxp,restart-ms", &val);
	if (ret == -EINVAL)
		t_restart = T_RESTART_250MS;
	else if (ret)
		return ret;
	else {
		switch (val) {
		case 250: t_restart = T_RESTART_250MS; break;
		case 500: t_restart = T_RESTART_500MS; break;
		default: return -EINVAL;
		}
	}

	ret = regmap_update_bits(pca9450->regmap, PCA9450_PWR_CTRL,
				 T_ON_DEB_MASK | T_OFF_DEB_MASK | T_ON_STEP_MASK |
				 T_OFF_STEP_MASK | T_RESTART_MASK,
				 t_on_deb | t_off_deb | t_on_step |
				 t_off_step | t_restart);
	if (ret)
		return dev_err_probe(&i2c->dev, ret,
				     "Failed to set PWR_CTRL debounce configuration\n");

	if (of_property_read_bool(i2c->dev.of_node, "nxp,i2c-lt-enable")) {
		/* Enable I2C Level Translator */
		ret = regmap_update_bits(pca9450->regmap, PCA9450_CONFIG2,
					 I2C_LT_MASK, I2C_LT_ON_STANDBY_RUN);
		if (ret)
			return dev_err_probe(&i2c->dev, ret,
					     "Failed to enable I2C level translator\n");
	}

	return 0;
}

static int pca9450_regulator_probe(struct device *dev)
{
	struct pca9450 *pca9450 = dev->parent->priv;
	enum pca9450_chip_type type = pca9450->type;
	const struct regulator_desc *regulator_desc;
	struct regulator_config config = { };
	struct regulator_dev *ldo5 = NULL;
	unsigned int device_id, i;
	const char *type_name;
	int ret;

	switch (type) {
	case PCA9450_TYPE_PCA9450A:
		regulator_desc = pca9450a_regulators;
		pca9450->rcnt = ARRAY_SIZE(pca9450a_regulators);
		pca9450->default_t_off_deb = T_OFF_DEB_120US;
		type_name = "pca9450a";
		break;
	case PCA9450_TYPE_PCA9450BC:
		regulator_desc = pca9450bc_regulators;
		pca9450->rcnt = ARRAY_SIZE(pca9450bc_regulators);
		pca9450->default_t_off_deb = T_OFF_DEB_120US;
		type_name = "pca9450bc";
		break;
	case PCA9450_TYPE_PCA9451A:
		regulator_desc = pca9451a_regulators;
		pca9450->rcnt = ARRAY_SIZE(pca9451a_regulators);
		pca9450->default_t_off_deb = T_OFF_DEB_2MS;
		type_name = "pca9451a";
		break;
	case PCA9450_TYPE_PCA9452:
		regulator_desc = pca9451a_regulators;
		pca9450->rcnt = ARRAY_SIZE(pca9451a_regulators);
		pca9450->default_t_off_deb = T_OFF_DEB_2MS;
		type_name = "pca9452";
		break;
	default:
		dev_err(pca9450->dev, "Unknown device type");
		return -EINVAL;
	}

	ret = regmap_read(pca9450->regmap, PCA9450_REG_DEV_ID, &device_id);
	if (ret)
		return dev_err_probe(pca9450->dev, ret, "Read device id error\n");

	/* Check your board and dts for match the right pmic */
	if (((device_id >> 4) != 0x1 && type == PCA9450_TYPE_PCA9450A) ||
	    ((device_id >> 4) != 0x3 && type == PCA9450_TYPE_PCA9450BC) ||
	    ((device_id >> 4) != 0x9 && type == PCA9450_TYPE_PCA9451A) ||
	    ((device_id >> 4) != 0x9 && type == PCA9450_TYPE_PCA9452))
		return dev_err_probe(pca9450->dev, -EINVAL,
				     "Device id(%x) mismatched\n", device_id >> 4);

	for (i = 0; i < pca9450->rcnt; i++) {
		const struct regulator_desc *desc = &regulator_desc[i];
		struct regulator_dev *rdev;

		if (type == PCA9450_TYPE_PCA9451A && !strcmp(desc->name, "ldo3"))
			continue;

		config.regmap = pca9450->regmap;
		config.dev = pca9450->dev;
		config.driver_data = pca9450;

		rdev = regulator_register(pca9450->dev, desc, &config);
		if (!rdev)
			return dev_err_probe(pca9450->dev, -EINVAL,
					     "Failed to register regulator(%s)\n", desc->name);

		if (!strcmp(desc->name, "ldo5"))
			ldo5 = rdev;
	}

	/* Clear PRESET_EN bit in BUCK123_DVS to use DVS registers */
	ret = regmap_clear_bits(pca9450->regmap, PCA9450_BUCK123_DVS,
				BUCK123_PRESET_EN);
	if (ret)
		return dev_err_probe(pca9450->dev, ret,  "Failed to clear PRESET_EN bit\n");

	ret = pca9450_of_init(pca9450);
	if (ret)
		return dev_err_probe(pca9450->dev, ret, "Unable to parse OF data\n");

	/*
	 * For LDO5 we need to be able to check the status of the SD_VSEL input in
	 * order to know which control register is used. Most boards connect SD_VSEL
	 * to the VSELECT signal, so we can use the GPIO that is internally routed
	 * to this signal (if SION bit is set in IOMUX).
	 */
	pca9450->sd_vsel_gpio = dev_gpiod_get_index(ldo5->dev, ldo5->node,
						    "sd-vsel", 0, GPIOD_IN, NULL);
	if (gpiod_not_found(pca9450->sd_vsel_gpio))
		pca9450->sd_vsel_gpio = NULL;
	else if (IS_ERR(pca9450->sd_vsel_gpio))
		return dev_err_probe(pca9450->dev, PTR_ERR(pca9450->sd_vsel_gpio),
				     "Failed to get SD_VSEL GPIO\n");

	pca9450->sd_vsel_fixed_low =
		of_property_read_bool(ldo5->dev->of_node, "nxp,sd-vsel-fixed-low");

	dev_info(pca9450->dev, "%s probed.\n", type_name);

	return 0;
}

static struct driver pca9450_regulator_driver = {
	.name = "pca9450-regulator",
	.probe = pca9450_regulator_probe,
};
coredevice_platform_driver(pca9450_regulator_driver);

MODULE_AUTHOR("Robin Gong <yibin.gong@nxp.com>");
MODULE_DESCRIPTION("NXP PCA9450 Power Management IC driver");
MODULE_LICENSE("GPL");
