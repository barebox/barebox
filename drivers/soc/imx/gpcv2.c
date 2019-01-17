/*
 * Copyright 2017 Impinj, Inc
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * Based on the code of analogus driver:
 *
 * Copyright 2015-2017 Pengutronix, Lucas Stach <kernel@pengutronix.de>
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <clock.h>
#include <abort.h>
#include <malloc.h>
#include <io.h>
#include <init.h>
#include <linux/iopoll.h>

#include <pm_domain.h>
#include <regulator.h>
#include <dt-bindings/power/imx7-power.h>

#define GPC_LPCR_A7_BSC			0x000

#define GPC_PGC_CPU_MAPPING		0x0ec
#define USB_HSIC_PHY_A7_DOMAIN		BIT(6)
#define USB_OTG2_PHY_A7_DOMAIN		BIT(5)
#define USB_OTG1_PHY_A7_DOMAIN		BIT(4)
#define PCIE_PHY_A7_DOMAIN		BIT(3)
#define MIPI_PHY_A7_DOMAIN		BIT(2)

#define GPC_PU_PGC_SW_PUP_REQ		0x0f8
#define GPC_PU_PGC_SW_PDN_REQ		0x104
#define USB_HSIC_PHY_SW_Pxx_REQ		BIT(4)
#define USB_OTG2_PHY_SW_Pxx_REQ		BIT(3)
#define USB_OTG1_PHY_SW_Pxx_REQ		BIT(2)
#define PCIE_PHY_SW_Pxx_REQ		BIT(1)
#define MIPI_PHY_SW_Pxx_REQ		BIT(0)

#define GPC_M4_PU_PDN_FLG		0x1bc

/*
 * The PGC offset values in Reference Manual
 * (Rev. 1, 01/2018 and the older ones) GPC chapter's
 * GPC_PGC memory map are incorrect, below offset
 * values are from design RTL.
 */
#define PGC_MIPI                       16
#define PGC_PCIE                       17
#define PGC_USB_HSIC                   20
#define GPC_PGC_CTRL(n)			(0x800 + (n) * 0x40)
#define GPC_PGC_SR(n)			(GPC_PGC_CTRL(n) + 0xc)

#define GPC_PGC_CTRL_PCR		BIT(0)

struct imx7_pgc_domain {
	struct generic_pm_domain genpd;
	void __iomem *base;
	struct regulator *regulator;

	unsigned int pgc;

	const struct {
		u32 pxx;
		u32 map;
	} bits;

	const int voltage;
	struct device_d *dev;
};

static int imx7_gpc_pu_pgc_sw_pxx_req(struct generic_pm_domain *genpd,
				      bool on)
{
	struct imx7_pgc_domain *domain = container_of(genpd,
						      struct imx7_pgc_domain,
						      genpd);
	unsigned int offset = on ?
		GPC_PU_PGC_SW_PUP_REQ : GPC_PU_PGC_SW_PDN_REQ;
	const bool enable_power_control = !on;
	const bool has_regulator = !IS_ERR(domain->regulator);
	int ret = 0;
	unsigned int mapping, ctrl = 0, pxx;

	mapping = readl(domain->base + GPC_PGC_CPU_MAPPING);
	mapping |= domain->bits.map;
	writel(mapping, domain->base + GPC_PGC_CPU_MAPPING);

	if (has_regulator && on) {
		ret = regulator_enable(domain->regulator);
		if (ret) {
			dev_err(domain->dev, "failed to enable regulator\n");
			goto unmap;
		}
	}

	if (enable_power_control) {
		ctrl = readl(domain->base + GPC_PGC_CTRL(domain->pgc));
		ctrl |= GPC_PGC_CTRL_PCR;
		writel(ctrl, domain->base + GPC_PGC_CTRL(domain->pgc));
	}

	pxx = readl(domain->base + offset);
	pxx |= domain->bits.pxx;
	writel(pxx, domain->base + offset);

	/*
	 * As per "5.5.9.4 Example Code 4" in IMX7DRM.pdf wait
	 * for PUP_REQ/PDN_REQ bit to be cleared
	 */
	ret = readl_poll_timeout(domain->base + offset, pxx,
				 !(pxx & domain->bits.pxx), MSECOND);
	if (ret < 0) {
		dev_err(domain->dev, "falied to command PGC\n");
		/*
		 * If we were in a process of enabling a
		 * domain and failed we might as well disable
		 * the regulator we just enabled. And if it
		 * was the opposite situation and we failed to
		 * power down -- keep the regulator on
		 */
		on = !on;
	}

	if (enable_power_control) {
		ctrl &= ~GPC_PGC_CTRL_PCR;
		writel(ctrl, domain->base + GPC_PGC_CTRL(domain->pgc));
	}

	if (has_regulator && !on) {
		int err;

		err = regulator_disable(domain->regulator);
		if (err)
			dev_err(domain->dev,
				"failed to disable regulator: %d\n", ret);
		/* Preserve earlier error code */
		ret = ret ?: err;
	}
unmap:
	mapping &= ~domain->bits.map;
	writel(mapping, domain->base + GPC_PGC_CPU_MAPPING);

	return ret;
}

static int imx7_gpc_pu_pgc_sw_pup_req(struct generic_pm_domain *genpd)
{
	return imx7_gpc_pu_pgc_sw_pxx_req(genpd, true);
}

static int imx7_gpc_pu_pgc_sw_pdn_req(struct generic_pm_domain *genpd)
{
	return imx7_gpc_pu_pgc_sw_pxx_req(genpd, false);
}

static const struct imx7_pgc_domain imx7_pgc_domains[] = {
	[IMX7_POWER_DOMAIN_MIPI_PHY] = {
		.genpd = {
			.name      = "mipi-phy",
		},
		.bits  = {
			.pxx = MIPI_PHY_SW_Pxx_REQ,
			.map = MIPI_PHY_A7_DOMAIN,
		},
		.voltage   = 1000000,
		.pgc	   = PGC_MIPI,
	},

	[IMX7_POWER_DOMAIN_PCIE_PHY] = {
		.genpd = {
			.name      = "pcie-phy",
		},
		.bits  = {
			.pxx = PCIE_PHY_SW_Pxx_REQ,
			.map = PCIE_PHY_A7_DOMAIN,
		},
		.voltage   = 1000000,
		.pgc	   = PGC_PCIE,
	},

	[IMX7_POWER_DOMAIN_USB_HSIC_PHY] = {
		.genpd = {
			.name      = "usb-hsic-phy",
		},
		.bits  = {
			.pxx = USB_HSIC_PHY_SW_Pxx_REQ,
			.map = USB_HSIC_PHY_A7_DOMAIN,
	},
		.voltage   = 1200000,
		.pgc	   = PGC_USB_HSIC,
	},
};

static int imx7_pgc_domain_probe(struct device_d *dev)
{
	struct imx7_pgc_domain *domain = dev->priv;
	int ret;

	domain->dev = dev;

	domain->regulator = regulator_get(domain->dev, "power");
	if (IS_ERR(domain->regulator)) {
		if (PTR_ERR(domain->regulator) != -ENODEV) {
			if (PTR_ERR(domain->regulator) != -EPROBE_DEFER)
				dev_err(domain->dev, "Failed to get domain's regulator\n");
			return PTR_ERR(domain->regulator);
		}
	} else {
		regulator_set_voltage(domain->regulator,
				      domain->voltage, domain->voltage);
	}

	ret = pm_genpd_init(&domain->genpd, NULL, true);
	if (ret) {
		dev_err(domain->dev, "Failed to init power domain\n");
		return ret;
	}

	ret = of_genpd_add_provider_simple(domain->dev->device_node,
					   &domain->genpd);
	if (ret) {
		dev_err(domain->dev, "Failed to add genpd provider\n");
	}

	return ret;
}

static const struct platform_device_id imx7_pgc_domain_id[] = {
	{ "imx7-pgc-domain", },
	{ },
};

static struct driver_d imx7_pgc_domain_driver = {
	.name = "imx-pgc",
	.probe = imx7_pgc_domain_probe,
	.id_table = imx7_pgc_domain_id,
};
coredevice_platform_driver(imx7_pgc_domain_driver);

static int imx_gpcv2_probe(struct device_d *dev)
{
	struct device_node *pgc_np, *np;
	struct resource *res;
	void __iomem *base;
	int ret;

	pgc_np = of_get_child_by_name(dev->device_node, "pgc");
	if (!pgc_np) {
		dev_err(dev, "No power domains specified in DT\n");
		return -EINVAL;
	}

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	base = IOMEM(res->start);

	for_each_child_of_node(pgc_np, np) {
		struct device_d *pd_dev;
		struct imx7_pgc_domain *domain;
		u32 domain_index;
		ret = of_property_read_u32(np, "reg", &domain_index);
		if (ret) {
			dev_err(dev, "Failed to read 'reg' property\n");
			return ret;
		}

		if (domain_index >= ARRAY_SIZE(imx7_pgc_domains)) {
			dev_warn(dev,
				 "Domain index %d is out of bounds\n",
				 domain_index);
			continue;
		}

		domain = xmemdup(&imx7_pgc_domains[domain_index],
				 sizeof(imx7_pgc_domains[domain_index]));
		domain->base = base;
		domain->genpd.power_on = imx7_gpc_pu_pgc_sw_pup_req;
		domain->genpd.power_off = imx7_gpc_pu_pgc_sw_pdn_req;

		pd_dev = xzalloc(sizeof(*pd_dev));
		pd_dev->device_node = np;
		pd_dev->id = domain_index;
		pd_dev->parent = dev;
		pd_dev->priv = domain;
		pd_dev->device_node = np;
		dev_set_name(pd_dev, imx7_pgc_domain_id[0].name);

		ret = platform_device_register(pd_dev);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct of_device_id imx_gpcv2_dt_ids[] = {
	{ .compatible = "fsl,imx7d-gpc" },
	{ }
};

static struct driver_d imx_gpcv2_driver = {
	.name = "imx7d-gpc",
	.probe = imx_gpcv2_probe,
	.of_compatible = DRV_OF_COMPAT(imx_gpcv2_dt_ids),
};
coredevice_platform_driver(imx_gpcv2_driver);
