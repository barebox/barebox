// SPDX-License-Identifier: GPL-2.0-only

#include <asm/system.h>
#include <clock.h>
#include <common.h>

/* Unlike the ARMv8, the timer is not generic to ARM32 */
void arm_architected_timer_udelay(unsigned long us)
{
	unsigned long long ticks, cntfrq = get_cntfrq();
	unsigned long long start = get_cntpct();

	ticks = DIV_ROUND_DOWN_ULL((us * cntfrq), 1000000);

	while ((long)(start + ticks - get_cntpct()) > 0)
		;
}
