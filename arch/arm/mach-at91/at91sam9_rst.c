// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Pengutronix, Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <mach/at91/at91_rstc.h>
#include <reset_source.h>

struct at91sam9x_rst {
	struct restart_handler restart;
	void __iomem *base;
};

static int reasons[] = {
	RESET_POR, /* GENERAL  Both VDDCORE and VDDBU rising */
	RESET_WKE, /* WAKEUP   VDDCORE rising */
	RESET_WDG, /* WATCHDOG Watchdog fault occurred */
	RESET_RST, /* SOFTWARE Reset required by the software */
	RESET_EXT, /* USER     NRST pin detected low */
};

static void at91sam9x_set_reset_reason(struct device *dev,
				       void __iomem *base)
{
	enum reset_src_type type = RESET_UKWN;
	u32 sr, rsttyp;

	sr = readl(base + AT91_RSTC_SR);
	rsttyp = FIELD_GET(AT91_RSTC_RSTTYP, sr);

	if (rsttyp < ARRAY_SIZE(reasons))
		type = reasons[rsttyp];

	dev_info(dev, "reset reason %s (RSTC_SR: 0x%05x)\n",
		reset_source_to_string(type), sr);

	reset_source_set(type);
}

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

static int at91sam9x_rst_probe(struct device *dev)
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

	at91sam9x_set_reset_reason(dev, priv->base);

	priv->restart.name = "at91sam9x-rst";
	priv->restart.restart = at91sam9x_restart_soc;

	return restart_handler_register(&priv->restart);
}

static const __maybe_unused struct of_device_id at91sam9x_rst_dt_ids[] = {
	{ .compatible = "atmel,at91sam9g45-rstc", },
	{ .compatible = "atmel,sama5d3-rstc", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, at91sam9x_rst_dt_ids);

static struct driver at91sam9x_rst_driver = {
	.name		= "at91sam9x-rst",
	.of_compatible	= DRV_OF_COMPAT(at91sam9x_rst_dt_ids),
	.probe		= at91sam9x_rst_probe,
};
device_platform_driver(at91sam9x_rst_driver);
