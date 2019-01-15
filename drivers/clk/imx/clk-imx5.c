/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <of.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/imx50-regs.h>
#include <mach/imx51-regs.h>
#include <mach/imx53-regs.h>
#include <dt-bindings/clock/imx5-clock.h>

#include "clk.h"

/* Register addresses of CCM*/
#define CCM_CCR		0x00
#define CCM_CCDR	0x04
#define CCM_CSR		0x08
#define CCM_CCSR	0x0C
#define CCM_CACRR	0x10
#define CCM_CBCDR	0x14
#define CCM_CBCMR	0x18
#define CCM_CSCMR1	0x1C
#define CCM_CSCMR2	0x20
#define CCM_CSCDR1	0x24
#define CCM_CS1CDR	0x28
#define CCM_CS2CDR	0x2C
#define CCM_CDCDR	0x30
#define CCM_CHSCDR	0x34
#define CCM_CSCDR2	0x38
#define CCM_CSCDR3	0x3C
#define CCM_CSCDR4	0x40
#define CCM_CWDR	0x44
#define CCM_CDHIPR	0x48
#define CCM_CDCR	0x4C
#define CCM_CTOR	0x50
#define CCM_CLPCR	0x54
#define CCM_CISR	0x58
#define CCM_CIMR	0x5C
#define CCM_CCOSR	0x60
#define CCM_CGPR	0x64
#define CCM_CCGR0	0x68
#define CCM_CCGR1	0x6C
#define CCM_CCGR2	0x70
#define CCM_CCGR3	0x74
#define CCM_CCGR4	0x78
#define CCM_CCGR5	0x7C
#define CCM_CCGR6	0x80
#define CCM_CCGR7	0x84

#define CCM_CMEOR	0x84

static struct clk *clks[IMX5_CLK_END];
static struct clk_onecell_data clk_data;

/* This is used multiple times */
static const char *standard_pll_sel[] = {
	"pll1_sw",
	"pll2_sw",
	"pll3_sw",
	"lp_apm",
};

static const char *mx50_3bit_clk_sel[] = {
	"pll1_sw",
	"pll2_sw",
	"pll3_sw",
	"lp_apm",
	"pfd0",
	"pfd1",
	"pfd4",
	"osc",
};

static const char *lp_apm_sel[] = {
	"osc",
};

static const char *periph_apm_sel[] = {
	"pll1_sw",
	"pll3_sw",
	"lp_apm",
};

static const char *main_bus_sel[] = {
	"pll2_sw",
	"periph_apm",
};

static const char *mx50_periph_clk_sel[] = {
	"pll1_sw",
	"pll2_sw",
	"pll3_sw",
	"lp_apm",
};

static const char *per_lp_apm_sel[] = {
	"main_bus",
	"lp_apm",
};

static const char *per_root_sel[] = {
	"per_podf",
	"ipg",
};

static const char *esdhc_c_sel[] = {
	"esdhc_a_podf",
	"esdhc_b_podf",
};

static const char *esdhc_d_sel[] = {
	"esdhc_a_podf",
	"esdhc_b_podf",
};

static const char *emi_slow_sel[] = {
	"main_bus",
	"ahb",
};

static const char *usb_phy_sel_str[] = {
	"osc",
	"usb_phy_podf",
};

static const char *mx51_ipu_di0_sel[] = {
	"di_pred",
	"osc",
	"ckih1",
	"tve_di",
};

static const char *mx53_ipu_di0_sel[] = {
	"di_pred",
	"osc",
	"ckih1",
	"di_pll4_podf",
	"dummy",
	"ldb_di0_div",
};

static const char *mx53_ldb_di0_sel[] = {
	"pll3_sw",
	"pll4_sw",
};

static const char *mx51_ipu_di1_sel[] = {
	"di_pred",
	"osc",
	"ckih1",
	"tve_di",
	"ipp_di1",
};

static const char *mx53_ipu_di1_sel[] = {
	"di_pred",
	"osc",
	"ckih1",
	"tve_di",
	"ipp_di1",
	"ldb_di1_div",
};

static const char *mx53_ldb_di1_sel[] = {
	"pll3_sw",
	"pll4_sw",
};

static const char *mx51_tve_ext_sel[] = {
	"osc",
	"ckih1",
};

static const char *mx53_tve_ext_sel[] = {
	"pll4_sw",
	"ckih1",
};

static const char *mx51_tve_sel[] = {
	"tve_pred",
	"tve_ext_sel",
};

static const char *ipu_sel[] = {
	"axi_a",
	"axi_b",
	"emi_slow_gate",
	"ahb",
};

static void __init mx5_clocks_common_init(struct device_d *dev, void __iomem *base)
{
	writel(0xffffffff, base + CCM_CCGR0);
	writel(0xffffffff, base + CCM_CCGR1);
	writel(0xffffffff, base + CCM_CCGR2);
	writel(0xffffffff, base + CCM_CCGR3);
	writel(0xffffffff, base + CCM_CCGR4);
	writel(0xffffffff, base + CCM_CCGR5);
	writel(0xffffffff, base + CCM_CCGR6);
	writel(0xffffffff, base + CCM_CCGR7);

	if (!IS_ENABLED(CONFIG_COMMON_CLK_OF_PROVIDER) || !dev->device_node) {
		clks[IMX5_CLK_CKIL] = clk_fixed("ckil", 32768);
		clks[IMX5_CLK_OSC] = clk_fixed("osc", 24000000);
	}

	clks[IMX5_CLK_PER_LP_APM] = imx_clk_mux("per_lp_apm", base + CCM_CBCMR, 1, 1,
				per_lp_apm_sel, ARRAY_SIZE(per_lp_apm_sel));
	clks[IMX5_CLK_PER_PRED1] = imx_clk_divider("per_pred1", "per_lp_apm", base + CCM_CBCDR, 6, 2);
	clks[IMX5_CLK_PER_PRED2] = imx_clk_divider("per_pred2", "per_pred1", base + CCM_CBCDR, 3, 3);
	clks[IMX5_CLK_PER_PODF] = imx_clk_divider("per_podf", "per_pred2", base + CCM_CBCDR, 0, 3);
	clks[IMX5_CLK_PER_ROOT] = imx_clk_mux("per_root", base + CCM_CBCMR, 0, 1,
				per_root_sel, ARRAY_SIZE(per_root_sel));
	clks[IMX5_CLK_AHB] = imx_clk_divider("ahb", "main_bus", base + CCM_CBCDR, 10, 3);
	clks[IMX5_CLK_IPG] = imx_clk_divider("ipg", "ahb", base + CCM_CBCDR, 8, 2);
	clks[IMX5_CLK_AXI_A] = imx_clk_divider("axi_a", "main_bus", base + CCM_CBCDR, 16, 3);
	clks[IMX5_CLK_AXI_B] = imx_clk_divider("axi_b", "main_bus", base + CCM_CBCDR, 19, 3);
	clks[IMX5_CLK_UART_SEL] = imx_clk_mux("uart_sel", base + CCM_CSCMR1, 24, 2,
				standard_pll_sel, ARRAY_SIZE(standard_pll_sel));
	clks[IMX5_CLK_UART_PRED] = imx_clk_divider("uart_pred", "uart_sel", base + CCM_CSCDR1, 3, 3);
	clks[IMX5_CLK_UART_ROOT] = imx_clk_divider("uart_root", "uart_pred", base + CCM_CSCDR1, 0, 3);
	clks[IMX5_CLK_ESDHC_A_PRED] = imx_clk_divider("esdhc_a_pred",
		"esdhc_a_sel", base + CCM_CSCDR1, 16, 3);
	clks[IMX5_CLK_ESDHC_A_PODF] = imx_clk_divider("esdhc_a_podf",
		"esdhc_a_pred", base + CCM_CSCDR1, 11, 3);
	clks[IMX5_CLK_ESDHC_B_PRED] = imx_clk_divider("esdhc_b_pred",
		"esdhc_b_sel", base + CCM_CSCDR1, 22, 3);
	clks[IMX5_CLK_ESDHC_B_PODF] = imx_clk_divider("esdhc_b_podf",
		"esdhc_b_pred", base + CCM_CSCDR1, 19, 3);
	clks[IMX5_CLK_ECSPI_SEL] = imx_clk_mux("ecspi_sel", base + CCM_CSCMR1,
		4, 2, standard_pll_sel, ARRAY_SIZE(standard_pll_sel));
	clks[IMX5_CLK_ECSPI_PRED] = imx_clk_divider("ecspi_pred",
		"ecspi_sel", base + CCM_CSCDR2, 25, 3);
	clks[IMX5_CLK_ECSPI_PODF] = imx_clk_divider("ecspi_podf",
		"ecspi_pred", base + CCM_CSCDR2, 19, 6);
	clks[IMX5_CLK_CPU_PODF] = imx_clk_divider("cpu_podf",
		"pll1_sw", base + CCM_CACRR, 0, 3);
}

static void mx5_clocks_mx51_mx53_init(void __iomem *base)
{
	clks[IMX5_CLK_LP_APM] = imx_clk_mux("lp_apm", base + CCM_CCSR, 9, 1,
				lp_apm_sel, ARRAY_SIZE(lp_apm_sel));
	clks[IMX5_CLK_PERIPH_APM] = imx_clk_mux("periph_apm", base + CCM_CBCMR, 12, 2,
				periph_apm_sel, ARRAY_SIZE(periph_apm_sel));
	clks[IMX5_CLK_MAIN_BUS] = imx_clk_mux("main_bus", base + CCM_CBCDR, 25, 1,
				main_bus_sel, ARRAY_SIZE(main_bus_sel));
	clks[IMX5_CLK_ESDHC_A_SEL] = imx_clk_mux("esdhc_a_sel", base + CCM_CSCMR1, 20, 2,
				standard_pll_sel, ARRAY_SIZE(standard_pll_sel));
	clks[IMX5_CLK_ESDHC_B_SEL] = imx_clk_mux("esdhc_b_sel", base + CCM_CSCMR1, 16, 2,
				standard_pll_sel, ARRAY_SIZE(standard_pll_sel));
	clks[IMX5_CLK_ESDHC_C_SEL] = imx_clk_mux("esdhc_c_sel", base + CCM_CSCMR1, 19, 1, esdhc_c_sel, ARRAY_SIZE(esdhc_c_sel));
	clks[IMX5_CLK_ESDHC_D_SEL] = imx_clk_mux("esdhc_d_sel", base + CCM_CSCMR1, 18, 1, esdhc_d_sel, ARRAY_SIZE(esdhc_d_sel));
	clks[IMX5_CLK_EMI_SEL] = imx_clk_mux("emi_sel", base + CCM_CBCDR, 26, 1,
				emi_slow_sel, ARRAY_SIZE(emi_slow_sel));
	clks[IMX5_CLK_EMI_SLOW_PODF] = imx_clk_divider("emi_slow_podf", "emi_sel", base + CCM_CBCDR, 22, 3);
	clks[IMX5_CLK_NFC_PODF] = imx_clk_divider("nfc_podf", "emi_slow_podf", base + CCM_CBCDR, 13, 3);
	clks[IMX5_CLK_USBOH3_SEL] = imx_clk_mux("usboh3_sel", base + CCM_CSCMR1, 22, 2,
				standard_pll_sel, ARRAY_SIZE(standard_pll_sel));
	clks[IMX5_CLK_USBOH3_PRED] = imx_clk_divider("usboh3_pred", "usboh3_sel", base + CCM_CSCDR1, 8, 3);
	clks[IMX5_CLK_USBOH3_PODF] = imx_clk_divider("usboh3_podf", "usboh3_pred", base + CCM_CSCDR1, 6, 2);
	clks[IMX5_CLK_USB_PHY_PRED] = imx_clk_divider("usb_phy_pred", "pll3_sw", base + CCM_CDCDR, 3, 3);
	clks[IMX5_CLK_USB_PHY_PODF] = imx_clk_divider("usb_phy_podf", "usb_phy_pred", base + CCM_CDCDR, 0, 3);
	clks[IMX5_CLK_DI_PRED] = imx_clk_divider_np("di_pred", "pll3_sw", base + CCM_CDCDR, 6, 3);
	clks[IMX5_CLK_USB_PHY_SEL] = imx_clk_mux("usb_phy_sel", base + CCM_CSCMR1, 26, 1,
				usb_phy_sel_str, ARRAY_SIZE(usb_phy_sel_str));
}

static void mx5_clocks_ipu_init(void __iomem *regs)
{
	clks[IMX5_CLK_IPU_SEL]		= imx_clk_mux("ipu_sel", regs + CCM_CBCMR, 6, 2, ipu_sel, ARRAY_SIZE(ipu_sel));
}

static int __init mx50_clocks_init(struct device_d *dev, void __iomem *regs)
{
	clks[IMX5_CLK_PLL1_SW] = imx_clk_pllv2("pll1_sw", "osc",
					       (void *)MX50_PLL1_BASE_ADDR);
	clks[IMX5_CLK_PLL2_SW] = imx_clk_pllv2("pll2_sw", "osc",
					       (void *)MX50_PLL2_BASE_ADDR);
	clks[IMX5_CLK_PLL3_SW] = imx_clk_pllv2("pll3_sw", "osc",
					       (void *)MX50_PLL3_BASE_ADDR);

	mx5_clocks_common_init(dev, regs);

	clks[IMX5_CLK_LP_APM] = imx_clk_mux("lp_apm", regs + CCM_CCSR, 10, 1, lp_apm_sel, ARRAY_SIZE(lp_apm_sel));
	clks[IMX5_CLK_MAIN_BUS] = imx_clk_mux("main_bus", regs + CCM_CBCDR, 25, 2, mx50_periph_clk_sel, ARRAY_SIZE(mx50_periph_clk_sel));
	clks[IMX5_CLK_ESDHC_A_SEL] = imx_clk_mux("esdhc_a_sel", regs + CCM_CSCMR1, 21, 2, standard_pll_sel, ARRAY_SIZE(standard_pll_sel));
	clks[IMX5_CLK_ESDHC_B_SEL] = imx_clk_mux("esdhc_b_sel", regs + CCM_CSCMR1, 16, 3, mx50_3bit_clk_sel, ARRAY_SIZE(mx50_3bit_clk_sel));
	clks[IMX5_CLK_ESDHC_C_SEL] = imx_clk_mux("esdhc_c_sel", regs + CCM_CSCMR1, 20, 1, esdhc_c_sel, ARRAY_SIZE(esdhc_c_sel));
	clks[IMX5_CLK_ESDHC_D_SEL] = imx_clk_mux("esdhc_d_sel", regs + CCM_CSCMR1, 19, 1, esdhc_d_sel, ARRAY_SIZE(esdhc_d_sel));
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX50_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX50_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX50_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX50_I2C1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX50_I2C2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX50_I2C3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX50_GPT1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_IPG], MX50_CSPI_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ECSPI_PODF], MX50_ECSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ECSPI_PODF], MX50_ECSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_IPG], MX50_FEC_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_A_PODF], MX50_ESDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_C_SEL], MX50_ESDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_B_PODF], MX50_ESDHC3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_D_SEL], MX50_ESDHC4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX50_PWM1_BASE_ADDR, "per");
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX50_PWM2_BASE_ADDR, "per");
	clkdev_add_physbase(clks[IMX5_CLK_AHB], MX50_OTG_BASE_ADDR, NULL);

	return 0;
}

static int imx50_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *regs;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	regs = IOMEM(iores->start);

	mx50_clocks_init(dev, regs);

	return 0;
}

static __maybe_unused struct of_device_id imx50_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx50-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx50_ccm_driver = {
	.probe	= imx50_ccm_probe,
	.name	= "imx50-ccm",
	.of_compatible = DRV_OF_COMPAT(imx50_ccm_dt_ids),
};

static void mx51_clocks_ipu_init(void __iomem *regs)
{
	clks[IMX5_CLK_IPU_DI0_SEL]	= imx_clk_mux_p("ipu_di0_sel", regs + CCM_CSCMR2, 26, 3,
						mx51_ipu_di0_sel, ARRAY_SIZE(mx51_ipu_di0_sel));
	clks[IMX5_CLK_IPU_DI1_SEL]	= imx_clk_mux_p("ipu_di1_sel", regs + CCM_CSCMR2, 29, 3,
						mx51_ipu_di1_sel, ARRAY_SIZE(mx51_ipu_di1_sel));
	clks[IMX5_CLK_TVE_EXT_SEL]	= imx_clk_mux_p("tve_ext_sel", regs + CCM_CSCMR1, 6, 1,
						mx51_tve_ext_sel, ARRAY_SIZE(mx51_tve_ext_sel));
	clks[IMX5_CLK_TVE_SEL]		= imx_clk_mux("tve_sel", regs + CCM_CSCMR1, 7, 1,
						mx51_tve_sel, ARRAY_SIZE(mx51_tve_sel));
	clks[IMX5_CLK_TVE_PRED]		= imx_clk_divider("tve_pred", "pll3_sw", regs + CCM_CDCDR, 28, 3);

	mx5_clocks_ipu_init(regs);

	clk_set_rate(clks[IMX5_CLK_DI_PRED], clk_get_rate(clks[IMX5_CLK_PLL3_SW]));

	clkdev_add_physbase(clks[IMX5_CLK_IPU_SEL], MX51_IPU_BASE_ADDR, "bus");
	clkdev_add_physbase(clks[IMX5_CLK_IPU_DI0_SEL], MX51_IPU_BASE_ADDR, "di0");
	clkdev_add_physbase(clks[IMX5_CLK_IPU_DI1_SEL], MX51_IPU_BASE_ADDR, "di1");
}

static int __init mx51_clocks_init(struct device_d *dev, void __iomem *regs)
{
	clks[IMX5_CLK_PLL1_SW] = imx_clk_pllv2("pll1_sw", "osc", (void *)MX51_PLL1_BASE_ADDR);
	clks[IMX5_CLK_PLL2_SW] = imx_clk_pllv2("pll2_sw", "osc", (void *)MX51_PLL2_BASE_ADDR);
	clks[IMX5_CLK_PLL3_SW] = imx_clk_pllv2("pll3_sw", "osc", (void *)MX51_PLL3_BASE_ADDR);

	mx5_clocks_common_init(dev, regs);
	mx5_clocks_mx51_mx53_init(regs);

	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX51_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX51_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX51_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX51_I2C1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX51_I2C2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX51_GPT1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_IPG], MX51_CSPI_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ECSPI_PODF], MX51_ECSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ECSPI_PODF], MX51_ECSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_IPG], MX51_MXC_FEC_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_A_PODF], MX51_MMC_SDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_B_PODF], MX51_MMC_SDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_C_SEL], MX51_MMC_SDHC3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_D_SEL], MX51_MMC_SDHC4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_IPG], MX51_ATA_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX51_PWM1_BASE_ADDR, "per");
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX51_PWM2_BASE_ADDR, "per");

	if (IS_ENABLED(CONFIG_DRIVER_VIDEO_IMX_IPUV3))
		mx51_clocks_ipu_init(regs);

	return 0;
}

static int imx51_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *regs;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	regs = IOMEM(iores->start);

	mx51_clocks_init(dev, regs);

	clk_data.clks = clks;
	clk_data.clk_num = IMX5_CLK_END;
	of_clk_add_provider(dev->device_node, of_clk_src_onecell_get, &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id imx51_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx51-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx51_ccm_driver = {
	.probe	= imx51_ccm_probe,
	.name	= "imx51-ccm",
	.of_compatible = DRV_OF_COMPAT(imx51_ccm_dt_ids),
};

static void mx53_clocks_ipu_init(void __iomem *regs)
{
	clks[IMX5_CLK_LDB_DI1_DIV_3_5]	= imx_clk_fixed_factor("ldb_di1_div_3_5", "ldb_di1_sel", 2, 7);
	clks[IMX5_CLK_LDB_DI1_DIV]	= imx_clk_divider_np("ldb_di1_div", "ldb_di1_div_3_5", regs + CCM_CSCMR2, 11, 1);
	clks[IMX5_CLK_LDB_DI1_SEL]	= imx_clk_mux_p("ldb_di1_sel", regs + CCM_CSCMR2, 9, 1,
						mx53_ldb_di1_sel, ARRAY_SIZE(mx53_ldb_di1_sel));
	clks[IMX5_CLK_DI_PLL4_PODF]	= imx_clk_divider("di_pll4_podf", "pll4_sw", regs + CCM_CDCDR, 16, 3);
	clks[IMX5_CLK_LDB_DI0_DIV_3_5]	= imx_clk_fixed_factor("ldb_di0_div_3_5", "ldb_di0_sel", 2, 7);
	clks[IMX5_CLK_LDB_DI0_DIV]	= imx_clk_divider("ldb_di0_div", "ldb_di0_div_3_5", regs + CCM_CSCMR2, 10, 1);
	clks[IMX5_CLK_LDB_DI0_SEL]	= imx_clk_mux_p("ldb_di0_sel", regs + CCM_CSCMR2, 8, 1,
						mx53_ldb_di0_sel, ARRAY_SIZE(mx53_ldb_di0_sel));
	clks[IMX5_CLK_IPU_DI0_SEL]	= imx_clk_mux_p("ipu_di0_sel", regs + CCM_CSCMR2, 26, 3,
						mx53_ipu_di0_sel, ARRAY_SIZE(mx53_ipu_di0_sel));
	clks[IMX5_CLK_IPU_DI1_SEL]	= imx_clk_mux_p("ipu_di1_sel", regs + CCM_CSCMR2, 29, 3,
						mx53_ipu_di1_sel, ARRAY_SIZE(mx53_ipu_di1_sel));
	clks[IMX5_CLK_TVE_EXT_SEL]	= imx_clk_mux_p("tve_ext_sel", regs + CCM_CSCMR1, 6, 1,
						mx53_tve_ext_sel, ARRAY_SIZE(mx53_tve_ext_sel));
	clks[IMX5_CLK_TVE_PRED]		= imx_clk_divider("tve_pred", "tve_ext_sel", regs + CCM_CDCDR, 28, 3);

	mx5_clocks_ipu_init(regs);

	clkdev_add_physbase(clks[IMX5_CLK_IPU_SEL], MX53_IPU_BASE_ADDR, "bus");
	clkdev_add_physbase(clks[IMX5_CLK_IPU_DI0_SEL], MX53_IPU_BASE_ADDR, "di0");
	clkdev_add_physbase(clks[IMX5_CLK_IPU_DI1_SEL], MX53_IPU_BASE_ADDR, "di1");
}

static int __init mx53_clocks_init(struct device_d *dev, void __iomem *regs)
{
	clks[IMX5_CLK_PLL1_SW] = imx_clk_pllv2("pll1_sw", "osc", (void *)MX53_PLL1_BASE_ADDR);
	clks[IMX5_CLK_PLL2_SW] = imx_clk_pllv2("pll2_sw", "osc", (void *)MX53_PLL2_BASE_ADDR);
	clks[IMX5_CLK_PLL3_SW] = imx_clk_pllv2("pll3_sw", "osc", (void *)MX53_PLL3_BASE_ADDR);
	clks[IMX5_CLK_PLL4_SW] = imx_clk_pllv2("pll4_sw", "osc", (void *)MX53_PLL4_BASE_ADDR);

	mx5_clocks_common_init(dev, regs);
	mx5_clocks_mx51_mx53_init(regs);

	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX53_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX53_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX53_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX53_UART4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_UART_ROOT], MX53_UART5_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX53_I2C1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX53_I2C2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX53_I2C3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX53_GPT1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_IPG], MX53_CSPI_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ECSPI_PODF], MX53_ECSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ECSPI_PODF], MX53_ECSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_IPG], MX53_FEC_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_A_PODF], MX53_ESDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_C_SEL], MX53_ESDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_B_PODF], MX53_ESDHC3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_ESDHC_D_SEL], MX53_ESDHC4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_AHB], MX53_SATA_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX53_PWM1_BASE_ADDR, "per");
	clkdev_add_physbase(clks[IMX5_CLK_PER_ROOT], MX53_PWM2_BASE_ADDR, "per");

	if (IS_ENABLED(CONFIG_DRIVER_VIDEO_IMX_IPUV3))
		mx53_clocks_ipu_init(regs);

	return 0;
}

static int imx53_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *regs;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	regs = IOMEM(iores->start);

	mx53_clocks_init(dev, regs);

	clk_data.clks = clks;
	clk_data.clk_num = IMX5_CLK_END;
	of_clk_add_provider(dev->device_node, of_clk_src_onecell_get, &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id imx53_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx53-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx53_ccm_driver = {
	.probe	= imx53_ccm_probe,
	.name	= "imx53-ccm",
	.of_compatible = DRV_OF_COMPAT(imx53_ccm_dt_ids),
};

static int imx5_ccm_init(void)
{
	if (IS_ENABLED(CONFIG_ARCH_IMX50))
		platform_driver_register(&imx50_ccm_driver);
	if (IS_ENABLED(CONFIG_ARCH_IMX51))
		platform_driver_register(&imx51_ccm_driver);
	if (IS_ENABLED(CONFIG_ARCH_IMX53))
		platform_driver_register(&imx53_ccm_driver);

	return 0;
}
core_initcall(imx5_ccm_init);
