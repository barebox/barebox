#include <common.h>
#include <io.h>
#include <mach/mxs.h>
#include <mach/imx-regs.h>

#define	MXS_BLOCK_SFTRST				(1 << 31)
#define	MXS_BLOCK_CLKGATE				(1 << 30)

int mxs_reset_block(void __iomem *reg, int just_enable)
{
	/* Clear SFTRST */
	writel(MXS_BLOCK_SFTRST, reg + BIT_CLR);
	mdelay(1);

	/* Clear CLKGATE */
	writel(MXS_BLOCK_CLKGATE, reg + BIT_CLR);

	if (!just_enable) {
		/* Set SFTRST */
		writel(MXS_BLOCK_SFTRST, reg + BIT_SET);
		mdelay(1);
	}

	/* Clear SFTRST */
	writel(MXS_BLOCK_SFTRST, reg + BIT_CLR);
	mdelay(1);

	/* Clear CLKGATE */
	writel(MXS_BLOCK_CLKGATE, reg + BIT_CLR);
	mdelay(1);

	return 0;
}
