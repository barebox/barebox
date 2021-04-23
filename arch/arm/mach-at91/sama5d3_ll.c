// SPDX-License-Identifier: GPL-2.0-only AND BSD-1-Clause
// SPDX-FileCopyrightText: 2017, Microchip Corporation

#include <mach/at91_wdt.h>
#include <mach/barebox-arm.h>
#include <mach/sama5d3_ll.h>

void sama5d3_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();

	at91_wdt_disable(IOMEM(SAMA5D3_BASE_WDT));
	at91_pmc_init(IOMEM(SAMA5D3_BASE_PMC), AT91_PMC_LL_SAMA5D3);

	/* At this stage the main oscillator
	 * is supposed to be enabled PCK = MCK = MOSC
	 */

	/* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
	at91_pmc_cfg_plla(IOMEM(SAMA5D3_BASE_PMC), AT91_PMC3_MUL_(43)
			  | AT91_PMC_OUT_0 | AT91_PMC_PLLCOUNT
			  | AT91_PMC_DIV_BYPASS, AT91_PMC_LL_SAMA5D3);

	/* Initialize PLLA charge pump */
	at91_pmc_init_pll(IOMEM(SAMA5D3_BASE_PMC), AT91_PMC_IPLLA_3);

	/* Switch PCK/MCK on Main clock output */
	at91_pmc_cfg_mck(IOMEM(SAMA5D3_BASE_PMC), AT91SAM9_PMC_MDIV_4
			 | AT91_PMC_CSS_MAIN, AT91_PMC_LL_SAMA5D3);

	/* Switch PCK/MCK on PLLA output */
	at91_pmc_cfg_mck(IOMEM(SAMA5D3_BASE_PMC), AT91SAM9_PMC_MDIV_4
			 | AT91_PMC_CSS_PLLA, AT91_PMC_LL_SAMA5D3);
}
