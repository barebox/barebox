// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2006, Atmel Corporation
 */

#ifndef AT91_PMC_LL_H
#define AT91_PMC_LL_H

#include <errno.h>
#include <asm/io.h>
#include <mach/at91_pmc.h>

#define AT91_PMC_LL_FLAG_SAM9X5_PMC	(1 << 0)
#define AT91_PMC_LL_FLAG_MEASURE_XTAL	(1 << 1)
#define AT91_PMC_LL_FLAG_DISABLE_RC	(1 << 2)
#define AT91_PMC_LL_FLAG_H32MXDIV	(1 << 3)
#define AT91_PMC_LL_FLAG_PMC_UTMI	(1 << 4)
#define AT91_PMC_LL_FLAG_GCSR		(1 << 5)
#define AT91_PMC_LL_FLAG_MCK_BYPASS	(1 << 6)

#define AT91_PMC_LL_AT91RM9200	(0)
#define AT91_PMC_LL_AT91SAM9260	(0)
#define AT91_PMC_LL_AT91SAM9261	(0)
#define AT91_PMC_LL_AT91SAM9263	(0)
#define AT91_PMC_LL_AT91SAM9G45	(AT91_PMC_LL_FLAG_PMC_UTMI)
#define AT91_PMC_LL_AT91SAM9X5	(AT91_PMC_LL_FLAG_SAM9X5_PMC | \
				 AT91_PMC_LL_FLAG_DISABLE_RC | \
				 AT91_PMC_LL_FLAG_PMC_UTMI)
#define AT91_PMC_LL_AT91SAM9N12	(AT91_PMC_LL_FLAG_SAM9X5_PMC | \
				 AT91_PMC_LL_FLAG_DISABLE_RC)
#define AT91_PMC_LL_SAMA5D2	(AT91_PMC_LL_FLAG_SAM9X5_PMC | \
				 AT91_PMC_LL_FLAG_MEASURE_XTAL | \
				 AT91_PMC_LL_FLAG_PMC_UTMI)
/* This assumes a crystal on both XIN and XOUT. If your board
 * instead has an extenal oscillator on XIN only,
 * AT91_PMC_LL_FLAG_MCK_BYPASS needs to be OR`ed in as well
 */
#define AT91_PMC_LL_SAMA5D3	(AT91_PMC_LL_FLAG_SAM9X5_PMC | \
				 AT91_PMC_LL_FLAG_DISABLE_RC | \
				 AT91_PMC_LL_FLAG_PMC_UTMI)
#define AT91_PMC_LL_SAMA5D4	(AT91_PMC_LL_FLAG_SAM9X5_PMC | \
				 AT91_PMC_LL_FLAG_H32MXDIV | \
				 AT91_PMC_LL_FLAG_PMC_UTMI)

void at91_pmc_init(void __iomem *pmc_base, unsigned int flags);
void at91_pmc_cfg_mck(void __iomem *pmc_base, u32 pmc_mckr, unsigned int flags);
void at91_pmc_cfg_plla(void __iomem *pmc_base, u32 pmc_pllar, unsigned int flags);

int at91_pmc_enable_generic_clock(void __iomem *pmc_base, void __iomem *sfr_base,
				  unsigned int periph_id,
				  unsigned int clk_source, unsigned int div,
				  unsigned int flags);

static inline void at91_pmc_init_pll(void __iomem *pmc_base, u32 pmc_pllicpr)
{
	writel(pmc_pllicpr, pmc_base + AT91_PMC_PLLICPR);
}

static inline void at91_pmc_enable_system_clock(void __iomem *pmc_base,
						unsigned clock_id)
{
	writel(clock_id, pmc_base +  AT91_PMC_SCER);
}

static inline int at91_pmc_enable_periph_clock(void __iomem *pmc_base,
					       unsigned periph_id)
{
	u32 mask = 0x01 << (periph_id % 32);

	if ((periph_id / 32) == 1)
		writel(mask, pmc_base + AT91_PMC_PCER1);
	else if ((periph_id / 32) == 0)
		writel(mask, pmc_base + AT91_PMC_PCER);
	else
		return -EINVAL;

	return 0;
}

static inline int at91_pmc_sam9x5_enable_periph_clock(void __iomem *pmc_base,
						      unsigned periph_id)
{
       u32 pcr = periph_id;

       if (periph_id >= 0x80) /* 7 bits only */
               return -EINVAL;

       writel(pcr, pmc_base + AT91_PMC_PCR);
       pcr |= readl(pmc_base + AT91_PMC_PCR) & AT91_PMC_PCR_DIV_MASK;
       pcr |= AT91_PMC_PCR_CMD | AT91_PMC_PCR_EN;
       writel(pcr, pmc_base + AT91_PMC_PCR);

       return 0;
}

static inline bool at91_pmc_check_mck_h32mxdiv(void __iomem *pmc_base,
					       unsigned flags)
{
	if (flags & AT91_PMC_LL_FLAG_H32MXDIV)
		return readl(pmc_base + AT91_PMC_MCKR) & AT91_PMC_H32MXDIV;

	return false;
}

#endif
