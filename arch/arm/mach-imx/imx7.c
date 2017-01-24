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
#include <linux/sizes.h>
#include <mach/imx7.h>
#include <mach/generic.h>
#include <mach/revision.h>
#include <mach/imx7-regs.h>

void imx7_init_lowlevel(void)
{
	void __iomem *aips1 = IOMEM(MX7_AIPS1_CONFIG_BASE_ADDR);
	void __iomem *aips2 = IOMEM(MX7_AIPS2_CONFIG_BASE_ADDR);

	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, aips1);
	writel(0x77777777, aips1 + 0x4);
	writel(0, aips1 + 0x40);
	writel(0, aips1 + 0x44);
	writel(0, aips1 + 0x48);
	writel(0, aips1 + 0x4c);
	writel(0, aips1 + 0x50);

	writel(0x77777777, aips2);
	writel(0x77777777, aips2 + 0x4);
	writel(0, aips2 + 0x40);
	writel(0, aips2 + 0x44);
	writel(0, aips2 + 0x48);
	writel(0, aips2 + 0x4c);
	writel(0, aips2 + 0x50);
}

#define SC_CNTCR	0x0
#define SC_CNTSR	0x4
#define SC_CNTCV1	0x8
#define SC_CNTCV2	0xc
#define SC_CNTFID0	0x20
#define SC_CNTFID1	0x24
#define SC_CNTFID2	0x28
#define SC_counterid	0xfcc

#define SC_CNTCR_ENABLE         (1 << 0)
#define SC_CNTCR_HDBG           (1 << 1)
#define SC_CNTCR_FREQ0          (1 << 8)
#define SC_CNTCR_FREQ1          (1 << 9)

static int imx7_timer_init(void)
{
	void __iomem *sctr = IOMEM(MX7_SYSCNT_CTRL_BASE_ADDR);
	unsigned long val, freq;

	freq = 8000000;
	asm("mcr p15, 0, %0, c14, c0, 0" : : "r" (freq));

	writel(freq, sctr + SC_CNTFID0);

	/* Enable system counter */
	val = readl(sctr + SC_CNTCR);
	val &= ~(SC_CNTCR_FREQ0 | SC_CNTCR_FREQ1);
	val |= SC_CNTCR_FREQ0 | SC_CNTCR_ENABLE | SC_CNTCR_HDBG;
	writel(val, sctr + SC_CNTCR);

	return 0;
}

#define CSU_NUM_REGS               64
#define CSU_INIT_SEC_LEVEL0        0x00FF00FF

static void imx7_init_csu(void)
{
	void __iomem *csu = IOMEM(MX7_CSU_BASE_ADDR);
	int i = 0;

	for (i = 0; i < CSU_NUM_REGS; i++)
		writel(CSU_INIT_SEC_LEVEL0, csu + i * 4);
}

int imx7_init(void)
{
	const char *cputypestr;
	u32 imx7_silicon_revision;

	imx7_init_lowlevel();

	imx7_init_csu();

	imx7_timer_init();

	imx7_boot_save_loc();

	imx7_silicon_revision = imx7_cpu_revision();

	switch (imx7_cpu_type()) {
	case IMX7_CPUTYPE_IMX7D:
		cputypestr = "i.MX7d";
		break;
	case IMX7_CPUTYPE_IMX7S:
		cputypestr = "i.MX7s";
		break;
	default:
		cputypestr = "unknown i.MX7";
		break;
	}

	imx_set_silicon_revision(cputypestr, imx7_silicon_revision);

	return 0;
}
