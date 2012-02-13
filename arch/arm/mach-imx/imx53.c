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
#include <mach/imx5.h>
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

#define setup_pll_1000(base)	imx5_setup_pll((base), 1000, ((10 << 4) + ((1 - 1) << 0)), (12 - 1), 5)
#define setup_pll_800(base)	imx5_setup_pll((base), 800, ((8 << 4) + ((1 - 1) << 0)), (3 - 1), 1)
#define setup_pll_400(base)	imx5_setup_pll((base), 400, ((8 << 4) + ((2 - 1)  << 0)), (3 - 1), 1)
#define setup_pll_455(base)	imx5_setup_pll((base), 455, ((9 << 4) + ((2 - 1)  << 0)), (48 - 1), 23)
#define setup_pll_216(base)	imx5_setup_pll((base), 216, ((8 << 4) + ((2 - 1)  << 0)), (1 - 1), 1)

void imx53_init_lowlevel(unsigned int cpufreq_mhz)
{
	void __iomem *ccm = (void __iomem *)MX53_CCM_BASE_ADDR;
	u32 r;

	imx5_init_lowlevel();

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

	if (cpufreq_mhz == 1000)
		setup_pll_1000((void __iomem *)MX53_PLL1_BASE_ADDR);
	else
		setup_pll_800((void __iomem *)MX53_PLL1_BASE_ADDR);

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

	r &= ~MX5_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PRED_MASK;
	r &= ~MX5_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PODF_MASK;
	r |= 1 << MX5_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PRED_OFFSET;

	r &= ~MX5_CCM_CSCDR1_ESDHC3_MX53_CLK_PRED_MASK;
	r &= ~MX5_CCM_CSCDR1_ESDHC3_MX53_CLK_PODF_MASK;
	r |= 1 << MX5_CCM_CSCDR1_ESDHC3_MX53_CLK_PODF_OFFSET;

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
}
