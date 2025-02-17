// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2022 Jules Maselbas

#include <common.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/units.h>
#include <linux/err.h>

#include "clk-sunxi.h"

#include "clk-sun50i-a64.h"

#define CCU_PLL_CPUX		0x00
#define CCU_PLL_PERIPH0		0x28
#define CCU_CPUX_AXI_CFG	0x50

static struct clk *clks[CLK_NUMBER];
static struct clk_onecell_data clk_data = {
	.clks = clks,
	.clk_num = ARRAY_SIZE(clks),
};

static struct clk_div_table div_apb1[] = {
	{ .val = 0, .div = 2 },
	{ .val = 1, .div = 2 },
	{ .val = 2, .div = 4 },
	{ .val = 3, .div = 8 },
	{ /* Sentinel */ }
};

static const char *sel_cpux[] = {
	"osc32k",
	"osc24M",
	"pll-cpux",
};

static const char *sel_ahb1[] = {
	"osc32k",
	"osc24M",
	"axi",
	"pll-periph0",
};

static const char *sel_apb2[] = {
	"osc32k",
	"osc24M",
	"pll-periph0-2x",
	"pll-periph0-2x",
};

static const char *sel_ahb2[] = {
	"ahb1",
	"pll-periph0",
};

static const char *sel_mmc[] = {
	"osc24M",
	"pll-periph0-2x",
	"pll-periph1-2x",
};

static void sun50i_a64_resets_init(void __iomem *regs)
{
	u32 rst;

	rst = 0 |
		/* RST_USB_PHY0 */ BIT(0) |
		/* RST_USB_PHY1 */ BIT(1) |
		/* RST_USB_HSIC */ BIT(2);
	writel(rst, regs + 0x0cc);

	rst = 0 |
		/* RST_BUS_MIPI_DSI */ BIT(1) |
		/* RST_BUS_CE   */ BIT(5) |
		/* RST_BUS_DMA  */ BIT(6) |
		/* RST_BUS_MMC0 */ BIT(8) |
		/* RST_BUS_MMC1 */ BIT(9) |
		/* RST_BUS_MMC2 */ BIT(10) |
		/* RST_BUS_NAND */ BIT(13) |
		/* RST_BUS_DRAM */ BIT(14) |
		/* RST_BUS_EMAC */ BIT(17) |
		/* RST_BUS_TS   */ BIT(18) |
		/* RST_BUS_HSTIMER */ BIT(19) |
		/* RST_BUS_SPI0  */ BIT(20) |
		/* RST_BUS_SPI1  */ BIT(21) |
		/* RST_BUS_OTG   */ BIT(23) |
		/* RST_BUS_EHCI0 */ BIT(24) |
		/* RST_BUS_EHCI1 */ BIT(25) |
		/* RST_BUS_OHCI0 */ BIT(28) |
		/* RST_BUS_OHCI1 */ BIT(29);
	writel(rst, regs + 0x2c0);

	rst = 0 |
		/* RST_BUS_VE    */ BIT(0) |
		/* RST_BUS_TCON0 */ BIT(3) |
		/* RST_BUS_TCON1 */ BIT(4) |
		/* RST_BUS_DEINTERLACE */ BIT(5) |
		/* RST_BUS_CSI   */ BIT(8) |
		/* RST_BUS_HDMI0 */ BIT(10) |
		/* RST_BUS_HDMI1 */ BIT(11) |
		/* RST_BUS_DE    */ BIT(12) |
		/* RST_BUS_GPU   */ BIT(20) |
		/* RST_BUS_MSGBOX   */ BIT(21) |
		/* RST_BUS_SPINLOCK */ BIT(22) |
		/* RST_BUS_DBG */ BIT(31);
	writel(rst, regs + 0x2c4);

	rst = /* RST_BUS_LVDS */ BIT(0);
	writel(rst, regs + 0x2c8);

	rst = 0 |
		/* RST_BUS_CODEC */ BIT(0) |
		/* RST_BUS_SPDIF */ BIT(1) |
		/* RST_BUS_THS   */ BIT(8) |
		/* RST_BUS_I2S0  */ BIT(12) |
		/* RST_BUS_I2S1  */ BIT(13) |
		/* RST_BUS_I2S2  */ BIT(14);
	writel(rst, regs + 0x2d0);

	rst = 0 |
		/* RST_BUS_I2C0 */ BIT(0) |
		/* RST_BUS_I2C1 */ BIT(1) |
		/* RST_BUS_I2C2 */ BIT(2) |
		/* RST_BUS_SCR  */ BIT(5) |
		/* RST_BUS_UART0 */ BIT(16) |
		/* RST_BUS_UART1 */ BIT(17) |
		/* RST_BUS_UART2 */ BIT(18) |
		/* RST_BUS_UART3 */ BIT(19) |
		/* RST_BUS_UART4 */ BIT(20);
	writel(rst, regs + 0x2d8);
}

static inline void sunxi_clk_set_pll(void __iomem *reg, u32 src, u32 freq)
{
	/* NOTE: using u32, max freq is 4GHz
	 * out freq: src * N * K
	 * factor N: [1->32]
	 * factor K: [1->4]
	 * from the manual: give priority to the choice of K >= 2
	 */
	u32 mul = freq / src; /* target multiplier (N * K) */
	u32 k, n;
	u32 cfg = BIT(31); /* ENABLE */

	for (k = 4; k > 1; k--) {
		if ((mul % k) == 0)
			break;
	}
	n = mul / k;

	cfg |= (k - 1) << 4;
	cfg |= (n - 1) << 8;

	writel(cfg, reg);

	/* wait for pll lock */
	while (!(readl(reg) & BIT(28)));
}

static void sun50i_a64_clocks_init(void __iomem *regs)
{
	/* switch cpu clock source to osc-24M */
	writel(0x10000, regs + CCU_CPUX_AXI_CFG);
	/* wait 8 cycles */
	nop_delay(8);
	/* set pll-cpux to 816MHz */
	sunxi_clk_set_pll(regs + CCU_PLL_CPUX, 24 * HZ_PER_MHZ, 816 * HZ_PER_MHZ);
	nop_delay(10000); /* HACK: ~1ms delay  */
	/* switch cpu clock source to pll-cpux*/
	writel( /* clk_src: 1=24Mhz 2=pll-cpux */ (2 << 16) |
		/* apb_div: /2 */ (1 << 8) |
		/* axi_div: /2 */ (1 << 0),
		regs + CCU_CPUX_AXI_CFG);
	/* wait 8 cycles */
	nop_delay(8);

	clks[CLK_PLL_CPUX] = clk_fixed("pll-cpux", 816 * HZ_PER_MHZ);
	clks[CLK_CPUX] = sunxi_clk_mux("cpux", sel_cpux, ARRAY_SIZE(sel_cpux), regs + CCU_CPUX_AXI_CFG, 16, 2);

	/* set pll-periph0-2x to 1.2GHz, as recommended */
	sunxi_clk_set_pll(regs + CCU_PLL_PERIPH0, 24 * HZ_PER_MHZ, 1200 * HZ_PER_MHZ);

	clks[CLK_PLL_PERIPH0_2X] = clk_fixed("pll-periph0-2x", 1200 * HZ_PER_MHZ);
	clks[CLK_PLL_PERIPH0]    = clk_fixed_factor("pll-periph0", "pll-periph0-2x", 1, 2, 0);

	clks[CLK_AXI] = sunxi_clk_div("axi", "cpux", regs + CCU_CPUX_AXI_CFG, 0, 2);

	clks[CLK_AHB1] = sunxi_clk_mux("ahb1", sel_ahb1, ARRAY_SIZE(sel_ahb1), regs + 0x054, 12, 2);
	clks[CLK_AHB2] = sunxi_clk_mux("ahb2", sel_ahb2, ARRAY_SIZE(sel_ahb2), regs + 0x05c, 0, 1);

	clks[CLK_APB1] = sunxi_clk_div_table("apb1", "ahb1", div_apb1, regs + 0x054, 8, 2);
	clks[CLK_APB2] = sunxi_clk_mux("apb2", sel_apb2, ARRAY_SIZE(sel_apb2), regs + 0x058, 24, 2);

	clks[CLK_BUS_MIPI_DSI] = sunxi_clk_gate("bus-mipi-dsi", "ahb1", regs + 0x060, 1);
	clks[CLK_BUS_CE]    = sunxi_clk_gate("bus-ce",    "ahb1", regs + 0x060, 5);
	clks[CLK_BUS_DMA]   = sunxi_clk_gate("bus-dma",   "ahb1", regs + 0x060, 6);
	clks[CLK_BUS_MMC0]  = sunxi_clk_gate("bus-mmc0",  "ahb1", regs + 0x060, 8);
	clks[CLK_BUS_MMC1]  = sunxi_clk_gate("bus-mmc1",  "ahb1", regs + 0x060, 9);
	clks[CLK_BUS_MMC2]  = sunxi_clk_gate("bus-mmc2",  "ahb1", regs + 0x060, 10);
	clks[CLK_BUS_NAND]  = sunxi_clk_gate("bus-nand",  "ahb1", regs + 0x060, 13);
	clks[CLK_BUS_DRAM]  = sunxi_clk_gate("bus-dram",  "ahb1", regs + 0x060, 14);
	clks[CLK_BUS_EMAC]  = sunxi_clk_gate("bus-emac",  "ahb2", regs + 0x060, 17);
	clks[CLK_BUS_TS]    = sunxi_clk_gate("bus-ts",    "ahb1", regs + 0x060, 18);
	clks[CLK_BUS_HSTIMER] = sunxi_clk_gate("bus-hstimer", "ahb1", regs + 0x060, 19);
	clks[CLK_BUS_SPI0]  = sunxi_clk_gate("bus-spi0",  "ahb1", regs + 0x060,  20);
	clks[CLK_BUS_SPI1]  = sunxi_clk_gate("bus-spi1",  "ahb1", regs + 0x060,  21);
	clks[CLK_BUS_OTG]   = sunxi_clk_gate("bus-otg",   "ahb1", regs + 0x060,  23);
	clks[CLK_BUS_EHCI0] = sunxi_clk_gate("bus-ehci0", "ahb1", regs + 0x060, 24);
	clks[CLK_BUS_EHCI1] = sunxi_clk_gate("bus-ehci1", "ahb2", regs + 0x060, 25);
	clks[CLK_BUS_OHCI0] = sunxi_clk_gate("bus-ohci0", "ahb1", regs + 0x060, 28);
	clks[CLK_BUS_OHCI1] = sunxi_clk_gate("bus-ohci1", "ahb2", regs + 0x060, 29);

	clks[CLK_BUS_CODEC] = sunxi_clk_gate("bus-codec", "apb1", regs + 0x068, 0);
	clks[CLK_BUS_SPDIF] = sunxi_clk_gate("bus-spdif", "apb1", regs + 0x068, 1);
	clks[CLK_BUS_PIO]   = sunxi_clk_gate("bus-pio",   "apb1", regs + 0x068, 5);
	clks[CLK_BUS_THS]   = sunxi_clk_gate("bus-ths",   "apb1", regs + 0x068, 8);
	clks[CLK_BUS_I2S0]  = sunxi_clk_gate("bus-i2s0",  "apb1", regs + 0x068, 12);
	clks[CLK_BUS_I2S1]  = sunxi_clk_gate("bus-i2s1",  "apb1", regs + 0x068, 13);
	clks[CLK_BUS_I2S2]  = sunxi_clk_gate("bus-i2s2",  "apb1", regs + 0x068, 14);

	clks[CLK_BUS_UART0] = sunxi_clk_gate("bus-uart0", "apb2", regs + 0x06c, 16);
	clks[CLK_BUS_UART1] = sunxi_clk_gate("bus-uart1", "apb2", regs + 0x06c, 17);
	clks[CLK_BUS_UART2] = sunxi_clk_gate("bus-uart2", "apb2", regs + 0x06c, 18);
	clks[CLK_BUS_UART3] = sunxi_clk_gate("bus-uart3", "apb2", regs + 0x06c, 19);
	clks[CLK_BUS_UART4] = sunxi_clk_gate("bus-uart4", "apb2", regs + 0x06c, 20);

	clks[CLK_MMC0] = sunxi_ccu_clk(
		"mmc0", sel_mmc, ARRAY_SIZE(sel_mmc),
		regs, 0x088,
		(struct ccu_clk_mux) { .shift = 24, .width = 2 },
		(const struct ccu_clk_div[4]) {
			{ .type = CCU_CLK_DIV_P,     .shift = 16, .width = 2, .off = 0 },
			{ .type = CCU_CLK_DIV_M,     .shift =  0, .width = 4, .off = 0 },
			{ .type = CCU_CLK_DIV_FIXED, .shift =  0, .width = 0, .off = 2 },
		},
		3,
		BIT(31),
		0);

	clks[CLK_MMC1] = sunxi_ccu_clk(
		"mmc1", sel_mmc, ARRAY_SIZE(sel_mmc),
		regs, 0x08c,
		(struct ccu_clk_mux) { .shift = 24, .width = 2 },
		(const struct ccu_clk_div[4]) {
			{ .type = CCU_CLK_DIV_P,     .shift = 16, .width = 2, .off = 0 },
			{ .type = CCU_CLK_DIV_M,     .shift =  0, .width = 4, .off = 0 },
			{ .type = CCU_CLK_DIV_FIXED, .shift =  0, .width = 0, .off = 2 },
		},
		3,
		BIT(31),
		0);

	clks[CLK_MMC2] = sunxi_ccu_clk(
		"mmc2", sel_mmc, ARRAY_SIZE(sel_mmc),
		regs, 0x090,
		(struct ccu_clk_mux) { .shift = 24, .width = 2 },
		(const struct ccu_clk_div[4]) {
			{ .type = CCU_CLK_DIV_P,     .shift = 16, .width = 2, .off = 0 },
			{ .type = CCU_CLK_DIV_M,     .shift =  0, .width = 4, .off = 0 },
			{ .type = CCU_CLK_DIV_FIXED, .shift =  0, .width = 0, .off = 2 },
		},
		3,
		BIT(31),
		0);
}

static int sun50i_a64_ccu_probe(struct device *dev)
{
	struct resource *iores;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	sun50i_a64_resets_init(IOMEM(iores->start));
	sun50i_a64_clocks_init(IOMEM(iores->start));

	return of_clk_add_provider(dev->of_node, of_clk_src_onecell_get, &clk_data);
}

static __maybe_unused struct of_device_id sun50i_a64_ccu_dt_ids[] = {
	{ .compatible = "allwinner,sun50i-a64-ccu" },
	{ }
};

static struct driver sun50i_a64_ccu_driver = {
	.probe	= sun50i_a64_ccu_probe,
	.name	= "sun50i-a64-ccu",
	.of_compatible = DRV_OF_COMPAT(sun50i_a64_ccu_dt_ids),
};
core_platform_driver(sun50i_a64_ccu_driver);
