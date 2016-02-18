#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/imx27-regs.h>
#include <mach/generic.h>
#include <mach/revision.h>

#include "clk.h"

/* Register offsets */
#define CCM_CSCR		0x0
#define CCM_MPCTL0		0x4
#define CCM_MPCTL1		0x8
#define CCM_SPCTL0		0xc
#define CCM_SPCTL1		0x10
#define CCM_OSC26MCTL		0x14
#define CCM_PCDR0		0x18
#define CCM_PCDR1		0x1c
#define CCM_PCCR0		0x20
#define CCM_PCCR1		0x24
#define CCM_CCSR		0x28
#define CCM_PMCTL		0x2c
#define CCM_PMCOUNT		0x30
#define CCM_WKGDCTL		0x34

#define PCCR0_SSI2_EN	(1 << 0)
#define PCCR0_SSI1_EN	(1 << 1)
#define PCCR0_SLCDC_EN	(1 << 2)
#define PCCR0_SDHC3_EN	(1 << 3)
#define PCCR0_SDHC2_EN	(1 << 4)
#define PCCR0_SDHC1_EN	(1 << 5)
#define PCCR0_SDC_EN	(1 << 6)
#define PCCR0_SAHARA_EN	(1 << 7)
#define PCCR0_RTIC_EN	(1 << 8)
#define PCCR0_RTC_EN	(1 << 9)
#define PCCR0_PWM_EN	(1 << 11)
#define PCCR0_OWIRE_EN	(1 << 12)
#define PCCR0_MSHC_EN	(1 << 13)
#define PCCR0_LCDC_EN	(1 << 14)
#define PCCR0_KPP_EN	(1 << 15)
#define PCCR0_IIM_EN	(1 << 16)
#define PCCR0_I2C2_EN	(1 << 17)
#define PCCR0_I2C1_EN	(1 << 18)
#define PCCR0_GPT6_EN	(1 << 19)
#define PCCR0_GPT5_EN	(1 << 20)
#define PCCR0_GPT4_EN	(1 << 21)
#define PCCR0_GPT3_EN	(1 << 22)
#define PCCR0_GPT2_EN	(1 << 23)
#define PCCR0_GPT1_EN	(1 << 24)
#define PCCR0_GPIO_EN	(1 << 25)
#define PCCR0_FEC_EN	(1 << 26)
#define PCCR0_EMMA_EN	(1 << 27)
#define PCCR0_DMA_EN	(1 << 28)
#define PCCR0_CSPI3_EN	(1 << 29)
#define PCCR0_CSPI2_EN	(1 << 30)
#define PCCR0_CSPI1_EN	(1 << 31)

#define PCCR1_MSHC_BAUDEN	(1 << 2)
#define PCCR1_NFC_BAUDEN	(1 << 3)
#define PCCR1_SSI2_BAUDEN	(1 << 4)
#define PCCR1_SSI1_BAUDEN	(1 << 5)
#define PCCR1_H264_BAUDEN	(1 << 6)
#define PCCR1_PERCLK4_EN	(1 << 7)
#define PCCR1_PERCLK3_EN	(1 << 8)
#define PCCR1_PERCLK2_EN	(1 << 9)
#define PCCR1_PERCLK1_EN	(1 << 10)
#define PCCR1_HCLK_USB		(1 << 11)
#define PCCR1_HCLK_SLCDC	(1 << 12)
#define PCCR1_HCLK_SAHARA	(1 << 13)
#define PCCR1_HCLK_RTIC		(1 << 14)
#define PCCR1_HCLK_LCDC		(1 << 15)
#define PCCR1_HCLK_H264		(1 << 16)
#define PCCR1_HCLK_FEC		(1 << 17)
#define PCCR1_HCLK_EMMA		(1 << 18)
#define PCCR1_HCLK_EMI		(1 << 19)
#define PCCR1_HCLK_DMA		(1 << 20)
#define PCCR1_HCLK_CSI		(1 << 21)
#define PCCR1_HCLK_BROM		(1 << 22)
#define PCCR1_HCLK_ATA		(1 << 23)
#define PCCR1_WDT_EN		(1 << 24)
#define PCCR1_USB_EN		(1 << 25)
#define PCCR1_UART6_EN		(1 << 26)
#define PCCR1_UART5_EN		(1 << 27)
#define PCCR1_UART4_EN		(1 << 28)
#define PCCR1_UART3_EN		(1 << 29)
#define PCCR1_UART2_EN		(1 << 30)
#define PCCR1_UART1_EN		(1 << 31)

enum mx27_clks {
	dummy, ckih, ckil, mpll, spll, mpll_main2, ahb, ipg, nfc_div, per1_div,
	per2_div, per3_div, per4_div, vpu_sel, vpu_div, usb_div, cpu_sel,
	clko_sel, cpu_div, clko_div, ssi1_sel, ssi2_sel, ssi1_div, ssi2_div,
	clko_en, ssi2_ipg_gate, ssi1_ipg_gate, slcdc_ipg_gate, sdhc3_ipg_gate,
	sdhc2_ipg_gate, sdhc1_ipg_gate, scc_ipg_gate, sahara_ipg_gate,
	rtc_ipg_gate, pwm_ipg_gate, owire_ipg_gate, lcdc_ipg_gate,
	kpp_ipg_gate, iim_ipg_gate, i2c2_ipg_gate, i2c1_ipg_gate,
	gpt6_ipg_gate, gpt5_ipg_gate, gpt4_ipg_gate, gpt3_ipg_gate,
	gpt2_ipg_gate, gpt1_ipg_gate, gpio_ipg_gate, fec_ipg_gate,
	emma_ipg_gate, dma_ipg_gate, cspi3_ipg_gate, cspi2_ipg_gate,
	cspi1_ipg_gate, nfc_baud_gate, ssi2_baud_gate, ssi1_baud_gate,
	vpu_baud_gate, per4_gate, per3_gate, per2_gate, per1_gate,
	usb_ahb_gate, slcdc_ahb_gate, sahara_ahb_gate, lcdc_ahb_gate,
	vpu_ahb_gate, fec_ahb_gate, emma_ahb_gate, emi_ahb_gate, dma_ahb_gate,
	csi_ahb_gate, brom_ahb_gate, ata_ahb_gate, wdog_ipg_gate, usb_ipg_gate,
	uart6_ipg_gate, uart5_ipg_gate, uart4_ipg_gate, uart3_ipg_gate,
	uart2_ipg_gate, uart1_ipg_gate, ckih_div1p5, fpm, mpll_osc_sel,
	mpll_sel, spll_gate, clk_max
};

static struct clk *clks[clk_max];

static const char *cpu_sel_clks[] = {
	"mpll_main2",
	"mpll",
};

static const char *mpll_sel_clks[] = {
	"fpm",
	"mpll_osc_sel",
};

static const char *mpll_osc_sel_clks[] = {
	"ckih",
	"ckih_div1p5",
};

static const char *clko_sel_clks[] = {
	"ckil",
	NULL,
	"ckih",
	"ckih",
	"ckih",
	"mpll",
	"spll",
	"cpu_div",
	"ahb",
	"ipg",
	"per1_div",
	"per2_div",
	"per3_div",
	"per4_div",
	NULL,
	NULL,
	"nfc_div",
	NULL,
	NULL,
	NULL,
	"ckil",
	"usb_div",
	NULL,
};

static int imx27_ccm_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	writel(PCCR0_SDHC3_EN | PCCR0_SDHC2_EN | PCCR0_SDHC1_EN |
			PCCR0_PWM_EN | PCCR0_KPP_EN | PCCR0_IIM_EN |
			PCCR0_I2C2_EN | PCCR0_I2C1_EN | PCCR0_GPT6_EN | PCCR0_GPT5_EN |
			PCCR0_GPT4_EN | PCCR0_GPT3_EN | PCCR0_GPT2_EN | PCCR0_GPT1_EN |
			PCCR0_GPIO_EN | PCCR0_FEC_EN | PCCR0_CSPI3_EN | PCCR0_CSPI2_EN |
			PCCR0_CSPI1_EN,
			base + CCM_PCCR0);

	writel(PCCR1_NFC_BAUDEN | PCCR1_PERCLK4_EN | PCCR1_PERCLK2_EN | PCCR1_PERCLK1_EN |
			PCCR1_HCLK_USB | PCCR1_HCLK_FEC | PCCR1_HCLK_EMI | PCCR1_WDT_EN |
			PCCR1_USB_EN | PCCR1_UART6_EN | PCCR1_UART5_EN | PCCR1_UART4_EN |
			PCCR1_UART3_EN | PCCR1_UART2_EN | PCCR1_UART1_EN,
			base + CCM_PCCR1);

	clks[dummy] = clk_fixed("dummy", 0);
	clks[ckih] = clk_fixed("ckih", 26000000);
	clks[ckil] = clk_fixed("ckil", 32768);
	clks[fpm] = imx_clk_fixed_factor("fpm", "ckil", 1024, 1);
	clks[ckih_div1p5] = imx_clk_fixed_factor("ckih_div1p5", "ckih", 2, 3);

	clks[mpll_osc_sel] = imx_clk_mux("mpll_osc_sel", base + CCM_CSCR, 4, 1,
			mpll_osc_sel_clks,
			ARRAY_SIZE(mpll_osc_sel_clks));
	clks[mpll_sel] = imx_clk_mux("mpll_sel", base + CCM_CSCR, 16, 1, mpll_sel_clks,
			ARRAY_SIZE(mpll_sel_clks));

	clks[mpll] = imx_clk_pllv1("mpll", "mpll_sel", base + CCM_MPCTL0);
	clks[spll] = imx_clk_pllv1("spll", "ckih", base + CCM_SPCTL0);
	clks[mpll_main2] = imx_clk_fixed_factor("mpll_main2", "mpll", 2, 3);

	if (imx_silicon_revision() >= IMX_CHIP_REV_2_0) {
		clks[ahb] = imx_clk_divider("ahb", "mpll_main2", base + CCM_CSCR, 8, 2);
		clks[ipg] = imx_clk_fixed_factor("ipg", "ahb", 1, 2);
	} else {
		clks[ahb] = imx_clk_divider("ahb", "mpll_main2", base + CCM_CSCR, 9, 4);
		clks[ipg] = imx_clk_divider("ipg", "ahb", base + CCM_CSCR, 8, 1);
	}

	clks[nfc_div] = imx_clk_divider("nfc_div", "ahb", base + CCM_PCDR0, 6, 4);
	clks[per1_div] = imx_clk_divider("per1_div", "mpll_main2", base + CCM_PCDR1, 0, 6);
	clks[per2_div] = imx_clk_divider("per2_div", "mpll_main2", base + CCM_PCDR1, 8, 6);
	clks[per3_div] = imx_clk_divider("per3_div", "mpll_main2", base + CCM_PCDR1, 16, 6);
	clks[per4_div] = imx_clk_divider("per4_div", "mpll_main2", base + CCM_PCDR1, 24, 6);
	clks[usb_div] = imx_clk_divider("usb_div", "spll", base + CCM_CSCR, 28, 3);
	clks[cpu_sel] = imx_clk_mux("cpu_sel", base + CCM_CSCR, 15, 1, cpu_sel_clks,
			ARRAY_SIZE(cpu_sel_clks));
	clks[clko_sel] = imx_clk_mux("clko_sel", base + CCM_CCSR, 0, 5, clko_sel_clks,
			ARRAY_SIZE(clko_sel_clks));
	if (imx_silicon_revision() >= IMX_CHIP_REV_2_0)
		clks[cpu_div] = imx_clk_divider("cpu_div", "cpu_sel", base + CCM_CSCR, 12, 2);
	else
		clks[cpu_div] = imx_clk_divider("cpu_div", "cpu_sel", base + CCM_CSCR, 13, 3);
	clks[clko_div] = imx_clk_divider("clko_div", "clko_sel", base + CCM_PCDR0, 22, 3);
	clks[per3_gate] = imx_clk_gate("per3_gate", "per3_div", base + CCM_PCCR1, 8);
	clks[lcdc_ahb_gate] = imx_clk_gate("lcdc_ahb_gate", "ahb", base + CCM_PCCR1, 15);
	clks[lcdc_ipg_gate] = imx_clk_gate("lcdc_ipg_gate", "ipg", base + CCM_PCCR0, 14);

	clkdev_add_physbase(clks[per1_div], MX27_GPT1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_GPT2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_GPT3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_GPT4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_GPT5_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_GPT6_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_UART1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_UART2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_UART3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_UART4_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_UART5_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per1_div], MX27_UART6_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX27_CSPI1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX27_CSPI2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX27_CSPI3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX27_I2C1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[ipg], MX27_I2C2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2_div], MX27_SDHC1_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2_div], MX27_SDHC2_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per2_div], MX27_SDHC3_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[per3_gate], MX27_LCDC_BASE_ADDR, "per");
	clkdev_add_physbase(clks[lcdc_ahb_gate], MX27_LCDC_BASE_ADDR, "ahb");
	clkdev_add_physbase(clks[lcdc_ipg_gate], MX27_LCDC_BASE_ADDR, "ipg");
	clkdev_add_physbase(clks[ipg], MX27_FEC_BASE_ADDR, NULL);

	return 0;
}

static __maybe_unused struct of_device_id imx27_ccm_dt_ids[] = {
	{
		.compatible = "fsl,imx27-ccm",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx27_ccm_driver = {
	.probe	= imx27_ccm_probe,
	.name	= "imx27-ccm",
	.of_compatible = DRV_OF_COMPAT(imx27_ccm_dt_ids),
};

static int imx27_ccm_init(void)
{
	return platform_driver_register(&imx27_ccm_driver);
}
core_initcall(imx27_ccm_init);
