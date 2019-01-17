#include <common.h>
#include <regmap.h>
#include <regulator.h>

/**
 * regulator_is_enabled_regmap - standard is_enabled() for regmap users
 *
 * @rdev: regulator to operate on
 *
 * Regulators that use regmap for their register I/O can set the
 * enable_reg and enable_mask fields in their descriptor and then use
 * this as their is_enabled operation, saving some code.
 */
int regulator_is_enabled_regmap(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, rdev->desc->enable_reg, &val);
	if (ret != 0)
		return ret;

	val &= rdev->desc->enable_mask;

	if (rdev->desc->enable_is_inverted) {
		if (rdev->desc->enable_val)
			return val != rdev->desc->enable_val;
		return val == 0;
	} else {
		if (rdev->desc->enable_val)
			return val == rdev->desc->enable_val;
		return val != 0;
	}
}
EXPORT_SYMBOL_GPL(regulator_is_enabled_regmap);

/**
 * regulator_enable_regmap - standard enable() for regmap users
 *
 * @rdev: regulator to operate on
 *
 * Regulators that use regmap for their register I/O can set the
 * enable_reg and enable_mask fields in their descriptor and then use
 * this as their enable() operation, saving some code.
 */
int regulator_enable_regmap(struct regulator_dev *rdev)
{
	unsigned int val;

	if (rdev->desc->enable_is_inverted) {
		val = rdev->desc->disable_val;
	} else {
		val = rdev->desc->enable_val;
		if (!val)
			val = rdev->desc->enable_mask;
	}

	return regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				  rdev->desc->enable_mask, val);
}
EXPORT_SYMBOL_GPL(regulator_enable_regmap);

/**
 * regulator_disable_regmap - standard disable() for regmap users
 *
 * @rdev: regulator to operate on
 *
 * Regulators that use regmap for their register I/O can set the
 * enable_reg and enable_mask fields in their descriptor and then use
 * this as their disable() operation, saving some code.
 */
int regulator_disable_regmap(struct regulator_dev *rdev)
{
	unsigned int val;

	if (rdev->desc->enable_is_inverted) {
		val = rdev->desc->enable_val;
		if (!val)
			val = rdev->desc->enable_mask;
	} else {
		val = rdev->desc->disable_val;
	}

	return regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				  rdev->desc->enable_mask, val);
}
EXPORT_SYMBOL_GPL(regulator_disable_regmap);

/**
 * regulator_set_voltage_sel_regmap - standard set_voltage_sel for regmap users
 *
 * @rdev: regulator to operate on
 * @sel: Selector to set
 *
 * Regulators that use regmap for their register I/O can set the
 * vsel_reg and vsel_mask fields in their descriptor and then use this
 * as their set_voltage_vsel operation, saving some code.
 */
int regulator_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	int ret;

	sel <<= ffs(rdev->desc->vsel_mask) - 1;

	ret = regmap_update_bits(rdev->regmap, rdev->desc->vsel_reg,
				  rdev->desc->vsel_mask, sel);
	if (ret)
		return ret;

	if (rdev->desc->apply_bit)
		ret = regmap_update_bits(rdev->regmap, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
}
EXPORT_SYMBOL_GPL(regulator_set_voltage_sel_regmap);

/**
 * regulator_map_voltage_linear - map_voltage() for simple linear mappings
 *
 * @rdev: Regulator to operate on
 * @min_uV: Lower bound for voltage
 * @max_uV: Upper bound for voltage
 *
 * Drivers providing min_uV and uV_step in their regulator_desc can
 * use this as their map_voltage() operation.
 */
int regulator_map_voltage_linear(struct regulator_dev *rdev,
				 int min_uV, int max_uV)
{
	int ret, voltage;

	/* Allow uV_step to be 0 for fixed voltage */
	if (rdev->desc->n_voltages == 1 && rdev->desc->uV_step == 0) {
		if (min_uV <= rdev->desc->min_uV && rdev->desc->min_uV <= max_uV)
			return 0;
		else
			return -EINVAL;
	}

	if (!rdev->desc->uV_step) {
		BUG_ON(!rdev->desc->uV_step);
		return -EINVAL;
	}

	if (min_uV < rdev->desc->min_uV)
		min_uV = rdev->desc->min_uV;

	ret = DIV_ROUND_UP(min_uV - rdev->desc->min_uV, rdev->desc->uV_step);
	if (ret < 0)
		return ret;

	ret += rdev->desc->linear_min_sel;

	/* Map back into a voltage to verify we're still in bounds */
	voltage = rdev->desc->ops->list_voltage(rdev, ret);
	if (voltage < min_uV || voltage > max_uV)
		return -EINVAL;

	return ret;
}
EXPORT_SYMBOL_GPL(regulator_map_voltage_linear);

/**
 * regulator_list_voltage_linear - List voltages with simple calculation
 *
 * @rdev: Regulator device
 * @selector: Selector to convert into a voltage
 *
 * Regulators with a simple linear mapping between voltages and
 * selectors can set min_uV and uV_step in the regulator descriptor
 * and then use this function as their list_voltage() operation,
 */
int regulator_list_voltage_linear(struct regulator_dev *rdev,
				  unsigned int selector)
{
	if (selector >= rdev->desc->n_voltages)
		return -EINVAL;
	if (selector < rdev->desc->linear_min_sel)
		return 0;

	selector -= rdev->desc->linear_min_sel;

	return rdev->desc->min_uV + (rdev->desc->uV_step * selector);
}
EXPORT_SYMBOL_GPL(regulator_list_voltage_linear);
