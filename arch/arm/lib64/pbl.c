// SPDX-License-Identifier: GPL-2.0-only

#include <asm/system.h>
#include <clock.h>
#include <common.h>

void udelay(unsigned long us)
{
	unsigned long cntfrq = get_cntfrq();
	unsigned long ticks = (us * cntfrq) / 1000000;
	unsigned long start = get_cntpct();

	while ((long)(start + ticks - get_cntpct()) > 0);
}

void mdelay(unsigned long ms)
{
	udelay(ms * 1000);
}
