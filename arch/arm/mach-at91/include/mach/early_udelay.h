#ifndef __EARLY_UDELAY_H__
#define __EARLY_UDELAY_H__

#include <linux/compiler.h>

/* requires PIT to be initialized, but not the clocksource framework */
void early_udelay(unsigned int usec);
void early_udelay_init(void __iomem *pmc_base,
		       void __iomem *pit_base,
		       unsigned int clock,
		       unsigned int master_clock_rate,
		       unsigned int flags);

#endif
