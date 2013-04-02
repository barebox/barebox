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
#include <sizes.h>
#include <environment.h>
#include <io.h>
#include <mach/imx5.h>
#include <mach/imx51-regs.h>
#include <mach/revision.h>
#include <mach/clock-imx51_53.h>
#include <mach/generic.h>

#define SI_REV 0x48

static int imx51_silicon_revision(void)
{
	void __iomem *rom = MX51_IROM_BASE_ADDR;
	u32 mx51_silicon_revision;
	u32 rev;

	rev = readl(rom + SI_REV);
	switch (rev) {
	case 0x1:
		mx51_silicon_revision = IMX_CHIP_REV_1_0;
		break;
	case 0x2:
		mx51_silicon_revision = IMX_CHIP_REV_1_1;
		break;
	case 0x10:
		mx51_silicon_revision = IMX_CHIP_REV_2_0;
		break;
	case 0x20:
		mx51_silicon_revision = IMX_CHIP_REV_3_0;
		break;
	default:
		mx51_silicon_revision = 0;
	}

	imx_set_silicon_revision("i.MX51", mx51_silicon_revision);

	return 0;
}

static int imx51_init(void)
{
	imx51_silicon_revision();
	imx51_boot_save_loc((void *)MX51_SRC_BASE_ADDR);

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
	add_generic_device("imx51-esdctl", 0, NULL, MX51_ESDCTL_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx51-usb-misc", 0, NULL, MX51_OTG_BASE_ADDR + 0x800, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
postcore_initcall(imx51_init);

/*
 * Saves the boot source media into the $bootsource environment variable
 *
 * This information is useful for barebox init scripts as we can then easily
 * use a kernel image stored on the same media that we launch barebox with
 * (for example).
 *
 * imx25 and imx35 can boot into barebox from several media such as
 * nand, nor, mmc/sd cards, serial roms. "mmc" is used to represent several
 * sources as its impossible to distinguish between them.
 *
 * Some sources such as serial roms can themselves have 3 different boot
 * possibilities (i2c1, i2c2 etc). It is assumed that any board will
 * only be using one of these at any one time.
 *
 * Note also that I suspect that the boot source pins are only sampled at
 * power up.
 */

void imx51_init_lowlevel(unsigned int cpufreq_mhz)
{
	void __iomem *ccm = (void __iomem *)MX51_CCM_BASE_ADDR;
	u32 r;

	imx5_init_lowlevel();

	/* disable write combine for TO 2 and lower revs */
	if (imx_silicon_revision() < IMX_CHIP_REV_3_0) {
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
		imx5_setup_pll_600((void __iomem *)MX51_PLL1_BASE_ADDR);
		break;
	default:
		/* Default maximum 800MHz */
		imx5_setup_pll_800((void __iomem *)MX51_PLL1_BASE_ADDR);
		break;
	}

	imx5_setup_pll_665((void __iomem *)MX51_PLL3_BASE_ADDR);

	/* Switch peripheral to PLL 3 */
	writel(0x000010C0, ccm + MX5_CCM_CBCMR);
	writel(0x13239145, ccm + MX5_CCM_CBCDR);

	imx5_setup_pll_665((void __iomem *)MX51_PLL2_BASE_ADDR);

	/* Switch peripheral to PLL2 */
	writel(0x19239145, ccm + MX5_CCM_CBCDR);
	writel(0x000020C0, ccm + MX5_CCM_CBCMR);

	imx5_setup_pll_216((void __iomem *)MX51_PLL3_BASE_ADDR);

	/* Set the platform clock dividers */
	writel(0x00000124, MX51_ARM_BASE_ADDR + 0x14);

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
