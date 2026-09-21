// SPDX-License-Identifier: GPL-2.0-only

#include <asm/system.h>
#include <clock.h>

/* Uses only architected timer system registers, so usable before relocation */
static __always_inline void arch_timer_udelay(unsigned long us)
{
	unsigned long cntfrq = get_cntfrq();
	unsigned long ticks = (us * cntfrq) / 1000000;
	unsigned long start = get_cntpct();

	while ((long)(start + ticks - get_cntpct()) > 0)
		;
}

/*
 * Until a clocksource is registered, read the counter directly, which
 * works as soon as CNTFRQ_EL0 is programmed, even before relocation.
 */
void __prereloc udelay(unsigned long us)
{
	if (clocksource_registered())
		clocksource_current_udelay(us);
	else
		arch_timer_udelay(us);
}

/*
 * Until a clocksource is registered, time stands still and timeouts
 * never fire, so polls wait indefinitely instead of panicking.
 */
uint64_t __prereloc get_time_ns(void)
{
	if (clocksource_registered())
		return clocksource_current_get_time_ns();

	return 0;
}

int __prereloc is_timeout(uint64_t start_ns, uint64_t time_offset_ns)
{
	if (clocksource_registered())
		return clocksource_current_is_timeout(start_ns, time_offset_ns);

	return 0;
}
