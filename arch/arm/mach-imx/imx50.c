/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <init.h>
#include <common.h>
#include <io.h>
#include <notifier.h>
#include <linux/sizes.h>
#include <mach/imx5.h>
#include <mach/imx50-regs.h>
#include <mach/revision.h>
#include <mach/clock-imx51_53.h>
#include <mach/generic.h>
#include <mach/reset-reason.h>
#include <mach/usb.h>

#define SI_REV 0x48

static int imx50_silicon_revision(void)
{
	void __iomem *rom = MX50_IROM_BASE_ADDR;
	u32 rev;
	u32 mx50_silicon_revision;

	rev = readl(rom + SI_REV);
	switch (rev) {
	case 0x10:
		mx50_silicon_revision = IMX_CHIP_REV_1_0;
		break;
	case 0x11:
		mx50_silicon_revision = IMX_CHIP_REV_1_1;
		break;
	default:
		mx50_silicon_revision = IMX_CHIP_REV_UNKNOWN;
	}

	imx_set_silicon_revision("i.MX50", mx50_silicon_revision);

	return 0;
}

int imx50_init(void)
{
	void __iomem *src = IOMEM(MX50_SRC_BASE_ADDR);

	imx50_silicon_revision();
	imx_set_reset_reason(src + IMX_SRC_SRSR, imx_reset_reasons);
	imx53_boot_save_loc();

	return 0;
}

int imx50_devices_init(void)
{
	add_generic_device("imx-iomuxv3", 0, NULL, MX50_IOMUXC_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx50-ccm", 0, NULL, MX50_CCM_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpt", 0, NULL, MX50_GPT1_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 0, NULL, MX50_GPIO1_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 1, NULL, MX50_GPIO2_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 2, NULL, MX50_GPIO3_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 3, NULL, MX50_GPIO4_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 4, NULL, MX50_GPIO5_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 5, NULL, MX50_GPIO6_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx21-wdt", 0, NULL, MX50_WDOG1_BASE_ADDR,
		0x1000, IORESOURCE_MEM, NULL);

	return 0;
}

static void imx50_init_lowlevel_early(unsigned int cpufreq_mhz)
{
	void __iomem *ccm = IOMEM(MX50_CCM_BASE_ADDR);
	u32 r;

	if ((readl(ccm + MX5_CCM_CCGR2) & MX5_CCM_CCGRx_CG13_MASK))
		imx_reset_otg_controller(IOMEM(MX50_OTG_BASE_ADDR));

	imx5_init_lowlevel();

	/*
	 * AIPS setup - Only setup MPROTx registers.
	 * The PACR default values are good.
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, MX50_AIPS1_BASE_ADDR + 0);
	writel(0x77777777, MX50_AIPS1_BASE_ADDR + 4);
	writel(0x77777777, MX50_AIPS2_BASE_ADDR + 0);
	writel(0x77777777, MX50_AIPS2_BASE_ADDR + 4);

	/* Gate of clocks to the peripherals first */
	writel(0x3fffffff, ccm + MX5_CCM_CCGR0);
	writel(0x00000000, ccm + MX5_CCM_CCGR1);
	writel(0x00000000, ccm + MX5_CCM_CCGR2);
	writel(0x00000000, ccm + MX5_CCM_CCGR3);
	writel(0x00030000, ccm + MX5_CCM_CCGR4);
	writel(0x00fff030, ccm + MX5_CCM_CCGR5);
	writel(0x0f00030f, ccm + MX5_CCM_CCGR6);
	writel(0x00000000, ccm + MX50_CCM_CCGR7);

	/* Switch ARM to step clock */
	writel(0x4, ccm + MX5_CCM_CCSR);

	if (cpufreq_mhz == 400)
		imx5_setup_pll_400(IOMEM(MX50_PLL1_BASE_ADDR));
	else
		imx5_setup_pll_800(IOMEM(MX50_PLL1_BASE_ADDR));

	imx5_setup_pll_216(IOMEM(MX50_PLL3_BASE_ADDR));

	/* Switch peripheral to PLL3 */
	writel(0x00015154, ccm + MX5_CCM_CBCMR);
	writel(0x04880945 | (1<<16), ccm + MX5_CCM_CBCDR);

	/* make sure change is effective */
	while (readl(ccm + MX5_CCM_CDHIPR));

	imx5_setup_pll_400(IOMEM(MX50_PLL2_BASE_ADDR));

	/* Switch peripheral to PLL2 */
	r = 0x02800145 |
		(2 << 10) |
		(0 << 16) |
		(1 << 19);

	writel(r, ccm + MX5_CCM_CBCDR);

	r = readl(ccm + MX5_CCM_CSCMR1);

	/* change uart clk parent to pll2 */
	r &= ~MX5_CCM_CSCMR1_UART_CLK_SEL_MASK;
	r |= 1 << MX5_CCM_CSCMR1_UART_CLK_SEL_OFFSET;

	writel(r, ccm + MX5_CCM_CSCMR1);

	/* make sure change is effective */
	while (readl(ccm + MX5_CCM_CDHIPR));

	/* Set the platform clock dividers */
	writel(0x00000124, MX50_ARM_BASE_ADDR + 0x14);

	writel(0, ccm + MX5_CCM_CACRR);

	/* Switch ARM back to PLL 1. */
	writel(0, ccm + MX5_CCM_CCSR);

	/* make uart div = 6*/
	r = readl(ccm + MX5_CCM_CSCDR1);
	r &= ~0x3f;
	r |= 0x0a;

	r &= ~MX5_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PRED_MASK;
	r &= ~MX5_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PODF_MASK;
	r |= 1 << MX5_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PRED_OFFSET;

	writel(r, ccm + MX5_CCM_CSCDR1);

	/* Restore the default values in the Gate registers */
	writel(0xffffffff, ccm + MX5_CCM_CCGR0);
	writel(0xffffffff, ccm + MX5_CCM_CCGR1);
	writel(0xffffffff, ccm + MX5_CCM_CCGR2);
	writel(0xffffffff, ccm + MX5_CCM_CCGR3);
	writel(0xffffffff, ccm + MX5_CCM_CCGR4);
	writel(0xffffffff, ccm + MX5_CCM_CCGR5);
	writel(0xffffffff, ccm + MX5_CCM_CCGR6);
	writel(0xffffffff, ccm + MX50_CCM_CCGR7);

	writel(0, ccm + MX5_CCM_CCDR);
}

void imx50_init_lowlevel(unsigned int cpufreq_mhz)
{
	imx50_init_lowlevel_early(cpufreq_mhz);

	clock_notifier_call_chain();
}
