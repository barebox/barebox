/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __SAMA5D3_LL_H__
#define __SAMA5D3_LL_H__

#include <mach/at91_pmc_ll.h>
#include <mach/debug_ll.h>
#include <mach/early_udelay.h>

void sama5d3_lowlevel_init(void);

static inline void sama5d3_pmc_enable_periph_clock(int clk)
{
	at91_pmc_enable_periph_clock(IOMEM(SAMA5D3_BASE_PMC), clk);
}

/* requires relocation */
static inline void sama5d3_udelay_init(unsigned int msc)
{
	early_udelay_init(IOMEM(SAMA5D3_BASE_PMC), IOMEM(SAMA5D3_BASE_PIT),
			  SAMA5D3_ID_PIT, msc, AT91_PMC_LL_SAMA5D3);
}

#endif /* __SAMA5D3_LL_H__ */
