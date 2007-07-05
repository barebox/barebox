#include <common.h>
#include <asm-generic/div64.h>
#include <clock.h>

static struct clocksource *current_clock;
static uint64_t time_ns;

/**
 * get_time_ns - get current timestamp in nanoseconds
 */
uint64_t get_time_ns(void)
{
	struct clocksource *cs = current_clock;
        uint64_t cycle_now, cycle_delta;
        uint64_t ns_offset;

        /* read clocksource: */
        cycle_now = cs->read();

        /* calculate the delta since the last call: */
        cycle_delta = (cycle_now - cs->cycle_last) & cs->mask;

        /* convert to nanoseconds: */
        ns_offset = cyc2ns(cs, cycle_delta);

	cs->cycle_last = cycle_now;

	time_ns += ns_offset;
        return time_ns;
}

/**
 * clocksource_hz2mult - calculates mult from hz and shift
 * @hz:                 Clocksource frequency in Hz
 * @shift_constant:     Clocksource shift factor
 *
 * Helper functions that converts a hz counter
 * frequency to a timsource multiplier, given the
 * clocksource shift value
 */
uint32_t clocksource_hz2mult(uint32_t hz, uint32_t shift_constant)
{
        /*  hz = cyc/(Billion ns)
         *  mult/2^shift  = ns/cyc
         *  mult = ns/cyc * 2^shift
         *  mult = 1Billion/hz * 2^shift
         *  mult = 1000000000 * 2^shift / hz
         *  mult = (1000000000<<shift) / hz
         */
        uint64_t tmp = ((uint64_t)1000000000) << shift_constant;

        tmp += hz/2; /* round for do_div */
        do_div(tmp, hz);

        return (uint32_t)tmp;
}

int is_timeout(uint64_t start_ns, uint64_t time_offset_ns)
{
//	if (ctrlc ())
//		return -1;

	if (start_ns + time_offset_ns < get_time_ns())
		return 1;
	else
		return 0;
}

int udelay(unsigned long usecs)
{
	uint64_t start = get_time_ns();

	while(!is_timeout(start, usecs * 1000));
//		if (ctrlc ())
//			return -1;
	return 0;
}

void mdelay(unsigned long msecs)
{
	uint64_t start = get_time_ns();

	while(!is_timeout(start, msecs * 1000000));
}

int init_clock(struct clocksource *cs)
{
	current_clock = cs;
	return 0;
}
