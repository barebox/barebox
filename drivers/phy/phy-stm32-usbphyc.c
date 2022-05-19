// SPDX-License-Identifier: GPL-2.0-only
/*
 * STMicroelectronics STM32 USB PHY Controller driver
 *
 * Copyright (C) 2018 STMicroelectronics
 * Author(s): Amelie Delaunay <amelie.delaunay@st.com>.
 */
#include <common.h>
#include <init.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/iopoll.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/math64.h>
#include <usb/phy.h>

#define STM32_USBPHYC_PLL	0x0
#define STM32_USBPHYC_MISC	0x8
#define STM32_USBPHYC_MONITOR(X) (0x108 + ((X) * 0x100))
#define STM32_USBPHYC_VERSION	0x3F4

/* STM32_USBPHYC_PLL bit fields */
#define PLLNDIV			GENMASK(6, 0)
#define PLLFRACIN		GENMASK(25, 10)
#define PLLEN			BIT(26)
#define PLLSTRB			BIT(27)
#define PLLSTRBYP		BIT(28)
#define PLLFRACCTL		BIT(29)
#define PLLDITHEN0		BIT(30)
#define PLLDITHEN1		BIT(31)

/* STM32_USBPHYC_MISC bit fields */
#define SWITHOST		BIT(0)

/* STM32_USBPHYC_MONITOR bit fields */
#define STM32_USBPHYC_MON_OUT	GENMASK(3, 0)
#define STM32_USBPHYC_MON_SEL	GENMASK(8, 4)
#define STM32_USBPHYC_MON_SEL_LOCKP 0x1F
#define STM32_USBPHYC_MON_OUT_LOCKP BIT(3)

/* STM32_USBPHYC_VERSION bit fields */
#define MINREV			GENMASK(3, 0)
#define MAJREV			GENMASK(7, 4)

#define PLL_FVCO_MHZ		2880
#define PLL_INFF_MIN_RATE_HZ	19200000
#define PLL_INFF_MAX_RATE_HZ	38400000
#define HZ_PER_MHZ		1000000L

struct pll_params {
	u8 ndiv;
	u16 frac;
};

struct stm32_usbphyc_phy {
	struct phy *phy;
	struct stm32_usbphyc *usbphyc;
	u32 index;
	bool active;
};

struct stm32_usbphyc {
	struct device_d *dev;
	void __iomem *base;
	struct clk *clk;
	struct stm32_usbphyc_phy **phys;
	int nphys;
	struct regulator *vdda1v1;
	struct regulator *vdda1v8;
	int n_pll_cons;
	int switch_setup;
};

static inline void stm32_usbphyc_set_bits(void __iomem *reg, u32 bits)
{
	writel(readl(reg) | bits, reg);
}

static inline void stm32_usbphyc_clr_bits(void __iomem *reg, u32 bits)
{
	writel(readl(reg) & ~bits, reg);
}

static int stm32_usbphyc_regulators_enable(struct stm32_usbphyc *usbphyc)
{
	int ret;

	ret = regulator_enable(usbphyc->vdda1v1);
	if (ret)
		return ret;

	ret = regulator_enable(usbphyc->vdda1v8);
	if (ret)
		goto vdda1v1_disable;

	return 0;

vdda1v1_disable:
	regulator_disable(usbphyc->vdda1v1);

	return ret;
}

static int stm32_usbphyc_regulators_disable(struct stm32_usbphyc *usbphyc)
{
	int ret;

	ret = regulator_disable(usbphyc->vdda1v8);
	if (ret)
		return ret;

	ret = regulator_disable(usbphyc->vdda1v1);
	if (ret)
		return ret;

	return 0;
}

static void stm32_usbphyc_get_pll_params(u32 clk_rate,
					 struct pll_params *pll_params)
{
	unsigned long long fvco, ndiv, frac;

	/*    _
	 *   | FVCO = INFF*2*(NDIV + FRACT/2^16) when DITHER_DISABLE[1] = 1
	 *   | FVCO = 2880MHz
	 *  <
	 *   | NDIV = integer part of input bits to set the LDF
	 *   |_FRACT = fractional part of input bits to set the LDF
	 *  =>	PLLNDIV = integer part of (FVCO / (INFF*2))
	 *  =>	PLLFRACIN = fractional part of(FVCO / INFF*2) * 2^16
	 * <=>  PLLFRACIN = ((FVCO / (INFF*2)) - PLLNDIV) * 2^16
	 */
	fvco = (unsigned long long)PLL_FVCO_MHZ * HZ_PER_MHZ;

	ndiv = fvco;
	do_div(ndiv, (clk_rate * 2));
	pll_params->ndiv = (u8)ndiv;

	frac = fvco * (1 << 16);
	do_div(frac, (clk_rate * 2));
	frac = frac - (ndiv * (1 << 16));
	pll_params->frac = (u16)frac;
}

static int stm32_usbphyc_pll_init(struct stm32_usbphyc *usbphyc)
{
	struct pll_params pll_params;
	u32 clk_rate = clk_get_rate(usbphyc->clk);
	u32 ndiv, frac;
	u32 usbphyc_pll;

	if ((clk_rate < PLL_INFF_MIN_RATE_HZ) ||
	    (clk_rate > PLL_INFF_MAX_RATE_HZ)) {
		dev_err(usbphyc->dev, "input clk freq (%dHz) out of range\n",
			clk_rate);
		return -EINVAL;
	}

	stm32_usbphyc_get_pll_params(clk_rate, &pll_params);
	ndiv = FIELD_PREP(PLLNDIV, pll_params.ndiv);
	frac = FIELD_PREP(PLLFRACIN, pll_params.frac);

	usbphyc_pll = PLLDITHEN1 | PLLDITHEN0 | PLLSTRBYP | ndiv;

	if (pll_params.frac)
		usbphyc_pll |= PLLFRACCTL | frac;

	writel(usbphyc_pll, usbphyc->base + STM32_USBPHYC_PLL);

	dev_dbg(usbphyc->dev, "input clk freq=%dHz, ndiv=%lu, frac=%lu\n",
		clk_rate, FIELD_GET(PLLNDIV, usbphyc_pll),
		FIELD_GET(PLLFRACIN, usbphyc_pll));

	return 0;
}

static int __stm32_usbphyc_pll_disable(struct stm32_usbphyc *usbphyc)
{
	void __iomem *pll_reg = usbphyc->base + STM32_USBPHYC_PLL;
	u32 pllen;

	stm32_usbphyc_clr_bits(pll_reg, PLLEN);

	/* Wait for minimum width of powerdown pulse (ENABLE = Low) */
	if (readl_poll_timeout(pll_reg, pllen, !(pllen & PLLEN), 50))
		dev_err(usbphyc->dev, "PLL not reset\n");

	return stm32_usbphyc_regulators_disable(usbphyc);
}

static int stm32_usbphyc_pll_disable(struct stm32_usbphyc *usbphyc)
{
	/* Check if a phy port is still active or clk48 in use */
	if (--usbphyc->n_pll_cons > 0)
		return 0;

	return __stm32_usbphyc_pll_disable(usbphyc);
}

static int stm32_usbphyc_pll_enable(struct stm32_usbphyc *usbphyc)
{
	void __iomem *pll_reg = usbphyc->base + STM32_USBPHYC_PLL;
	bool pllen = readl(pll_reg) & PLLEN;
	int ret;

	/*
	 * Check if a phy port or clk48 prepare has configured the pll
	 * and ensure the PLL is enabled
	 */
	if (++usbphyc->n_pll_cons > 1 && pllen)
		return 0;

	if (pllen) {
		/*
		 * PLL shouldn't be enabled without known consumer,
		 * disable it and reinit n_pll_cons
		 */
		dev_warn(usbphyc->dev, "PLL enabled without known consumers\n");

		ret = __stm32_usbphyc_pll_disable(usbphyc);
		if (ret)
			return ret;
	}

	ret = stm32_usbphyc_regulators_enable(usbphyc);
	if (ret)
		goto dec_n_pll_cons;

	ret = stm32_usbphyc_pll_init(usbphyc);
	if (ret)
		goto reg_disable;

	stm32_usbphyc_set_bits(pll_reg, PLLEN);

	return 0;

reg_disable:
	stm32_usbphyc_regulators_disable(usbphyc);

dec_n_pll_cons:
	usbphyc->n_pll_cons--;

	return ret;
}

static int stm32_usbphyc_phy_init(struct phy *phy)
{
	struct stm32_usbphyc_phy *usbphyc_phy = phy_get_drvdata(phy);
	struct stm32_usbphyc *usbphyc = usbphyc_phy->usbphyc;
	u32 reg_mon = STM32_USBPHYC_MONITOR(usbphyc_phy->index);
	u32 monsel = FIELD_PREP(STM32_USBPHYC_MON_SEL,
				STM32_USBPHYC_MON_SEL_LOCKP);
	u32 monout;
	int ret;

	ret = stm32_usbphyc_pll_enable(usbphyc);
	if (ret)
		return ret;

	/* Check that PLL Lock input to PHY is High */
	writel(monsel, usbphyc->base + reg_mon);
	ret = readl_poll_timeout(usbphyc->base + reg_mon, monout,
				 (monout & STM32_USBPHYC_MON_OUT_LOCKP),
				 1000);
	if (ret) {
		dev_err(usbphyc->dev, "PLL Lock input to PHY is Low (val=%x)\n",
			(u32)(monout & STM32_USBPHYC_MON_OUT));
		goto pll_disable;
	}

	usbphyc_phy->active = true;

	return 0;

pll_disable:
	return stm32_usbphyc_pll_disable(usbphyc);
}

static int stm32_usbphyc_phy_exit(struct phy *phy)
{
	struct stm32_usbphyc_phy *usbphyc_phy = phy_get_drvdata(phy);
	struct stm32_usbphyc *usbphyc = usbphyc_phy->usbphyc;

	usbphyc_phy->active = false;

	return stm32_usbphyc_pll_disable(usbphyc);
}

static const struct phy_ops stm32_usbphyc_phy_ops = {
	.init = stm32_usbphyc_phy_init,
	.exit = stm32_usbphyc_phy_exit,
};

static void stm32_usbphyc_switch_setup(struct stm32_usbphyc *usbphyc,
				       u32 utmi_switch)
{
	if (!utmi_switch)
		stm32_usbphyc_clr_bits(usbphyc->base + STM32_USBPHYC_MISC,
				       SWITHOST);
	else
		stm32_usbphyc_set_bits(usbphyc->base + STM32_USBPHYC_MISC,
				       SWITHOST);
	usbphyc->switch_setup = utmi_switch;
}

static struct phy *stm32_usbphyc_of_xlate(struct device_d *dev,
					  struct of_phandle_args *args)
{
	struct stm32_usbphyc *usbphyc = dev->priv;
	struct stm32_usbphyc_phy *usbphyc_phy = NULL;
	struct device_node *phynode = args->np;
	int port = 0;

	for (port = 0; port < usbphyc->nphys; port++) {
		if (phynode == usbphyc->phys[port]->phy->dev.device_node) {
			usbphyc_phy = usbphyc->phys[port];
			break;
		}
	}

	if (!usbphyc_phy) {
		dev_err(dev, "failed to find phy\n");
		return ERR_PTR(-EINVAL);
	}

	if (((usbphyc_phy->index == 0) && (args->args_count != 0)) ||
	    ((usbphyc_phy->index == 1) && (args->args_count != 1))) {
		dev_err(dev, "invalid number of cells for phy port%d\n",
			usbphyc_phy->index);
		return ERR_PTR(-EINVAL);
	}

	/* Configure the UTMI switch for PHY port#2 */
	if (usbphyc_phy->index == 1) {
		if (usbphyc->switch_setup < 0) {
			stm32_usbphyc_switch_setup(usbphyc, args->args[0]);
		} else {
			if (args->args[0] != usbphyc->switch_setup) {
				dev_err(dev, "phy port1 already used\n");
				return ERR_PTR(-EBUSY);
			}
		}
	}

	return usbphyc_phy->phy;
}

static int stm32_usbphyc_probe(struct device_d *dev)
{
	struct stm32_usbphyc *usbphyc;
	struct device_node *child, *np = dev->device_node;
	struct resource *iores;
	struct phy_provider *phy_provider;
	u32 pllen, version;
	int ret, port = 0;

	usbphyc = xzalloc(sizeof(*usbphyc));

	usbphyc->dev = dev;
	dev->priv = usbphyc;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	usbphyc->base = IOMEM(iores->start);

	usbphyc->clk = clk_get(dev, 0);
	if (IS_ERR(usbphyc->clk)) {
		ret = PTR_ERR(usbphyc->clk);
		dev_err(dev, "clk get failed: %d\n", ret);
		goto release_region;
	}

	ret = clk_enable(usbphyc->clk);
	if (ret) {
		dev_err(dev, "clk enable failed: %d\n", ret);
		goto release_region;
	}

	ret = device_reset_us(dev, 2);
	if (ret == -EPROBE_DEFER)
		goto clk_disable;
	if (ret)
		stm32_usbphyc_clr_bits(usbphyc->base + STM32_USBPHYC_PLL, PLLEN);

	/*
	 * Wait for minimum width of powerdown pulse (ENABLE = Low):
	 * we have to ensure the PLL is disabled before phys initialization.
	 */
	if (readl_poll_timeout(usbphyc->base + STM32_USBPHYC_PLL,
			       pllen, !(pllen & PLLEN), 50)) {
		dev_warn(usbphyc->dev, "PLL not reset\n");
		ret = -EPROBE_DEFER;
		goto clk_disable;
	}

	usbphyc->switch_setup = -EINVAL;
	usbphyc->nphys = of_get_child_count(np);
	usbphyc->phys = calloc(usbphyc->nphys, sizeof(*usbphyc->phys));
	if (!usbphyc->phys) {
		ret = -ENOMEM;
		goto clk_disable;
	}

	usbphyc->vdda1v1 = regulator_get(dev, "vdda1v1");
	if (IS_ERR(usbphyc->vdda1v1)) {
		ret = PTR_ERR(usbphyc->vdda1v1);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get vdda1v1 supply: %d\n", ret);
		goto clk_disable;
	}

	usbphyc->vdda1v8 = regulator_get(dev, "vdda1v8");
	if (IS_ERR(usbphyc->vdda1v8)) {
		ret = PTR_ERR(usbphyc->vdda1v8);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get vdda1v8 supply: %d\n", ret);
		goto clk_disable;
	}

	for_each_child_of_node(np, child) {
		struct stm32_usbphyc_phy *usbphyc_phy;
		struct device_d *phydev;
		struct phy *phy;
		u32 index;

		phydev = of_platform_device_create(child, dev);
		if (!phydev)
			continue;

		of_platform_device_dummy_drv(phydev);

		phy = phy_create(phydev, child, &stm32_usbphyc_phy_ops);
		if (IS_ERR(phy)) {
			ret = PTR_ERR(phy);
			if (ret != -EPROBE_DEFER)
				dev_err(dev, "failed to create phy%d: %d\n",
					port, ret);
			goto clk_disable;
		}

		usbphyc_phy = xzalloc(sizeof(*usbphyc_phy));

		ret = of_property_read_u32(child, "reg", &index);
		if (ret || index > usbphyc->nphys) {
			dev_err(&phy->dev, "invalid reg property: %d\n", ret);
			goto clk_disable;
		}

		usbphyc->phys[port] = usbphyc_phy;
		phy_set_bus_width(phy, 8);
		phy_set_drvdata(phy, usbphyc_phy);

		usbphyc->phys[port]->phy = phy;
		usbphyc->phys[port]->usbphyc = usbphyc;
		usbphyc->phys[port]->index = index;
		usbphyc->phys[port]->active = false;

		port++;
	}

	phy_provider = of_phy_provider_register(dev, stm32_usbphyc_of_xlate);
	if (IS_ERR(phy_provider)) {
		ret = PTR_ERR(phy_provider);
		dev_err(dev, "failed to register phy provider: %d\n", ret);
		goto clk_disable;
	}

	version = readl(usbphyc->base + STM32_USBPHYC_VERSION);
	dev_info(dev, "registered rev: %lu.%lu\n",
		 FIELD_GET(MAJREV, version), FIELD_GET(MINREV, version));

	return 0;

clk_disable:
	clk_disable(usbphyc->clk);
release_region:
	release_region(iores);

	free(usbphyc->phys);
	free(usbphyc);

	return ret;
}

static void stm32_usbphyc_remove(struct device_d *dev)
{
	struct stm32_usbphyc *usbphyc = dev->priv;
	int port;

	/* Ensure PHYs are not active, to allow PLL disabling */
	for (port = 0; port < usbphyc->nphys; port++)
		if (usbphyc->phys[port]->active)
			stm32_usbphyc_phy_exit(usbphyc->phys[port]->phy);

	clk_disable(usbphyc->clk);
}

static const struct of_device_id stm32_usbphyc_of_match[] = {
	{ .compatible = "st,stm32mp1-usbphyc", },
	{ /* sentinel */ },
};

static struct driver_d stm32_usbphyc_driver = {
	.name = "stm32-usbphyc",
	.probe = stm32_usbphyc_probe,
	.remove = stm32_usbphyc_remove,
	.of_compatible = stm32_usbphyc_of_match,
};
device_platform_driver(stm32_usbphyc_driver);

MODULE_DESCRIPTION("STMicroelectronics STM32 USBPHYC driver");
MODULE_AUTHOR("Amelie Delaunay <amelie.delaunay@st.com>");
MODULE_LICENSE("GPL v2");
