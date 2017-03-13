#include <common.h>
#include <init.h>
#include <driver.h>
#include <efi.h>
#include <efi/efi.h>
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

static uint64_t efi_x86_cs_read(void)
{
	return 1000 * 1000 * ticks_read() / freq;
}

static int efi_x86_cs_init(struct clocksource *cs)
{
	cs->mult = clocksource_hz2mult(1000 * 1000, cs->shift);

	freq = ticks_freq();

	return 0;
}

static struct clocksource efi_x86_cs = {
	.read   = efi_x86_cs_read,
	.mask   = CLOCKSOURCE_MASK(64),
	.shift  = 0,
	.priority = 100,
	.init   = efi_x86_cs_init,
};

static int efi_x86_cs_probe(struct device_d *dev)
{
	return init_clock(&efi_x86_cs);
}

static struct driver_d efi_x86_cs_driver = {
	.name = "efi-cs-x86",
	.probe = efi_x86_cs_probe,
};

static int efi_x86_cs_initcall(void)
{
	return platform_driver_register(&efi_x86_cs_driver);
}
/* for efi the time must be init at core initcall level */
core_initcall(efi_x86_cs_initcall);
