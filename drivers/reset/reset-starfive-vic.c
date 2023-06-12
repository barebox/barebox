// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021 Ahmad Fatoum, Pengutronix
 *
 * StarFive Reset Controller driver
 */
#define pr_fmt(fmt) "reset-starfive: " fmt

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/reset-controller.h>
#include <soc/starfive/rstgen.h>
#include <dt-bindings/reset-controller/starfive-jh7100.h>
#include <dt-bindings/clock/starfive-jh7100.h>

struct starfive_rstgen {
	void __iomem *base;
	struct reset_controller_dev rcdev;
	const struct starfive_rstgen_ops *ops;
	struct device_node *clknp;
	const int *sync_resets;
};

static struct starfive_rstgen *to_starfive_rstgen(struct reset_controller_dev *rcdev)
{
	return container_of(rcdev, struct starfive_rstgen, rcdev);
}

static const int jh7110_rstgen_sync_resets[RSTN_END] = {
	[RSTN_SGDMA2P_AHB]   = CLK_SGDMA2P_AHB,
	[RSTN_SGDMA2P_AXI]   = CLK_SGDMA2P_AXI,
	[RSTN_DMA2PNOC_AXI]  = CLK_DMA2PNOC_AXI,
	[RSTN_DLA_AXI]       = CLK_DLA_AXI,
	[RSTN_DLANOC_AXI]    = CLK_DLANOC_AXI,
	[RSTN_DLA_APB]       = CLK_DLA_APB,
	[RSTN_VDECBRG_MAIN]  = CLK_VDECBRG_MAIN,
	[RSTN_VDEC_AXI]      = CLK_VDEC_AXI,
	[RSTN_VDEC_BCLK]     = CLK_VDEC_BCLK,
	[RSTN_VDEC_CCLK]     = CLK_VDEC_CCLK,
	[RSTN_VDEC_APB]      = CLK_VDEC_APB,
	[RSTN_JPEG_AXI]      = CLK_JPEG_AXI,
	[RSTN_JPEG_CCLK]     = CLK_JPEG_CCLK,
	[RSTN_JPEG_APB]      = CLK_JPEG_APB,
	[RSTN_JPCGC300_MAIN] = CLK_JPCGC300_MAIN,
	[RSTN_GC300_2X]      = CLK_GC300_2X,
	[RSTN_GC300_AXI]     = CLK_GC300_AXI,
	[RSTN_GC300_AHB]     = CLK_GC300_AHB,
	[RSTN_VENC_AXI]      = CLK_VENC_AXI,
	[RSTN_VENCBRG_MAIN]  = CLK_VENCBRG_MAIN,
	[RSTN_VENC_BCLK]     = CLK_VENC_BCLK,
	[RSTN_VENC_CCLK]     = CLK_VENC_CCLK,
	[RSTN_VENC_APB]      = CLK_VENC_APB,
	[RSTN_DDRPHY_APB]    = CLK_DDRPHY_APB,
	[RSTN_USB_AXI]       = CLK_USB_AXI,
	[RSTN_SGDMA1P_AXI]   = CLK_SGDMA1P_AXI,
	[RSTN_DMA1P_AXI]     = CLK_DMA1P_AXI,
	[RSTN_NNE_AHB]       = CLK_NNE_AHB,
	[RSTN_NNE_AXI]       = CLK_NNE_AXI,
	[RSTN_NNENOC_AXI]    = CLK_NNENOC_AXI,
	[RSTN_DLASLV_AXI]    = CLK_DLASLV_AXI,
	[RSTN_VOUT_SRC]      = CLK_VOUT_SRC,
	[RSTN_DISP_AXI]      = CLK_DISP_AXI,
	[RSTN_DISPNOC_AXI]   = CLK_DISPNOC_AXI,
	[RSTN_SDIO0_AHB]     = CLK_SDIO0_AHB,
	[RSTN_SDIO1_AHB]     = CLK_SDIO1_AHB,
	[RSTN_GMAC_AHB]      = CLK_GMAC_AHB,
	[RSTN_SPI2AHB_AHB]   = CLK_SPI2AHB_AHB,
	[RSTN_SPI2AHB_CORE]  = CLK_SPI2AHB_CORE,
	[RSTN_EZMASTER_AHB]  = CLK_EZMASTER_AHB,
	[RSTN_SEC_AHB]       = CLK_SEC_AHB,
	[RSTN_AES]           = CLK_AES,
	[RSTN_PKA]           = CLK_PKA,
	[RSTN_SHA]           = CLK_SHA,
	[RSTN_TRNG_APB]      = CLK_TRNG_APB,
	[RSTN_OTP_APB]       = CLK_OTP_APB,
	[RSTN_UART0_APB]     = CLK_UART0_APB,
	[RSTN_UART0_CORE]    = CLK_UART0_CORE,
	[RSTN_UART1_APB]     = CLK_UART1_APB,
	[RSTN_UART1_CORE]    = CLK_UART1_CORE,
	[RSTN_SPI0_APB]      = CLK_SPI0_APB,
	[RSTN_SPI0_CORE]     = CLK_SPI0_CORE,
	[RSTN_SPI1_APB]      = CLK_SPI1_APB,
	[RSTN_SPI1_CORE]     = CLK_SPI1_CORE,
	[RSTN_I2C0_APB]      = CLK_I2C0_APB,
	[RSTN_I2C0_CORE]     = CLK_I2C0_CORE,
	[RSTN_I2C1_APB]      = CLK_I2C1_APB,
	[RSTN_I2C1_CORE]     = CLK_I2C1_CORE,
	[RSTN_GPIO_APB]      = CLK_GPIO_APB,
	[RSTN_UART2_APB]     = CLK_UART2_APB,
	[RSTN_UART2_CORE]    = CLK_UART2_CORE,
	[RSTN_UART3_APB]     = CLK_UART3_APB,
	[RSTN_UART3_CORE]    = CLK_UART3_CORE,
	[RSTN_SPI2_APB]      = CLK_SPI2_APB,
	[RSTN_SPI2_CORE]     = CLK_SPI2_CORE,
	[RSTN_SPI3_APB]      = CLK_SPI3_APB,
	[RSTN_SPI3_CORE]     = CLK_SPI3_CORE,
	[RSTN_I2C2_APB]      = CLK_I2C2_APB,
	[RSTN_I2C2_CORE]     = CLK_I2C2_CORE,
	[RSTN_I2C3_APB]      = CLK_I2C3_APB,
	[RSTN_I2C3_CORE]     = CLK_I2C3_CORE,
	[RSTN_WDTIMER_APB]   = CLK_WDTIMER_APB,
	[RSTN_WDT]           = CLK_WDT_CORE,
	[RSTN_VP6INTC_APB]   = CLK_VP6INTC_APB,
	[RSTN_TEMP_APB]      = CLK_TEMP_APB,
	[RSTN_TEMP_SENSE]    = CLK_TEMP_SENSE,
};

static struct clk *starfive_reset_clk_get(struct starfive_rstgen *priv, unsigned id)
{
	struct of_phandle_args clkspec = {
		.np = priv->clknp,
		.args_count = 1,
	};

	if (!priv->sync_resets || !priv->sync_resets[id])
		return 0;

	clkspec.args[0] = priv->sync_resets[id];

	pr_debug("synchronous reset=%u clk=%u\n", id, priv->sync_resets[id]);

	return of_clk_get_from_provider(&clkspec);
}

static int starfive_reset_clk_enable(struct starfive_rstgen *priv, unsigned id)
{
	return clk_enable(starfive_reset_clk_get(priv, id));
}

static void starfive_reset_clk_disable(struct starfive_rstgen *priv, unsigned id)
{
	clk_disable(starfive_reset_clk_get(priv, id));
}

static int starfive_rstgen(struct starfive_rstgen *priv, unsigned id, bool assert)
{
	void __iomem *base = priv->base;

	__starfive_rstgen(base, id, assert);

	return wait_on_timeout(NSEC_PER_MSEC, __starfive_rstgen_asserted(base, id) == assert);
}

static int starfive_rstgen_assert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct starfive_rstgen *priv = to_starfive_rstgen(rcdev);
	int ret;

	starfive_reset_clk_enable(priv, id);
	ret = starfive_rstgen(priv, id, true);
	starfive_reset_clk_disable(priv, id);

	return ret;
}

static int starfive_rstgen_deassert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct starfive_rstgen *priv = to_starfive_rstgen(rcdev);
	int ret;

	starfive_reset_clk_enable(priv, id);
	ret = starfive_rstgen(priv, id, false);
	starfive_reset_clk_disable(priv, id);

	return ret;
}

static int starfive_reset(struct reset_controller_dev *rcdev, unsigned long id)
{
	struct starfive_rstgen *priv = to_starfive_rstgen(rcdev);
	int ret;

	starfive_reset_clk_enable(priv, id);

	ret = starfive_rstgen(priv, id, true);
	if (ret)
		goto out;

	udelay(2);

	ret = starfive_rstgen(priv, id, false);

out:
	starfive_reset_clk_disable(priv, id);

	return ret;
}

static const struct reset_control_ops starfive_rstgen_ops = {
	.assert		= starfive_rstgen_assert,
	.deassert	= starfive_rstgen_deassert,
	.reset		= starfive_reset,
};

static int starfive_rstgen_probe(struct device *dev)
{
	struct starfive_rstgen *priv;
	struct resource *iores;

	priv = xzalloc(sizeof(*priv));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	if ((priv->sync_resets = device_get_match_data(dev))) {
		priv->clknp = of_find_compatible_node(NULL, NULL, "starfive,jh7100-clkgen");
		if (!priv->clknp)
			return -ENODEV;
	}

	priv->base = IOMEM(iores->start);
	priv->rcdev.nr_resets = RSTN_END;
	priv->rcdev.ops = &starfive_rstgen_ops;
	priv->rcdev.of_node = dev->of_node;

	return reset_controller_register(&priv->rcdev);
}

static const struct of_device_id starfive_rstgen_reset_dt_ids[] = {
	{ .compatible = "starfive,jh7100-rstgen", .data = jh7110_rstgen_sync_resets },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, starfive_rstgen_reset_dt_ids);

static struct driver starfive_rstgen_reset_driver = {
	.name = "starfive_rstgen",
	.probe = starfive_rstgen_probe,
	.of_compatible = starfive_rstgen_reset_dt_ids,
};
core_platform_driver(starfive_rstgen_reset_driver);
