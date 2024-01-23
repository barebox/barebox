// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <io.h>
#include <asm/syscounter.h>
#include <asm/system.h>
#include <mach/layerscape/errata.h>
#include <mach/layerscape/lowlevel.h>
#include <soc/fsl/immap_lsch3.h>
#include <soc/fsl/scfg.h>

static void ls1028a_timer_init(void)
{
	u32 __iomem *cntcr = IOMEM(LSCH3_TIMER_ADDR);
	u32 __iomem *cltbenr = IOMEM(LSCH3_PMU_CLTBENR);

	u32 __iomem *pctbenr = IOMEM(LSCH3_PCTBENR_OFFSET);

	/* Enable timebase for all clusters.
	 * It is safe to do so even some clusters are not enabled.
	 */
	out_le32(cltbenr, 0xf);

	/*
	 * In certain Layerscape SoCs, the clock for each core's
	 * has an enable bit in the PMU Physical Core Time Base Enable
	 * Register (PCTBENR), which allows the watchdog to operate.
	 */
	setbits_le32(pctbenr, 0xff);

	/* Enable clock for timer
	 * This is a global setting.
	 */
	out_le32(cntcr, 0x1);
}

void ls1028a_init_lowlevel(void)
{
	scfg_init(SCFG_ENDIANESS_LITTLE);
	set_cntfrq(25000000);
	ls1028a_timer_init();
	ls1028a_errata();
}
