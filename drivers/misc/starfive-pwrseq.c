// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021 Ahmad Fatoum, Pengutronix
 */

#include <driver.h>
#include <init.h>
#include <linux/reset.h>
#include <dt-bindings/clock/starfive-jh7100.h>
#include <linux/clk.h>

struct starfive_pwrseq {
	const char **names;
};

static int starfive_pwrseq_probe(struct device *dev)
{
	int ret;

	ret = device_reset_all(dev);
	if (ret)
		return ret;

	return of_platform_populate(dev->of_node, NULL, dev);
}

static struct of_device_id starfive_pwrseq_dt_ids[] = {
	{ .compatible = "starfive,axi-dma" },
	{ .compatible = "cm,cm521-vpu" },
	{ .compatible = "starfive,vic-sec" },
	{ .compatible = "sfc,tempsensor" },
	{ .compatible = "cm,codaj12-jpu-1" },
	{ .compatible = "cdns,xrp" },
	{ .compatible = "starfive,nne50" },
	{ .compatible = "nvidia,nvdla_os_initial" },
	{ .compatible = "starfive,spi2ahb" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, starfive_pwrseq_dt_ids);

static struct driver starfive_pwrseq_driver = {
	.name	= "starfive_pwrseq",
	.probe	= starfive_pwrseq_probe,
	.of_compatible = starfive_pwrseq_dt_ids,
};

static const int clks[] = {
	CLK_VDEC_AXI, CLK_VDECBRG_MAIN, CLK_VDEC_BCLK, CLK_VDEC_CCLK, CLK_VDEC_APB,
	CLK_JPEG_AXI, CLK_JPEG_CCLK, CLK_JPEG_APB,
	CLK_DLA_AXI, CLK_DLANOC_AXI, CLK_DLA_APB, CLK_NNENOC_AXI, CLK_DLASLV_AXI,
	CLK_VENC_AXI, CLK_VENCBRG_MAIN, CLK_VENC_BCLK, CLK_VENC_CCLK, CLK_VENC_APB,
	CLK_SGDMA1P_AXI,
	CLK_DMA2PNOC_AXI, CLK_SGDMA2P_AXI, CLK_SGDMA2P_AHB,
	CLK_SDIO0_AHB,
	CLK_SDIO1_AHB,
	CLK_SPI2AHB_AHB, CLK_SPI2AHB_CORE,
	CLK_EZMASTER_AHB,
	CLK_SEC_AHB, CLK_AES, CLK_SHA, CLK_PKA,
	CLK_UART0_APB, CLK_UART0_CORE,
	CLK_UART1_APB, CLK_UART1_CORE,
	CLK_UART2_APB, CLK_UART2_CORE,
	CLK_UART3_APB, CLK_UART3_CORE,
	CLK_SPI0_APB, CLK_SPI0_CORE,
	CLK_SPI1_APB, CLK_SPI1_CORE,
	CLK_SPI2_APB, CLK_SPI2_CORE,
	CLK_SPI3_APB, CLK_SPI3_CORE,
	CLK_I2C0_APB, CLK_I2C0_CORE,
	CLK_I2C1_APB, CLK_I2C1_CORE,
	CLK_I2C2_APB, CLK_I2C2_CORE,
	CLK_I2C3_APB, CLK_I2C3_CORE,
	CLK_VP6INTC_APB,
	CLK_TEMP_APB, CLK_TEMP_SENSE,

	CLK_END
};

static int __init starfive_pwrseq_driver_register(void)
{
	struct of_phandle_args clkspec;
	int i;

	clkspec.args_count = 1;
	clkspec.np = of_find_compatible_node(NULL, NULL, "starfive,jh7100-clkgen");
	if (clkspec.np) {
		for (i = 0; clks[i] != CLK_END; i++) {
			clkspec.args[0] = clks[i];
			clk_enable(of_clk_get_from_provider(&clkspec));
		}
	}

	return platform_driver_register(&starfive_pwrseq_driver);
}
device_initcall(starfive_pwrseq_driver_register);
