// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2017 Impinj, Inc
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * Based on the code of analogus driver:
 *
 * Copyright 2015-2017 Pengutronix, Lucas Stach <kernel@pengutronix.de>
 */

#include <of_device.h>
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

#include <dt-bindings/power/imx8mq-power.h>

#define GPC_LPCR_A_BSC			0x000

#define GPC_PGC_CPU_MAPPING		0x0ec

#define IMX7_USB_HSIC_PHY_A_DOMAIN	BIT(6)
#define IMX7_USB_OTG2_PHY_A_DOMAIN	BIT(5)
#define IMX7_USB_OTG1_PHY_A_DOMAIN	BIT(4)
#define IMX7_PCIE_PHY_A_DOMAIN		BIT(3)
#define IMX7_MIPI_PHY_A_DOMAIN		BIT(2)

#define IMX8M_PCIE2_A53_DOMAIN		BIT(15)
#define IMX8M_MIPI_CSI2_A53_DOMAIN	BIT(14)
#define IMX8M_MIPI_CSI1_A53_DOMAIN	BIT(13)
#define IMX8M_DISP_A53_DOMAIN		BIT(12)
#define IMX8M_HDMI_A53_DOMAIN		BIT(11)
#define IMX8M_VPU_A53_DOMAIN		BIT(10)
#define IMX8M_GPU_A53_DOMAIN		BIT(9)
#define IMX8M_DDR2_A53_DOMAIN		BIT(8)
#define IMX8M_DDR1_A53_DOMAIN		BIT(7)
#define IMX8M_OTG2_A53_DOMAIN		BIT(5)
#define IMX8M_OTG1_A53_DOMAIN		BIT(4)
#define IMX8M_PCIE1_A53_DOMAIN		BIT(3)
#define IMX8M_MIPI_A53_DOMAIN		BIT(2)

#define GPC_PU_PGC_SW_PUP_REQ		0x0f8
#define GPC_PU_PGC_SW_PDN_REQ		0x104

#define IMX7_USB_HSIC_PHY_SW_Pxx_REQ	BIT(4)
#define IMX7_USB_OTG2_PHY_SW_Pxx_REQ	BIT(3)
#define IMX7_USB_OTG1_PHY_SW_Pxx_REQ	BIT(2)
#define IMX7_PCIE_PHY_SW_Pxx_REQ	BIT(1)
#define IMX7_MIPI_PHY_SW_Pxx_REQ	BIT(0)

#define IMX8M_PCIE2_SW_Pxx_REQ		BIT(13)
#define IMX8M_MIPI_CSI2_SW_Pxx_REQ	BIT(12)
#define IMX8M_MIPI_CSI1_SW_Pxx_REQ	BIT(11)
#define IMX8M_DISP_SW_Pxx_REQ		BIT(10)
#define IMX8M_HDMI_SW_Pxx_REQ		BIT(9)
#define IMX8M_VPU_SW_Pxx_REQ		BIT(8)
#define IMX8M_GPU_SW_Pxx_REQ		BIT(7)
#define IMX8M_DDR2_SW_Pxx_REQ		BIT(6)
#define IMX8M_DDR1_SW_Pxx_REQ		BIT(5)
#define IMX8M_OTG2_SW_Pxx_REQ		BIT(3)
#define IMX8M_OTG1_SW_Pxx_REQ		BIT(2)
#define IMX8M_PCIE1_SW_Pxx_REQ		BIT(1)
#define IMX8M_MIPI_SW_Pxx_REQ		BIT(0)

#define GPC_M4_PU_PDN_FLG		0x1bc

/*
 * The PGC offset values in Reference Manual
 * (Rev. 1, 01/2018 and the older ones) GPC chapter's
 * GPC_PGC memory map are incorrect, below offset
 * values are from design RTL.
 */
#define IMX7_PGC_MIPI                       16
#define IMX7_PGC_PCIE                       17
#define IMX7_PGC_USB_HSIC                   20


#define IMX8M_PGC_MIPI			16
#define IMX8M_PGC_PCIE1		17
#define IMX8M_PGC_OTG1			18
#define IMX8M_PGC_OTG2			19
#define IMX8M_PGC_DDR1			21
#define IMX8M_PGC_GPU			23
#define IMX8M_PGC_VPU			24
#define IMX8M_PGC_DISP			26
#define IMX8M_PGC_MIPI_CSI1		27
#define IMX8M_PGC_MIPI_CSI2		28
#define IMX8M_PGC_PCIE2		29

#define GPC_PGC_CTRL(n)			(0x800 + (n) * 0x40)
#define GPC_PGC_SR(n)			(GPC_PGC_CTRL(n) + 0xc)

#define GPC_PGC_CTRL_PCR		BIT(0)

struct imx_pgc_domain {
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

struct imx_pgc_domain_data {
       const struct imx_pgc_domain *domains;
       size_t domains_num;
};

static int imx_gpc_pu_pgc_sw_pxx_req(struct generic_pm_domain *genpd,
				      bool on)
{
	struct imx_pgc_domain *domain = container_of(genpd,
						     struct imx_pgc_domain,
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

static int imx_gpc_pu_pgc_sw_pup_req(struct generic_pm_domain *genpd)
{
	return imx_gpc_pu_pgc_sw_pxx_req(genpd, true);
}

static int imx_gpc_pu_pgc_sw_pdn_req(struct generic_pm_domain *genpd)
{
	return imx_gpc_pu_pgc_sw_pxx_req(genpd, false);
}

static const struct imx_pgc_domain imx7_pgc_domains[] = {
	[IMX7_POWER_DOMAIN_MIPI_PHY] = {
		.genpd = {
			.name      = "mipi-phy",
		},
		.bits  = {
			.pxx = IMX7_MIPI_PHY_SW_Pxx_REQ,
			.map = IMX7_MIPI_PHY_A_DOMAIN,
		},
		.voltage   = 1000000,
		.pgc	   = IMX7_PGC_MIPI,
	},

	[IMX7_POWER_DOMAIN_PCIE_PHY] = {
		.genpd = {
			.name      = "pcie-phy",
		},
		.bits  = {
			.pxx = IMX7_PCIE_PHY_SW_Pxx_REQ,
			.map = IMX7_PCIE_PHY_A_DOMAIN,
		},
		.voltage   = 1000000,
		.pgc	   = IMX7_PGC_PCIE,
	},

	[IMX7_POWER_DOMAIN_USB_HSIC_PHY] = {
		.genpd = {
			.name      = "usb-hsic-phy",
		},
		.bits  = {
			.pxx = IMX7_USB_HSIC_PHY_SW_Pxx_REQ,
			.map = IMX7_USB_HSIC_PHY_A_DOMAIN,
	},
		.voltage   = 1200000,
		.pgc	   = IMX7_PGC_USB_HSIC,
	},
};

static const struct imx_pgc_domain_data imx7_pgc_domain_data = {
       .domains = imx7_pgc_domains,
       .domains_num = ARRAY_SIZE(imx7_pgc_domains),
};

static const struct imx_pgc_domain imx8m_pgc_domains[] = {
	[IMX8M_POWER_DOMAIN_MIPI] = {
		.genpd = {
			.name      = "mipi",
		},
		.bits  = {
			.pxx = IMX8M_MIPI_SW_Pxx_REQ,
			.map = IMX8M_MIPI_A53_DOMAIN,
		},
		.pgc	   = IMX8M_PGC_MIPI,
	},

	[IMX8M_POWER_DOMAIN_PCIE1] = {
		.genpd = {
			.name = "pcie1",
		},
		.bits  = {
			.pxx = IMX8M_PCIE1_SW_Pxx_REQ,
			.map = IMX8M_PCIE1_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_PCIE1,
	},

	[IMX8M_POWER_DOMAIN_USB_OTG1] = {
		.genpd = {
			.name = "usb-otg1",
		},
		.bits  = {
			.pxx = IMX8M_OTG1_SW_Pxx_REQ,
			.map = IMX8M_OTG1_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_OTG1,
	},

	[IMX8M_POWER_DOMAIN_USB_OTG2] = {
		.genpd = {
			.name = "usb-otg2",
		},
		.bits  = {
			.pxx = IMX8M_OTG2_SW_Pxx_REQ,
			.map = IMX8M_OTG2_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_OTG2,
	},

	[IMX8M_POWER_DOMAIN_DDR1] = {
		.genpd = {
			.name = "ddr1",
		},
		.bits  = {
			.pxx = IMX8M_DDR1_SW_Pxx_REQ,
			.map = IMX8M_DDR2_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_DDR1,
	},

	[IMX8M_POWER_DOMAIN_GPU] = {
		.genpd = {
			.name = "gpu",
		},
		.bits  = {
			.pxx = IMX8M_GPU_SW_Pxx_REQ,
			.map = IMX8M_GPU_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_GPU,
	},

	[IMX8M_POWER_DOMAIN_VPU] = {
		.genpd = {
			.name = "vpu",
		},
		.bits  = {
			.pxx = IMX8M_VPU_SW_Pxx_REQ,
			.map = IMX8M_VPU_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_VPU,
	},

	[IMX8M_POWER_DOMAIN_DISP] = {
		.genpd = {
			.name = "disp",
		},
		.bits  = {
			.pxx = IMX8M_DISP_SW_Pxx_REQ,
			.map = IMX8M_DISP_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_DISP,
	},

	[IMX8M_POWER_DOMAIN_MIPI_CSI1] = {
		.genpd = {
			.name = "mipi-csi1",
		},
		.bits  = {
			.pxx = IMX8M_MIPI_CSI1_SW_Pxx_REQ,
			.map = IMX8M_MIPI_CSI1_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_MIPI_CSI1,
	},

	[IMX8M_POWER_DOMAIN_MIPI_CSI2] = {
		.genpd = {
			.name = "mipi-csi2",
		},
		.bits  = {
			.pxx = IMX8M_MIPI_CSI2_SW_Pxx_REQ,
			.map = IMX8M_MIPI_CSI2_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_MIPI_CSI2,
	},

	[IMX8M_POWER_DOMAIN_PCIE2] = {
		.genpd = {
			.name = "pcie2",
		},
		.bits  = {
			.pxx = IMX8M_PCIE2_SW_Pxx_REQ,
			.map = IMX8M_PCIE2_A53_DOMAIN,
		},
		.pgc   = IMX8M_PGC_PCIE2,
	},
};

static const struct imx_pgc_domain_data imx8m_pgc_domain_data = {
	.domains = imx8m_pgc_domains,
	.domains_num = ARRAY_SIZE(imx8m_pgc_domains),
};

static int imx_pgc_domain_probe(struct device_d *dev)
{
	struct imx_pgc_domain *domain = dev->priv;
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

static const struct platform_device_id imx_pgc_domain_id[] = {
	{ "imx-pgc-domain", },
	{ },
};

static struct driver_d imx_pgc_domain_driver = {
	.name = "imx-pgc",
	.probe = imx_pgc_domain_probe,
	.id_table = imx_pgc_domain_id,
};
coredevice_platform_driver(imx_pgc_domain_driver);

static int imx_gpcv2_probe(struct device_d *dev)
{
	static const struct imx_pgc_domain_data *domain_data;
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

	domain_data = of_device_get_match_data(dev);

	for_each_child_of_node(pgc_np, np) {
		struct device_d *pd_dev;
		struct imx_pgc_domain *domain;
		u32 domain_index;
		ret = of_property_read_u32(np, "reg", &domain_index);
		if (ret) {
			dev_err(dev, "Failed to read 'reg' property\n");
			return ret;
		}

		if (domain_index >= domain_data->domains_num) {
			dev_warn(dev,
				 "Domain index %d is out of bounds\n",
				 domain_index);
			continue;
		}

		domain = xmemdup(&domain_data->domains[domain_index],
				 sizeof(domain_data->domains[domain_index]));
		domain->base = base;
		domain->genpd.power_on = imx_gpc_pu_pgc_sw_pup_req;
		domain->genpd.power_off = imx_gpc_pu_pgc_sw_pdn_req;

		pd_dev = xzalloc(sizeof(*pd_dev));
		pd_dev->device_node = np;
		pd_dev->id = domain_index;
		pd_dev->parent = dev;
		pd_dev->priv = domain;
		pd_dev->device_node = np;
		dev_set_name(pd_dev, imx_pgc_domain_id[0].name);

		ret = platform_device_register(pd_dev);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct of_device_id imx_gpcv2_dt_ids[] = {
	{ .compatible = "fsl,imx7d-gpc", .data = &imx7_pgc_domain_data },
	{ .compatible = "fsl,imx8mq-gpc", .data = &imx8m_pgc_domain_data, },
	{ }
};

static struct driver_d imx_gpcv2_driver = {
	.name = "imx7d-gpc",
	.probe = imx_gpcv2_probe,
	.of_compatible = DRV_OF_COMPAT(imx_gpcv2_dt_ids),
};
coredevice_platform_driver(imx_gpcv2_driver);
