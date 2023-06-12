// SPDX-License-Identifier: GPL-2.0-only
/*
 * Rockchip IO Voltage Domain driver
 *
 * Copyright 2014 MundoReader S.L.
 * Copyright 2014 Google, Inc.
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <linux/err.h>
#include <mfd/syscon.h>
#include <regmap.h>
#include <regulator.h>

#define MAX_SUPPLIES		16

/*
 * The max voltage for 1.8V and 3.3V come from the Rockchip datasheet under
 * "Recommended Operating Conditions" for "Digital GPIO".   When the typical
 * is 3.3V the max is 3.6V.  When the typical is 1.8V the max is 1.98V.
 *
 * They are used like this:
 * - If the voltage on a rail is above the "1.8" voltage (1.98V) we'll tell the
 *   SoC we're at 3.3.
 * - If the voltage on a rail is above the "3.3" voltage (3.6V) we'll consider
 *   that to be an error.
 */
#define MAX_VOLTAGE_1_8		1980000
#define MAX_VOLTAGE_3_3		3600000

#define RK3568_PMU_GRF_IO_VSEL0 (0x0140)
#define RK3568_PMU_GRF_IO_VSEL1 (0x0144)
#define RK3568_PMU_GRF_IO_VSEL2 (0x0148)

struct rockchip_iodomain;

struct rockchip_iodomain_supply {
	struct rockchip_iodomain *iod;
	struct regulator *reg;
	int idx;
};

struct rockchip_iodomain_soc_data {
	int grf_offset;
	const char *supply_names[MAX_SUPPLIES];
	void (*init)(struct rockchip_iodomain *iod);
	int (*write)(struct rockchip_iodomain_supply *supply, int uV);
};

struct rockchip_iodomain {
	struct device *dev;
	struct regmap *grf;
	const struct rockchip_iodomain_soc_data *soc_data;
	struct rockchip_iodomain_supply supplies[MAX_SUPPLIES];
	int (*write)(struct rockchip_iodomain_supply *supply, int uV);
};

static int rk3568_iodomain_write(struct rockchip_iodomain_supply *supply,
				 int uV)
{
	struct rockchip_iodomain *iod = supply->iod;
	u32 is_3v3 = uV > MAX_VOLTAGE_1_8;
	u32 val0, val1;
	int b;

	dev_dbg(iod->dev, "set domain %d to %d uV\n", supply->idx, uV);

	switch (supply->idx) {
	case 0: /* pmuio1 */
		break;
	case 1: /* pmuio2 */
		b = supply->idx;
		val0 = BIT(16 + b) | (is_3v3 ? 0 : BIT(b));
		b = supply->idx + 4;
		val1 = BIT(16 + b) | (is_3v3 ? BIT(b) : 0);

		regmap_write(iod->grf, RK3568_PMU_GRF_IO_VSEL2, val0);
		regmap_write(iod->grf, RK3568_PMU_GRF_IO_VSEL2, val1);
		break;
	case 3: /* vccio2 */
		break;
	case 2: /* vccio1 */
	case 4: /* vccio3 */
	case 5: /* vccio4 */
	case 6: /* vccio5 */
	case 7: /* vccio6 */
	case 8: /* vccio7 */
		b = supply->idx - 1;
		val0 = BIT(16 + b) | (is_3v3 ? 0 : BIT(b));
		val1 = BIT(16 + b) | (is_3v3 ? BIT(b) : 0);

		regmap_write(iod->grf, RK3568_PMU_GRF_IO_VSEL0, val0);
		regmap_write(iod->grf, RK3568_PMU_GRF_IO_VSEL1, val1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct rockchip_iodomain_soc_data soc_data_rk3568_pmu = {
	.grf_offset = 0x140,
	.supply_names = {
		"pmuio1",
		"pmuio2",
		"vccio1",
		"vccio2",
		"vccio3",
		"vccio4",
		"vccio5",
		"vccio6",
		"vccio7",
	},
	.write = rk3568_iodomain_write,
};

static const struct of_device_id rockchip_iodomain_match[] = {
	{ .compatible = "rockchip,rk3568-pmu-io-voltage-domain",
	  .data = &soc_data_rk3568_pmu },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, rockchip_iodomain_match);

static int rockchip_iodomain_probe(struct device *dev)
{
	struct device_node *np = dev->of_node, *parent;
	struct rockchip_iodomain *iod;
	int i, ret = 0;

	if (!np)
		return -ENODEV;

	iod = xzalloc(sizeof(*iod));
	iod->dev = dev;
	iod->soc_data = device_get_match_data(dev);

	if (iod->soc_data->write)
		iod->write = iod->soc_data->write;

	parent = of_get_parent(np);
	if (parent) {
		iod->grf = syscon_node_to_regmap(parent);
	} else {
		dev_dbg(dev, "falling back to old binding\n");
		iod->grf = syscon_regmap_lookup_by_phandle(np, "rockchip,grf");
	}

	if (IS_ERR(iod->grf)) {
		dev_err(dev, "couldn't find grf regmap\n");
		return PTR_ERR(iod->grf);
	}

	for (i = 0; i < MAX_SUPPLIES; i++) {
		const char *supply_name = iod->soc_data->supply_names[i];
		struct rockchip_iodomain_supply *supply = &iod->supplies[i];
		struct regulator *reg;
		int uV;

		if (!supply_name)
			continue;

		reg = regulator_get(dev, supply_name);
		if (IS_ERR(reg)) {
			ret = PTR_ERR(reg);

			/* If a supply wasn't specified, that's OK */
			if (ret == -ENODEV)
				continue;
			else if (ret != -EPROBE_DEFER)
				dev_err(dev, "couldn't get regulator %s\n",
					supply_name);
			goto out;
		}

		/* set initial correct value */
		uV = regulator_get_voltage(reg);

		/* must be a regulator we can get the voltage of */
		if (uV < 0) {
			dev_err(dev, "Can't determine voltage: %s\n",
				supply_name);
			ret = uV;
			goto out;
		}

		if (uV > MAX_VOLTAGE_3_3) {
			dev_crit(dev, "%d uV is too high. May damage SoC!\n",
				 uV);
			ret = -EINVAL;
			goto out;
		}

		/* setup our supply */
		supply->idx = i;
		supply->iod = iod;
		supply->reg = reg;

		ret = iod->write(supply, uV);
		if (ret) {
			supply->reg = NULL;
			goto out;
		}
	}

	if (iod->soc_data->init)
		iod->soc_data->init(iod);

	ret = 0;
out:
	return ret;
}

static struct driver rockchip_iodomain_driver = {
	.name = "rockchip-iodomain",
	.probe = rockchip_iodomain_probe,
	.of_compatible = rockchip_iodomain_match,
};
coredevice_platform_driver(rockchip_iodomain_driver);
