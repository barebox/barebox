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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <init.h>
#include <common.h>
#include <io.h>
#include <sizes.h>
#include <mach/imx53-regs.h>
#include <mach/clock-imx51_53.h>

#include "gpio.h"

void *imx_gpio_base[] = {
	(void *)MX53_GPIO1_BASE_ADDR,
	(void *)MX53_GPIO2_BASE_ADDR,
	(void *)MX53_GPIO3_BASE_ADDR,
	(void *)MX53_GPIO4_BASE_ADDR,
	(void *)MX53_GPIO5_BASE_ADDR,
	(void *)MX53_GPIO6_BASE_ADDR,
	(void *)MX53_GPIO7_BASE_ADDR,
};

int imx_gpio_count = ARRAY_SIZE(imx_gpio_base) * 32;

static int imx53_init(void)
{
	add_generic_device("imx_iim", 0, NULL, MX53_IIM_BASE_ADDR, SZ_4K,
			IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx53_init);

static void setup_pll(void __iomem *base, int freq, u32 op, u32 mfd, u32 mfn)
{
	u32 r;

	/*
	 * If freq < 300MHz, we need to set dpdck0_2_en to 0
	 */
	r = 0x00000232;
	if (freq >= 300)
		r |= 0x1000;

	writel(r, base + MX5_PLL_DP_CTL);

	writel(0x2, base + MX5_PLL_DP_CONFIG);

	writel(op, base + MX5_PLL_DP_OP);
	writel(op, base + MX5_PLL_DP_HFS_OP);

	writel(mfd, base + MX5_PLL_DP_MFD);
	writel(mfd, base + MX5_PLL_DP_HFS_MFD);

	writel(mfn, base + MX5_PLL_DP_MFN);
	writel(mfn, base + MX5_PLL_DP_HFS_MFN);

	writel(0x00001232, base + MX5_PLL_DP_CTL);

	while (!(readl(base + MX5_PLL_DP_CTL) & 1));
}

#define setup_pll_1000(base)	setup_pll((base), 1000, ((10 << 4) + ((1 - 1) << 0)), (12 - 1), 5)
#define setup_pll_400(base)	setup_pll((base), 400, ((8 << 4) + ((2 - 1)  << 0)), (3 - 1), 1)
#define setup_pll_455(base)	setup_pll((base), 455, ((9 << 4) + ((2 - 1)  << 0)), (48 - 1), 23)
#define setup_pll_216(base)	setup_pll((base), 216, ((8 << 4) + ((2 - 1)  << 0)), (1 - 1), 1)

int mx53_init_lowlevel(void)
{
	void __iomem *ccm = (void __iomem *)MX53_CCM_BASE_ADDR;
	u32 r;

	/* ARM errata ID #468414 */
	__asm__ __volatile__("mrc 15, 0, %0, c1, c0, 1":"=r"(r));
	r |= (1 << 5);    /* enable L1NEON bit */
	r &= ~(1 << 1);   /* explicitly disable L2 cache */
	__asm__ __volatile__("mcr 15, 0, %0, c1, c0, 1" : : "r"(r));

        /* reconfigure L2 cache aux control reg */
	r = 0xc0 |		/* tag RAM */
		0x4 |		/* data RAM */
		(1 << 24) |	/* disable write allocate delay */
		(1 << 23) |	/* disable write allocate combine */
		(1 << 22);	/* disable write allocate */

	__asm__ __volatile__("mcr 15, 1, %0, c9, c0, 2" : : "r"(r));

	__asm__ __volatile__("mrc 15, 0, %0, c1, c0, 1":"=r"(r));
	r |= 1 << 1; 	/* enable L2 cache */
	__asm__ __volatile__("mcr 15, 0, %0, c1, c0, 1" : : "r"(r));

	/*
	 * AIPS setup - Only setup MPROTx registers.
	 * The PACR default values are good.
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, MX53_AIPS1_BASE_ADDR + 0);
	writel(0x77777777, MX53_AIPS1_BASE_ADDR + 4);
	writel(0x77777777, MX53_AIPS2_BASE_ADDR + 0);
	writel(0x77777777, MX53_AIPS2_BASE_ADDR + 4);

	/* Gate of clocks to the peripherals first */
	writel(0x3fffffff, ccm + MX5_CCM_CCGR0);
	writel(0x00000000, ccm + MX5_CCM_CCGR1);
	writel(0x00000000, ccm + MX5_CCM_CCGR2);
	writel(0x00000000, ccm + MX5_CCM_CCGR3);
	writel(0x00030000, ccm + MX5_CCM_CCGR4);
	writel(0x00fff030, ccm + MX5_CCM_CCGR5);
	writel(0x0f00030f, ccm + MX5_CCM_CCGR6);
	writel(0x00000000, ccm + MX53_CCM_CCGR7);

	/* Switch ARM to step clock */
	writel(0x4, ccm + MX5_CCM_CCSR);

	setup_pll_1000((void __iomem *)MX53_PLL1_BASE_ADDR);
	setup_pll_400((void __iomem *)MX53_PLL3_BASE_ADDR);

        /* Switch peripheral to PLL3 */
	writel(0x00015154, ccm + MX5_CCM_CBCMR);
	writel(0x02888945 | (1<<16), ccm + MX5_CCM_CBCDR);

	/* make sure change is effective */
	while (readl(ccm + MX5_CCM_CDHIPR));

	setup_pll_400((void __iomem *)MX53_PLL2_BASE_ADDR);

	/* Switch peripheral to PLL2 */
	r = 0x00808145 |
		(2 << 10) |
		(0 << 16) |
		(1 << 19);

	writel(r, ccm + MX5_CCM_CBCDR);

	writel(0x00016154, ccm + MX5_CCM_CBCMR);

	/* change uart clk parent to pll2 */
	r = readl(ccm + MX5_CCM_CSCMR1);
	r &= ~(3 << 24);
	r |= (1 << 24);
	writel(r, ccm + MX5_CCM_CSCMR1);

	/* make sure change is effective */
	while (readl(ccm + MX5_CCM_CDHIPR));

	setup_pll_216((void __iomem *)MX53_PLL3_BASE_ADDR);
	setup_pll_455((void __iomem *)MX53_PLL4_BASE_ADDR);

	/* Set the platform clock dividers */
	writel(0x00000124, MX53_ARM_BASE_ADDR + 0x14);

	writel(0, ccm + MX5_CCM_CACRR);

	/* Switch ARM back to PLL 1. */
	writel(0, ccm + MX5_CCM_CCSR);

	/* make uart div = 6*/
	r = readl(ccm + MX5_CCM_CSCDR1);
	r &= ~0x3f;
	r |= 0x0a;
	writel(r, ccm + MX5_CCM_CSCDR1);

	/* Restore the default values in the Gate registers */
	writel(0xffffffff, ccm + MX5_CCM_CCGR0);
	writel(0xffffffff, ccm + MX5_CCM_CCGR1);
	writel(0xffffffff, ccm + MX5_CCM_CCGR2);
	writel(0xffffffff, ccm + MX5_CCM_CCGR3);
	writel(0xffffffff, ccm + MX5_CCM_CCGR4);
	writel(0xffffffff, ccm + MX5_CCM_CCGR5);
	writel(0xffffffff, ccm + MX5_CCM_CCGR6);
	writel(0xffffffff, ccm + MX53_CCM_CCGR7);

	writel(0, ccm + MX5_CCM_CCDR);

	return 0;
}
