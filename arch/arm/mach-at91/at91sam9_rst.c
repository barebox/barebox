// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Pengutronix, Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <linux/clk.h>
#include <mach/at91_rstc.h>

struct at91sam9x_rst {
	struct restart_handler restart;
	void __iomem *base;
};

static void __noreturn at91sam9x_restart_soc(struct restart_handler *rst)
{
	struct at91sam9x_rst *priv = container_of(rst, struct at91sam9x_rst, restart);

	writel(AT91_RSTC_PROCRST
	       | AT91_RSTC_PERRST
	       | AT91_RSTC_EXTRST
	       | AT91_RSTC_KEY,
	       priv->base + AT91_RSTC_CR);

	hang();
}

static int at91sam9x_rst_probe(struct device_d *dev)
{
	struct at91sam9x_rst *priv;
	struct resource *iores;
	struct clk *clk;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get reset memory region\n");
		return PTR_ERR(iores);
	}

	priv = xzalloc(sizeof(*priv));
	priv->base = IOMEM(iores->start);

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		release_region(iores);
		free(priv);
		return PTR_ERR(clk);
	}

	clk_enable(clk);

	priv->restart.name = "at91sam9x-rst";
	priv->restart.restart = at91sam9x_restart_soc;

	return restart_handler_register(&priv->restart);
}

static const __maybe_unused struct of_device_id at91sam9x_rst_dt_ids[] = {
	{ .compatible = "atmel,at91sam9g45-rstc", },
	{ .compatible = "atmel,sama5d3-rstc", },
	{ /* sentinel */ },
};

static struct driver_d at91sam9x_rst_driver = {
	.name		= "at91sam9x-rst",
	.of_compatible	= DRV_OF_COMPAT(at91sam9x_rst_dt_ids),
	.probe		= at91sam9x_rst_probe,
};
device_platform_driver(at91sam9x_rst_driver);
