/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SOC_STARFIVE_RSTGEN_H_
#define __SOC_STARFIVE_RSTGEN_H_

#include <dt-bindings/reset-controller/starfive-jh7100.h>
#include <linux/bitops.h>
#include <linux/types.h>

#define STARFIVE_RSTGEN_STATUS	0x10

static inline void __iomem *starfive_rstgen_bank(void __iomem *base, unsigned long *id)
{
	void __iomem *bank = base + *id / (4 * BITS_PER_BYTE) * 4;
	*id %= 4 * BITS_PER_BYTE;
	return bank;
}

static inline void __starfive_rstgen(void __iomem *base, unsigned long id, bool assert)
{
	void __iomem *bank = starfive_rstgen_bank(base, &id);
	u32 val;

	val = readl(bank);

	if (assert)
		val |= BIT(id);
	else
		val &= ~BIT(id);

	writel(val, bank);
}

static bool __starfive_rstgen_asserted(void __iomem *base, unsigned long id)
{
	void __iomem *bank = starfive_rstgen_bank(base, &id);

	return !(readl(bank + STARFIVE_RSTGEN_STATUS) & BIT(id));
}

#endif
