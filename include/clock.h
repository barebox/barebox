#include <types.h>

#ifndef CLOCK_H
#define CLOCK_H

#define CLOCKSOURCE_MASK(bits) (uint64_t)((bits) < 64 ? ((1ULL<<(bits))-1) : -1)

struct clocksource {
	uint32_t	shift;
	uint32_t	mult;
	uint64_t	(*read)(void);
	uint64_t	cycle_last;
	uint64_t	mask;

};

static inline uint32_t cyc2ns(struct clocksource *cs, uint64_t cycles)
{
        uint64_t ret = cycles;
        ret = (ret * cs->mult) >> cs->shift;
        return ret;
}

int init_clock(struct clocksource *);

uint64_t get_time_ns(void);

uint32_t clocksource_hz2mult(uint32_t hz, uint32_t shift_constant);

int is_timeout(uint64_t start_ns, uint64_t time_offset_ns);

// void udelay(unsigned long usecs);

void ndelay(unsigned long nsecs);
void mdelay(unsigned long msecs);

#define SECOND ((uint64_t)(1000 * 1000 * 1000))
#define MSECOND ((uint64_t)(1000 * 1000))
#define USECOND ((uint64_t)(1000))

#endif /* CLOCK_H */
