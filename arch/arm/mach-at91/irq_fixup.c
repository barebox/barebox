/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#include <io.h>
#include <mach/at91_rtt.h>

/*
 * As the RTT is powered by the backup power so if the interrupt
 * is still on when the kernel start, the kernel will end up with
 * dead lock interrupt that it can not clear. Because the interrupt line is
 * shared with the basic timer (PIT) on AT91_ID_SYS.
 */
void at91_rtt_irq_fixup(void *base)
{
	void *reg = base + AT91_RTT_MR;
	u32 mr = readl(reg);

	writel(mr & ~(AT91_RTT_ALMIEN | AT91_RTT_RTTINCIEN), reg);
}
