#include <common.h>
#include <init.h>
#include <clock.h>
#include <asm/arch/linux.h>

uint64_t linux_clocksource_read(void)
{
	return linux_get_time();
}

static struct clocksource cs = {
	.read	= linux_clocksource_read,
	.mask	= 0xffffffff,
	.shift	= 10,
};

static int clocksource_init (void)
{
	cs.mult = clocksource_hz2mult(1000 * 1000 * 1000, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);
