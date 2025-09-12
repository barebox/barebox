// SPDX-License-Identifier: GPL-2.0-or-later

#include <io.h>
#include <linux/bits.h>
#include <mach/imx/snvs.h>
#include <mach/imx/imx7-regs.h>
#include <mach/imx/imx8m-regs.h>

#define SNVS_HPCOMR	0x04
#define SNVS_HPCOMR_NPSWA_EN	BIT(31)

#define SNVS_LPSR	0x4c

#define SNVS_LPLVDR	0x64
#define SNVS_LPPGDR_INIT	0x41736166

static void snvs_init(void __iomem *snvs)
{
	u32 val;

	/* Ensure SNVS HPCOMR sets NPSWA_EN to allow unpriv access to SNVS LP */
	val = readl(snvs + SNVS_HPCOMR);
	val |= SNVS_HPCOMR_NPSWA_EN;
	writel(val, snvs + SNVS_HPCOMR);
}

void imx7_snvs_init(void)
{
	void __iomem *snvs = IOMEM(MX7_SNVS_BASE_ADDR);

	snvs_init(snvs);
}

void imx8m_setup_snvs(void)
{
	void __iomem *snvs = IOMEM(MX8M_SNVS_BASE_ADDR);

        /* Initialize glitch detect */
        writel(SNVS_LPPGDR_INIT, snvs + SNVS_LPLVDR);
        /* Clear interrupt status */
        writel(0xffffffff, snvs + SNVS_LPSR);

        snvs_init(snvs);
}
