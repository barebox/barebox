#include <common.h>
#include <efi.h>
#include <mach/efi.h>
#include <clock.h>

#ifdef __x86_64__
uint64_t ticks_read(void)
{
	uint64_t a, d;

	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));

	return (d << 32) | a;
}
#else
uint64_t ticks_read(void)
{
	uint64_t val;

	__asm__ volatile ("rdtsc" : "=A" (val));

	return val;
}
#endif

static uint64_t freq;

/* count TSC ticks during a millisecond delay */
static uint64_t ticks_freq(void)
{
	uint64_t ticks_start, ticks_end;

	ticks_start = ticks_read();
	BS->stall(1000);
	ticks_end = ticks_read();

	return (ticks_end - ticks_start) * 1000;
}

static uint64_t efi_clocksource_read(void)
{
	return 1000 * 1000 * ticks_read() / freq;
}

static struct clocksource cs = {
	.read   = efi_clocksource_read,
	.mask   = CLOCKSOURCE_MASK(64),
	.shift  = 0,
};

int efi_clocksource_init(void)
{
	cs.mult = clocksource_hz2mult(1000 * 1000, cs.shift);

	freq = ticks_freq();

	init_clock(&cs);

	return 0;
}
