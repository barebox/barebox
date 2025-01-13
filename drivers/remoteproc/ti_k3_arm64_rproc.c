// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments' K3 ARM64 Remoteproc driver
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - https://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 */

#include <driver.h>
#include <linux/remoteproc.h>
#include <linux/printk.h>
#include <errno.h>
#include <linux/clk.h>
#include <io.h>
#include <of.h>
#include <soc/ti/ti_sci_protocol.h>
#include <pm_domain.h>
#include <mach/k3/r5.h>

#include "ti_sci_proc.h"

#define INVALID_ID	0xffff

#define GTC_CNTCR_REG	0x0
#define GTC_CNTFID0_REG	0x20
#define GTC_CNTR_EN	0x3

/**
 * struct k3_arm64_privdata - Structure representing Remote processor data.
 * @rproc_pwrdmn:	rproc power domain data
 * @rproc_rst:		rproc reset control data
 * @sci:		Pointer to TISCI handle
 * @tsp:		TISCI processor control helper structure
 * @gtc_clk:		GTC clock description
 * @gtc_base:		Timer base address.
 */
struct k3_arm64_privdata {
	struct device *dev;
	struct reset_control *rproc_rst;
	struct ti_sci_proc tsp;
	struct clk *gtc_clk;
	void *gtc_base;
	struct rproc *rproc;
	struct device *cluster_pwrdmn;
	struct device *rproc_pwrdmn;
	struct device *gtc_pwrdmn;
};

/**
 * k3_arm64_load() - Load up the Remote processor image
 * @dev:	rproc device pointer
 * @addr:	Address at which image is available
 * @size:	size of the image
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int k3_arm64_load(struct rproc *rproc, const struct firmware *fw)
{
	struct k3_arm64_privdata *priv = rproc->priv;
	ulong gtc_rate;
	int ret;

	dev_dbg(priv->dev, "%s\n", __func__);

	/* request for the processor */
	ret = ti_sci_proc_request(&priv->tsp);
	if (ret)
		return ret;

	ret = pm_runtime_resume_and_get_genpd(priv->gtc_pwrdmn);
	if (ret)
		return ret;

	gtc_rate = clk_get_rate(priv->gtc_clk);
	dev_dbg(priv->dev, "GTC RATE= %lu\n", gtc_rate);

	/* Store the clock frequency down for GTC users to pick  up */
	writel((u32)gtc_rate, priv->gtc_base + GTC_CNTFID0_REG);

	/* Enable the timer before starting remote core */
	writel(GTC_CNTR_EN, priv->gtc_base + GTC_CNTCR_REG);

	/*
	 * Setting the right clock frequency would have taken care by
	 * assigned-clock-rates during the device probe. So no need to
	 * set the frequency again here.
	 */
	if (priv->cluster_pwrdmn) {
		ret = pm_runtime_resume_and_get_genpd(priv->cluster_pwrdmn);
		if (ret)
			return ret;
	}

	return ti_sci_proc_set_config(&priv->tsp, (unsigned long)fw->data, 0, 0);
}

/**
 * k3_arm64_start() - Start the remote processor
 * @dev:	rproc device pointer
 *
 * Return: 0 if all went ok, else return appropriate error
 */
static int k3_arm64_start(struct rproc *rproc)
{
	struct k3_arm64_privdata *priv = rproc->priv;
	int ret;

	dev_dbg(priv->dev, "%s\n", __func__);
	ret = pm_runtime_resume_and_get_genpd(priv->rproc_pwrdmn);
	if (ret)
		return ret;

	return ti_sci_proc_release(&priv->tsp);
}

static const struct rproc_ops k3_arm64_ops = {
	.load = k3_arm64_load,
	.start = k3_arm64_start,
};

static int ti_sci_proc_of_to_priv(struct k3_arm64_privdata *priv, struct ti_sci_proc *tsp)
{
	u32 val;
	int ret;

	tsp->sci = ti_sci_get_by_phandle(priv->dev, "ti,sci");
	if (IS_ERR(tsp->sci)) {
		dev_err(priv->dev, "ti_sci get failed: %ld\n", PTR_ERR(tsp->sci));
		return PTR_ERR(tsp->sci);
	}

	ret = of_property_read_u32(priv->dev->of_node, "ti,sci-proc-id", &val);
	if (ret) {
		dev_err(priv->dev, "proc id not populated\n");
		return -ENOENT;
	}
	tsp->proc_id = val;

	ret = of_property_read_u32(priv->dev->of_node, "ti,sci-host-id", &val);
	if (ret)
		val = INVALID_ID;

	tsp->host_id = val;

	tsp->ops = &tsp->sci->ops.proc_ops;

	return 0;
}

static struct rproc *ti_k3_am64_rproc;

struct rproc *ti_k3_am64_get_handle(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "ti,am654-rproc");
	if (!np)
		return ERR_PTR(-ENODEV);
        of_device_ensure_probed(np);

	return ti_k3_am64_rproc;
}


static int ti_k3_rproc_probe(struct device *dev)
{
	struct k3_arm64_privdata *priv;
	struct rproc *rproc;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	rproc = rproc_alloc(dev, dev_name(dev), &k3_arm64_ops, sizeof(*priv));
	if (!rproc)
		return -ENOMEM;

	priv = rproc->priv;
	priv->dev = dev;

	priv->cluster_pwrdmn = dev_pm_domain_attach_by_id(dev, 2);
	if (IS_ERR(priv->cluster_pwrdmn))
		priv->cluster_pwrdmn = NULL;

	priv->rproc_pwrdmn = dev_pm_domain_attach_by_id(dev, 1);
	if (IS_ERR(priv->rproc_pwrdmn))
		return dev_err_probe(dev, PTR_ERR(priv->rproc_pwrdmn), "no rproc pm domain\n");

	priv->gtc_pwrdmn = dev_pm_domain_attach_by_id(dev, 0);
	if (IS_ERR(priv->gtc_pwrdmn))
		return dev_err_probe(dev, PTR_ERR(priv->gtc_pwrdmn), "no gtc pm domain\n");

	priv->gtc_clk = clk_get(dev, 0);
	if (IS_ERR(priv->gtc_clk))
		return dev_err_probe(dev, PTR_ERR(priv->gtc_clk), "No clock\n");

	ret = ti_sci_proc_of_to_priv(priv, &priv->tsp);
	if (ret)
		return ret;

	priv->gtc_base = dev_request_mem_region(dev, 0);
	if (IS_ERR(priv->gtc_base))
		return dev_err_probe(dev, PTR_ERR(priv->gtc_base), "No iomem\n");

	ret = rproc_add(rproc);
	if (ret)
		return dev_err_probe(dev, ret, "rproc_add failed\n");

	ti_k3_am64_rproc = rproc;

	dev_dbg(dev, "Remoteproc successfully probed\n");

	return 0;
}

static const struct of_device_id k3_arm64_ids[] = {
	{ .compatible = "ti,am654-rproc"},
	{}
};

static struct driver ti_k3_arm64_rproc_driver = {
        .name = "ti-k3-rproc",
        .probe = ti_k3_rproc_probe,
        .of_compatible = DRV_OF_COMPAT(k3_arm64_ids),
};
device_platform_driver(ti_k3_arm64_rproc_driver);
