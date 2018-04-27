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
#include <linux/sizes.h>
#include <environment.h>
#include <io.h>
#include <mach/imx5.h>
#include <mach/imx51-regs.h>
#include <mach/revision.h>
#include <mach/clock-imx51_53.h>
#include <mach/generic.h>
#include <mach/reset-reason.h>

#define IIM_SREV 0x24

static int imx51_silicon_revision(void)
{
	void __iomem *iim_base = IOMEM(MX51_IIM_BASE_ADDR);
	u32 rev = readl(iim_base + IIM_SREV) & 0xff;

	switch (rev) {
	case 0x0:
		return IMX_CHIP_REV_2_0;
	case 0x10:
		return IMX_CHIP_REV_3_0;
	default:
		return IMX_CHIP_REV_UNKNOWN;
	}

	return 0;
}

static void imx51_ipu_mipi_setup(void)
{
	void __iomem *hsc_addr = IOMEM(MX51_MIPI_HSC_BASE_ADDR);
	u32 val;

	/* setup MIPI module to legacy mode */
	writel(0xf00, hsc_addr);

	/* CSI mode: reserved; DI control mode: legacy (from Freescale BSP) */
	val = readl(hsc_addr + 0x800);
	val |= 0x30ff;
	writel(val, hsc_addr + 0x800);
}

int imx51_init(void)
{
	void __iomem *src = IOMEM(MX51_SRC_BASE_ADDR);

	imx_set_silicon_revision("i.MX51", imx51_silicon_revision());
	imx_set_reset_reason(src + IMX_SRC_SRSR, imx_reset_reasons);
	imx51_boot_save_loc();
	add_generic_device("imx51-esdctl", 0, NULL, MX51_ESDCTL_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	imx51_ipu_mipi_setup();

	return 0;
}

int imx51_devices_init(void)
{
	add_generic_device("imx_iim", 0, NULL, MX51_IIM_BASE_ADDR, SZ_4K,
			IORESOURCE_MEM, NULL);

	add_generic_device("imx-iomuxv3", 0, NULL, MX51_IOMUXC_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx51-ccm", 0, NULL, MX51_CCM_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpt", 0, NULL, MX51_GPT1_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 0, NULL, MX51_GPIO1_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 1, NULL, MX51_GPIO2_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 2, NULL, MX51_GPIO3_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 3, NULL, MX51_GPIO4_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx21-wdt", 0, NULL, MX51_WDOG_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx51-usb-misc", 0, NULL, MX51_OTG_BASE_ADDR + 0x800, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}

#define DP_MFN_800_DIT 60 /* PL Dither mode */

/*
 * Workaround for i.MX51 PLL errata. This is needed by all boards using the
 * i.MX51 silicon version up until (including) 3.0 running at 800MHz.
 * The PLL's in the i.MX51 processor can go out of lock due to a metastable
 * condition in an analog flip-flop when used at high frequencies.
 * This workaround implements an undocumented feature in the PLL (dither
 * mode), which causes the effect of this failure to be much lower (in terms
 * of frequency deviation), avoiding system failure, or at least decreasing
 * the likelihood of system failure.
 */
static void imx51_setup_pll800_bug(void)
{
	void __iomem *base = IOMEM(MX51_PLL1_BASE_ADDR);
	u32 dp_config;
	volatile int i;

	imx5_setup_pll_864(base);

	dp_config = readl(base + MX5_PLL_DP_CONFIG);
	dp_config &= ~MX5_PLL_DP_CONFIG_AREN;
	writel(dp_config, base + MX5_PLL_DP_CONFIG);

	/* Restart PLL with PLM = 1 */
	writel(0x00001236, base + MX5_PLL_DP_CTL);

	/* Wait for lock */
	while (!(readl(base + MX5_PLL_DP_CTL) & 1));

	/* Modify MFN value */
	writel(DP_MFN_800_DIT, base + MX5_PLL_DP_MFN);
	writel(DP_MFN_800_DIT, base + MX5_PLL_DP_HFS_MFN);

	/* Reload MFN value */
	writel(0x1, base + MX5_PLL_DP_CONFIG);

	while (readl(base + MX5_PLL_DP_CONFIG) & 1);

	/* Wait at least 4 us */
	for (i = 0; i < 100; i++);

	/* Enable auto-restart AREN bit */
	dp_config |= MX5_PLL_DP_CONFIG_AREN;
	writel(dp_config, base + MX5_PLL_DP_CONFIG);
}

void imx51_init_lowlevel(unsigned int cpufreq_mhz)
{
	void __iomem *ccm = IOMEM(MX51_CCM_BASE_ADDR);
	u32 r;
	int rev = imx51_silicon_revision();

	imx5_init_lowlevel();

	/* disable write combine for TO 2 and lower revs */
	if (rev < IMX_CHIP_REV_3_0) {
		__asm__ __volatile__("mrc 15, 1, %0, c9, c0, 1":"=r"(r));
		r |= (1 << 25);
		__asm__ __volatile__("mcr 15, 1, %0, c9, c0, 1" : : "r"(r));
	}

	/* Gate of clocks to the peripherals first */
	writel(0x3fffffff, ccm + MX5_CCM_CCGR0);
	writel(0x00000000, ccm + MX5_CCM_CCGR1);
	writel(0x00000000, ccm + MX5_CCM_CCGR2);
	writel(0x00000000, ccm + MX5_CCM_CCGR3);
	writel(0x00030000, ccm + MX5_CCM_CCGR4);
	writel(0x00fff030, ccm + MX5_CCM_CCGR5);
	writel(0x00000300, ccm + MX5_CCM_CCGR6);

	/* Disable IPU and HSC dividers */
	writel(0x00060000, ccm + MX5_CCM_CCDR);

	/* Make sure to switch the DDR away from PLL 1 */
	writel(0x19239145, ccm + MX5_CCM_CBCDR);
	/* make sure divider effective */
	while (readl(ccm + MX5_CCM_CDHIPR));

	/* Switch ARM to step clock */
	writel(0x4, ccm + MX5_CCM_CCSR);

	switch (cpufreq_mhz) {
	case 600:
		imx5_setup_pll_600(IOMEM(MX51_PLL1_BASE_ADDR));
		break;
	default:
		/* Default maximum 800MHz */
		if (rev <= IMX_CHIP_REV_3_0)
			imx51_setup_pll800_bug();
		else
			imx5_setup_pll_800(IOMEM(MX51_PLL1_BASE_ADDR));
		break;
	}

	imx5_setup_pll_665(IOMEM(MX51_PLL3_BASE_ADDR));

	/* Switch peripheral to PLL 3 */
	writel(0x000010C0, ccm + MX5_CCM_CBCMR);
	writel(0x13239145, ccm + MX5_CCM_CBCDR);

	imx5_setup_pll_665(IOMEM(MX51_PLL2_BASE_ADDR));

	/* Switch peripheral to PLL2 */
	writel(0x19239145, ccm + MX5_CCM_CBCDR);
	writel(0x000020C0, ccm + MX5_CCM_CBCMR);

	imx5_setup_pll_216(IOMEM(MX51_PLL3_BASE_ADDR));

	/* Set the platform clock dividers */
	writel(0x00000125, MX51_ARM_BASE_ADDR + 0x14);

	/* Run at Full speed */
	writel(0x0, ccm + MX5_CCM_CACRR);

	/* Switch ARM back to PLL 1 */
	writel(0x0, ccm + MX5_CCM_CCSR);

	/* setup the rest */
	/* Use lp_apm (24MHz) source for perclk */
	writel(0x000020C2, ccm + MX5_CCM_CBCMR);
	/* ddr clock from PLL 1, all perclk dividers are 1 since using 24MHz */
	writel(0x59239100, ccm + MX5_CCM_CBCDR);

	/* Restore the default values in the Gate registers */
	writel(0xffffffff, ccm + MX5_CCM_CCGR0);
	writel(0xffffffff, ccm + MX5_CCM_CCGR1);
	writel(0xffffffff, ccm + MX5_CCM_CCGR2);
	writel(0xffffffff, ccm + MX5_CCM_CCGR3);
	writel(0xffffffff, ccm + MX5_CCM_CCGR4);
	writel(0xffffffff, ccm + MX5_CCM_CCGR5);
	writel(0xffffffff, ccm + MX5_CCM_CCGR6);

	/* Use PLL 2 for UART's, get 66.5MHz from it */
	writel(0xA591A020, ccm + MX5_CCM_CSCMR1);
	writel(0x00C30321, ccm + MX5_CCM_CSCDR1);

	/* make sure divider effective */
	while (readl(ccm + MX5_CCM_CDHIPR));

	writel(0x0, ccm + MX5_CCM_CCDR);
}
