/*
 * Copyright 2016 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>

#define SRC_SCR		0x0

#define SCR_WARM_RESET_ENABLE	BIT(0)

static int imx_src_reset_probe(struct device_d *dev)
{
	struct resource *res;
	u32 val;
	void __iomem *membase;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	membase = IOMEM(res->start);

	/*
	 * Generate cold reset for warm reset sources. Needed for
	 * some boards to come up properly after reset.
	 */
	val = readl(membase + SRC_SCR);
	val &= ~SCR_WARM_RESET_ENABLE;
	writel(val, membase + SRC_SCR);

	return 0;
}

static const struct of_device_id imx_src_dt_ids[] = {
	{ .compatible = "fsl,imx51-src", },
	{ /* sentinel */ },
};

static struct driver_d imx_src_reset_driver = {
	.name = "imx-src",
	.probe	= imx_src_reset_probe,
	.of_compatible	= DRV_OF_COMPAT(imx_src_dt_ids),
};

static int imx_src_reset_init(void)
{
	return platform_driver_register(&imx_src_reset_driver);
}
postcore_initcall(imx_src_reset_init);
