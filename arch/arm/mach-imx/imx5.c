#include <common.h>
#include <io.h>
#include <sizes.h>
#include <mach/imx5.h>
#include <mach/clock-imx51_53.h>

void imx5_setup_pll(void __iomem *base, int freq, u32 op, u32 mfd, u32 mfn)
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

void imx5_init_lowlevel(void)
{
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
}
