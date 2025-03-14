// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments power domain driver
 *
 * Copyright (C) 2020-2021 Texas Instruments Incorporated - https://www.ti.com/
 *	Tero Kristo <t-kristo@ti.com>
 */

#define pr_fmt(fmt)     "ti-k3-pm-domain: " fmt

#include <io.h>
#include <stdio.h>
#include <of_device.h>
#include <malloc.h>
#include <init.h>
#include <pm_domain.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/device.h>

#define PSC_PTCMD		0x120
#define PSC_PTCMD_H		0x124
#define PSC_PTSTAT		0x128
#define PSC_PTSTAT_H		0x12C
#define PSC_PDSTAT		0x200
#define PSC_PDCTL		0x300
#define PSC_MDSTAT		0x800
#define PSC_MDCTL		0xa00

#define PDCTL_STATE_MASK		0x1
#define PDCTL_STATE_OFF			0x0
#define PDCTL_STATE_ON			0x1

#define MDSTAT_STATE_MASK		0x3f
#define MDSTAT_BUSY_MASK		0x30
#define MDSTAT_STATE_SWRSTDISABLE	0x0
#define MDSTAT_STATE_ENABLE		0x3

#define LPSC_TIMEOUT		1000
#define PD_TIMEOUT		1000

#define LPSC_MODULE_EXISTS      BIT(0)
#define LPSC_NO_CLOCK_GATING    BIT(1)
#define LPSC_DEPENDS            BIT(2)
#define LPSC_HAS_RESET_ISO      BIT(3)
#define LPSC_HAS_LOCAL_RESET    BIT(4)
#define LPSC_NO_MODULE_RESET    BIT(5)

#define PSC_PD_EXISTS           BIT(0)
#define PSC_PD_ALWAYSON         BIT(1)
#define PSC_PD_DEPENDS          BIT(2)

#define MDSTAT_STATE_MASK		0x3f
#define MDSTAT_BUSY_MASK		0x30
#define MDSTAT_STATE_SWRSTDISABLE	0x0
#define MDSTAT_STATE_ENABLE		0x3

struct ti_psc {
	int id;
	void __iomem *base;
};

struct ti_pd;

struct ti_pd {
	int id;
	int usecount;
	struct ti_psc *psc;
	struct ti_pd *depend;
};

struct ti_lpsc;

struct ti_lpsc {
	int id;
	int usecount;
	struct ti_psc *psc;
	struct ti_pd *pd;
	struct ti_lpsc *depend;
};

struct ti_dev {
	int lpsc;
	int id;
};

/**
 * struct ti_k3_pd_platdata - pm domain controller information structure
 */
struct ti_k3_pd_platdata {
	struct ti_psc *psc;
	struct ti_pd *pd;
	struct ti_lpsc *lpsc;
	struct ti_dev *devs;
	int num_psc;
	int num_pd;
	int num_lpsc;
	int num_devs;
};

struct ti_k3_pm_domain {
	struct generic_pm_domain genpd;
	struct ti_lpsc *lpsc;
};

struct ti_k3_priv {
	const struct ti_k3_pd_platdata *data;
	struct genpd_onecell_data pd_data;
	struct ti_k3_pm_domain *pd;
};

static u32 psc_read(struct ti_psc *psc, u32 reg)
{
	return readl(psc->base + reg);
}

static void psc_write(u32 val, struct ti_psc *psc, u32 reg)
{
	writel(val, psc->base + reg);
}

static u32 pd_read(struct ti_pd *pd, u32 reg)
{
	return psc_read(pd->psc, reg + 4 * pd->id);
}

static void pd_write(u32 val, struct ti_pd *pd, u32 reg)
{
	psc_write(val, pd->psc, reg + 4 * pd->id);
}

static u32 lpsc_read(struct ti_lpsc *lpsc, u32 reg)
{
	return psc_read(lpsc->psc, reg + 4 * lpsc->id);
}

static void lpsc_write(u32 val, struct ti_lpsc *lpsc, u32 reg)
{
	psc_write(val, lpsc->psc, reg + 4 * lpsc->id);
}

static int ti_pd_wait(struct ti_pd *pd)
{
	u32 ptstat;
	u32 pdoffset = 0;
	u32 ptstatreg = PSC_PTSTAT;
	int ret;

	if (pd->id > 31) {
		pdoffset = 32;
		ptstatreg = PSC_PTSTAT_H;
	}

	ret = readl_poll_timeout(pd->psc->base + ptstatreg, ptstat,
				 !(ptstat & BIT(pd->id - pdoffset)), PD_TIMEOUT);

	if (ret)
		pr_err("%s: psc%d, pd%d failed to transition.\n", __func__,
		       pd->psc->id, pd->id);

	return ret;
}

static void ti_pd_transition(struct ti_pd *pd)
{
	u32 pdoffset = 0;
	u32 ptcmdreg = PSC_PTCMD;

	if (pd->id > 31) {
		pdoffset = 32;
		ptcmdreg = PSC_PTCMD_H;
	}

	psc_write(BIT(pd->id - pdoffset), pd->psc, ptcmdreg);
}

static int ti_pd_get(struct ti_pd *pd)
{
	u32 pdctl;
	int ret;

	pd->usecount++;

	if (pd->usecount > 1)
		return 0;

	if (pd->depend) {
		ret = ti_pd_get(pd->depend);
		if (ret)
			return ret;
		ti_pd_transition(pd->depend);
		ret = ti_pd_wait(pd->depend);
		if (ret)
			return ret;
	}

	pdctl = pd_read(pd, PSC_PDCTL);

	if ((pdctl & PDCTL_STATE_MASK) == PDCTL_STATE_ON)
		return 0;

	pr_debug("%s: enabling psc:%d, pd:%d\n", __func__, pd->psc->id, pd->id);

	pdctl &= ~PDCTL_STATE_MASK;
	pdctl |= PDCTL_STATE_ON;

	pd_write(pdctl, pd, PSC_PDCTL);

	return 0;
}

static int ti_pd_put(struct ti_pd *pd)
{
	u32 pdctl;
	int ret;

	pd->usecount--;

	if (pd->usecount > 0)
		return 0;

	pdctl = pd_read(pd, PSC_PDCTL);
	if ((pdctl & PDCTL_STATE_MASK) == PDCTL_STATE_OFF)
		return 0;

	pdctl &= ~PDCTL_STATE_MASK;
	pdctl |= PDCTL_STATE_OFF;

	pr_debug("%s: disabling psc:%d, pd:%d\n", __func__, pd->psc->id, pd->id);

	pd_write(pdctl, pd, PSC_PDCTL);

	if (pd->depend) {
		ti_pd_transition(pd);
		ret = ti_pd_wait(pd);
		if (ret)
			return ret;

		ret = ti_pd_put(pd->depend);
		if (ret)
			return ret;
		ti_pd_transition(pd->depend);
		ret = ti_pd_wait(pd->depend);
		if (ret)
			return ret;
	}

	return 0;
}

static int ti_lpsc_wait(struct ti_lpsc *lpsc)
{
	u32 mdstat;
	int ret;

	ret = readl_poll_timeout(lpsc->psc->base + PSC_MDSTAT + lpsc->id * 4,
				 mdstat,
				 !(mdstat & MDSTAT_BUSY_MASK), LPSC_TIMEOUT);

	if (ret)
		pr_err("%s: module %d failed to transition.\n", __func__,
		       lpsc->id);

	return ret;
}

static int ti_lpsc_transition(struct ti_lpsc *lpsc, u8 state)
{
	struct ti_pd *psc_pd;
	int ret;
	u32 mdctl;

	psc_pd = lpsc->pd;

	if (state == MDSTAT_STATE_ENABLE) {
		lpsc->usecount++;
		if (lpsc->usecount > 1)
			return 0;
	} else {
		lpsc->usecount--;
		if (lpsc->usecount >= 1)
			return 0;
	}

	pr_debug("%s: transitioning psc:%d, lpsc:%d to %x\n", __func__,
		 lpsc->psc->id, lpsc->id, state);

	if (lpsc->depend)
		ti_lpsc_transition(lpsc->depend, state);

	mdctl = lpsc_read(lpsc, PSC_MDCTL);
	if ((mdctl & MDSTAT_STATE_MASK) == state)
		return 0;

	if (state == MDSTAT_STATE_ENABLE)
		ti_pd_get(psc_pd);
	else
		ti_pd_put(psc_pd);

	mdctl &= ~MDSTAT_STATE_MASK;
	mdctl |= state;

	lpsc_write(mdctl, lpsc, PSC_MDCTL);

	ti_pd_transition(psc_pd);

	ret = ti_pd_wait(psc_pd);
	if (ret)
		return ret;

	return ti_lpsc_wait(lpsc);
}

static inline struct ti_k3_pm_domain *to_ti_k3_pd(struct generic_pm_domain *gpd)
{
	return container_of(gpd, struct ti_k3_pm_domain, genpd);
}

static int ti_k3_pm_domain_on(struct generic_pm_domain *domain)
{
	struct ti_k3_pm_domain *pd = to_ti_k3_pd(domain);

	return ti_lpsc_transition(pd->lpsc, MDSTAT_STATE_ENABLE);
}       
        
static int ti_k3_pm_domain_off(struct generic_pm_domain *domain)
{       
	struct ti_k3_pm_domain *pd = to_ti_k3_pd(domain);

	return ti_lpsc_transition(pd->lpsc, MDSTAT_STATE_SWRSTDISABLE);
}       

static struct ti_psc am625_psc[] = {
	[0] = { .id = 0, .base = (void *)0x04000000 },
	[1] = { .id = 1, .base = (void *)0x00400000 },
};

static struct ti_pd am625_pd[] = {
	[0] = { .id = 0, .psc = &am625_psc[1], },
	[1] = { .id = 2, .psc = &am625_psc[1], .depend = &am625_pd[0] },
	[2] = { .id = 3, .psc = &am625_psc[1], .depend = &am625_pd[0] },
	[3] = { .id = 4, .psc = &am625_psc[1], .depend = &am625_pd[2] },
	[4] = { .id = 5, .psc = &am625_psc[1], .depend = &am625_pd[2] },
};

static struct ti_lpsc am625_lpsc[] = {
	[0] = { .id = 0, .psc = &am625_psc[1], .pd = &am625_pd[0], },
	[1] = { .id = 9, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[11] },
	[2] = { .id = 10, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[1] },
	[3] = { .id = 11, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[2] },
	[4] = { .id = 12, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[8] },
	[5] = { .id = 13, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[9] },
	[6] = { .id = 20, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[11] },
	[7] = { .id = 21, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[11] },
	[8] = { .id = 23, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[11] },
	[9] = { .id = 24, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[11] },
	[10] = { .id = 28, .psc = &am625_psc[1], .pd = &am625_pd[0], .depend = &am625_lpsc[11] },
	[11] = { .id = 34, .psc = &am625_psc[1], .pd = &am625_pd[0], },
	[12] = { .id = 41, .psc = &am625_psc[1], .pd = &am625_pd[1], .depend = &am625_lpsc[11] },
	[13] = { .id = 42, .psc = &am625_psc[1], .pd = &am625_pd[2], .depend = &am625_lpsc[11] },
	[14] = { .id = 45, .psc = &am625_psc[1], .pd = &am625_pd[3], .depend = &am625_lpsc[13] },
	[15] = { .id = 46, .psc = &am625_psc[1], .pd = &am625_pd[4], .depend = &am625_lpsc[13] },
};

static struct ti_dev am625_dev[] = {
	{ .id = 16,  .lpsc = 0 },
	{ .id = 77,  .lpsc = 0 },
	{ .id = 61,  .lpsc = 0 },
	{ .id = 95,  .lpsc = 0 },
	{ .id = 107, .lpsc = 0 },
	{ .id = 170, .lpsc = 1 },
	{ .id = 177, .lpsc = 2 },
	{ .id = 55,  .lpsc = 3 },
	{ .id = 178, .lpsc = 4 },
	{ .id = 179, .lpsc = 5 },
	{ .id = 57,  .lpsc = 6 },
	{ .id = 58,  .lpsc = 7 },
	{ .id = 161, .lpsc = 8 },
	{ .id = 162, .lpsc = 9 },
	{ .id = 75,  .lpsc = 10 },
	{ .id = 36,  .lpsc = 11 },
	{ .id = 102, .lpsc = 11 },
	{ .id = 146, .lpsc = 11 },
	{ .id = 13,  .lpsc = 12 },
	{ .id = 166, .lpsc = 13 },
	{ .id = 135, .lpsc = 14 },
	{ .id = 136, .lpsc = 15 },
};

const struct ti_k3_pd_platdata am62x_pd_platdata = {
	.psc = am625_psc,
	.pd = am625_pd,
	.lpsc = am625_lpsc,
	.devs = am625_dev,
	.num_psc = ARRAY_SIZE(am625_psc),
	.num_pd = ARRAY_SIZE(am625_pd),
	.num_lpsc = ARRAY_SIZE(am625_lpsc),
	.num_devs = ARRAY_SIZE(am625_dev),
};

static struct generic_pm_domain *ti_k3_pd_xlate(struct of_phandle_args *args,
						void *data)
{
	struct genpd_onecell_data *genpd_data = data;
	struct ti_k3_priv *priv = container_of(genpd_data, struct ti_k3_priv, pd_data);
	unsigned int idx = args->args[0];
	int i;

	for (i = 0; i < priv->data->num_devs; i++)
		if (priv->data->devs[i].id == idx) {
			unsigned int lpsc = priv->data->devs[i].lpsc;
			pr_debug("Translate: %d -> %d\n", idx, lpsc);
			return &priv->pd[lpsc].genpd;
		}

	return 0;
}

static int ti_k3_pm_domain_probe(struct device *dev)
{
	struct ti_k3_priv *priv;
	const struct ti_k3_pd_platdata *data;
	struct ti_k3_pm_domain *pd;
	struct generic_pm_domain **domains;
	struct genpd_onecell_data *pd_data;
	int num_domains;
	int i;

	priv = xzalloc(sizeof(*priv));

	if (of_machine_is_compatible("ti,am625"))
		data = &am62x_pd_platdata;
	else
		return dev_err_probe(dev, -EINVAL, "Unknown SoC\n");

	priv->data = data;

	num_domains = data->num_lpsc;

	pd = devm_kcalloc(dev, num_domains, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return -ENOMEM;

	domains = devm_kcalloc(dev, num_domains, sizeof(*domains), GFP_KERNEL);
	if (!domains)
		return -ENOMEM;

	priv->pd = pd;

	for (i = 0; i < num_domains; i++, pd++) {
		pd->genpd.name = basprintf("pd:%d", i);
		pd->lpsc = &data->lpsc[i];
		pd->genpd.power_off = ti_k3_pm_domain_off;
		pd->genpd.power_on = ti_k3_pm_domain_on;

		pm_genpd_init(&pd->genpd, NULL, true);

		domains[i] = &pd->genpd;
	}

	pd_data = &priv->pd_data;

	pd_data->domains = domains;
	pd_data->num_domains = num_domains;
	pd_data->xlate = ti_k3_pd_xlate;

	return of_genpd_add_provider_onecell(dev->of_node, pd_data);
}

static const struct of_device_id ti_k3_power_domain_of_match[] = {
	{ .compatible = "ti,sci-pm-domain" },
	{ /* sentinel */ }
};

static struct driver ti_k3_pm_domains_driver = {
        .probe = ti_k3_pm_domain_probe,
        .name = "ti_k3_pm_domains",
        .of_match_table = ti_k3_power_domain_of_match,
};
core_platform_driver(ti_k3_pm_domains_driver);
