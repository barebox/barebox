// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2016 Sascha Hauer <s.hauer@pengutronix.de>

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>

#define SRC_SCR		0x0

#define SCR_WARM_RESET_ENABLE	BIT(0)

static int imx_src_reset_probe(struct device *dev)
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
MODULE_DEVICE_TABLE(of, imx_src_dt_ids);

static struct driver imx_src_reset_driver = {
	.name = "imx-src",
	.probe	= imx_src_reset_probe,
	.of_compatible	= DRV_OF_COMPAT(imx_src_dt_ids),
};
postcore_platform_driver(imx_src_reset_driver);
