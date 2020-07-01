#ifndef __MACH_SAMA5D2_LL__
#define __MACH_SAMA5D2_LL__

#include <mach/sama5d2.h>
#include <mach/at91_pmc_ll.h>
#include <mach/early_udelay.h>
#include <mach/ddramc.h>

#include <common.h>

void sama5d2_lowlevel_init(void);

static inline void sama5d2_pmc_enable_periph_clock(int clk)
{
	at91_pmc_sam9x5_enable_periph_clock(SAMA5D2_BASE_PMC, clk);
}

/* requires relocation */
static inline void sama5d2_udelay_init(unsigned int msc)
{
	early_udelay_init(SAMA5D2_BASE_PMC, SAMA5D2_BASE_PITC,
			  SAMA5D2_ID_PIT, msc, AT91_PMC_LL_SAMA5D2);
}


void sama5d2_ddr2_init(struct at91_ddramc_register *ddramc_reg_config);

static inline int sama5d2_pmc_enable_generic_clock(unsigned int periph_id,
						   unsigned int clk_source,
						   unsigned int div)
{
	return at91_pmc_enable_generic_clock(SAMA5D2_BASE_PMC,
					     SAMA5D2_BASE_SFR,
					     periph_id, clk_source, div,
					     AT91_PMC_LL_SAMA5D2);
}

#endif
