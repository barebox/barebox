// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

#include <common.h>
#include <init.h>
#include <mfd/syscon.h>
#include <regmap.h>
#include <regulator.h>

struct anatop_regulator {
	u32 control_reg;
	struct regmap *anatop;
	int vol_bit_shift;
	int vol_bit_width;
	u32 delay_reg;
	int delay_bit_shift;
	int delay_bit_width;
	int min_bit_val;
	int min_voltage;
	int max_voltage;

	struct regulator_dev  rdev;
	struct regulator_desc rdesc;

	bool bypass;
	int sel;
};

static struct regulator_ops anatop_rops = {
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.list_voltage = regulator_list_voltage_linear,
};

static int anatop_regulator_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct device_node *anatop_np;
	struct regulator_desc *rdesc;
	struct regulator_dev *rdev;
	struct anatop_regulator *sreg;
	int ret = 0;

	sreg  = xzalloc(sizeof(*sreg));
	rdesc = &sreg->rdesc;
	rdev  = &sreg->rdev;

	anatop_np = of_get_parent(np);
	if (!anatop_np)
		return -ENODEV;

	rdev->desc = rdesc;
	rdev->regmap = syscon_node_to_regmap(anatop_np);
	rdev->dev = dev;
	if (IS_ERR(rdev->regmap))
		return PTR_ERR(rdev->regmap);

	ret = of_property_read_u32(np, "anatop-reg-offset",
				   &sreg->control_reg);
	if (ret) {
		dev_err(dev, "no anatop-reg-offset property set\n");
		return ret;
	}
	ret = of_property_read_u32(np, "anatop-vol-bit-width",
				   &sreg->vol_bit_width);
	if (ret) {
		dev_err(dev, "no anatop-vol-bit-width property set\n");
		return ret;
	}
	ret = of_property_read_u32(np, "anatop-vol-bit-shift",
				   &sreg->vol_bit_shift);
	if (ret) {
		dev_err(dev, "no anatop-vol-bit-shift property set\n");
		return ret;
	}
	ret = of_property_read_u32(np, "anatop-min-bit-val",
				   &sreg->min_bit_val);
	if (ret) {
		dev_err(dev, "no anatop-min-bit-val property set\n");
		return ret;
	}
	ret = of_property_read_u32(np, "anatop-min-voltage",
				   &sreg->min_voltage);
	if (ret) {
		dev_err(dev, "no anatop-min-voltage property set\n");
		return ret;
	}
	ret = of_property_read_u32(np, "anatop-max-voltage",
				   &sreg->max_voltage);
	if (ret) {
		dev_err(dev, "no anatop-max-voltage property set\n");
		return ret;
	}

	/* read LDO ramp up setting, only for core reg */
	of_property_read_u32(np, "anatop-delay-reg-offset",
			     &sreg->delay_reg);
	of_property_read_u32(np, "anatop-delay-bit-width",
			     &sreg->delay_bit_width);
	of_property_read_u32(np, "anatop-delay-bit-shift",
			     &sreg->delay_bit_shift);

	rdesc->n_voltages = (sreg->max_voltage - sreg->min_voltage) / 25000 + 1
			    + sreg->min_bit_val;
	rdesc->min_uV = sreg->min_voltage;
	rdesc->uV_step = 25000;
	rdesc->linear_min_sel = sreg->min_bit_val;
	rdesc->vsel_reg  = sreg->control_reg;
	rdesc->vsel_mask = GENMASK(sreg->vol_bit_width + sreg->vol_bit_shift,
				   sreg->vol_bit_shift);

	/* Only core regulators have the ramp up delay configuration. */
	if (sreg->control_reg && sreg->delay_bit_width) {
		free(sreg);
		/* FIXME: This case is not supported */
		return 0;
	} else {
		u32 enable_bit;

		rdesc->ops = &anatop_rops;

		if (!of_property_read_u32(np, "anatop-enable-bit",
					  &enable_bit)) {
			anatop_rops.enable  = regulator_enable_regmap;
			anatop_rops.disable = regulator_disable_regmap;
			anatop_rops.is_enabled = regulator_is_enabled_regmap;

			rdesc->enable_reg = sreg->control_reg;
			rdesc->enable_mask = BIT(enable_bit);
		}
	}

	return of_regulator_register(rdev, dev->of_node);
}

static const struct of_device_id of_anatop_regulator_match_tbl[] = {
	{ .compatible = "fsl,anatop-regulator", },
	{ /* end */ }
};
MODULE_DEVICE_TABLE(of, of_anatop_regulator_match_tbl);

static struct driver anatop_regulator_driver = {
	.name = "anatop_regulator",
	.probe = anatop_regulator_probe,
	.of_compatible = DRV_OF_COMPAT(of_anatop_regulator_match_tbl),
};
device_platform_driver(anatop_regulator_driver);

