// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Marek Vasut <marex@denx.de>
 *
 * This code is based on drivers/gpu/drm/mxsfb/mxsfb*
 */

#include <linux/clk.h>
#include <dma.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <video/videomode.h>

#include "lcdif_drv.h"
#include "lcdif_regs.h"

#include <fb.h>
#include <video/vpl.h>

static int lcdif_probe(struct device *dev)
{
	struct lcdif_drm_private *lcdif;
	struct resource *res;
	int ret;

	lcdif = xzalloc(sizeof(*lcdif));
	if (!lcdif)
		return -ENOMEM;

	lcdif->dev = dev;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	lcdif->base = IOMEM(res->start);
	if (IS_ERR(lcdif->base))
		return PTR_ERR(lcdif->base);

	lcdif->clk = clk_get(lcdif->dev, "pix");
	if (IS_ERR(lcdif->clk))
		return PTR_ERR(lcdif->clk);

	lcdif->clk_axi = clk_get(lcdif->dev, "axi");
	if (IS_ERR(lcdif->clk_axi))
		return PTR_ERR(lcdif->clk_axi);

	lcdif->clk_disp_axi = clk_get(lcdif->dev, "disp_axi");
	if (IS_ERR(lcdif->clk_disp_axi))
		return PTR_ERR(lcdif->clk_disp_axi);

	ret = lcdif_kms_init(lcdif);
	if (ret < 0) {
		dev_err(lcdif->dev, "Failed to initialize KMS pipeline\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id lcdif_dt_ids[] = {
	{ .compatible = "fsl,imx8mp-lcdif" },
	{ .compatible = "fsl,imx93-lcdif" },
	{ /* sentinel */ }
};

static struct driver lcdif_platform_driver = {
	.probe		= lcdif_probe,
	.name		= "imx-lcdif",
	.of_compatible	= lcdif_dt_ids,
};
device_platform_driver(lcdif_platform_driver);
