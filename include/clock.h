#ifndef CLOCK_H
#define CLOCK_H

#include <types.h>
#include <linux/time.h>

#define CLOCKSOURCE_MASK(bits) (uint64_t)((bits) < 64 ? ((1ULL<<(bits))-1) : -1)

struct clocksource {
	uint32_t	shift;
	uint32_t	mult;
	uint64_t	(*read)(void);
	uint64_t	cycle_last;
	uint64_t	mask;
	int		priority;
	int		(*init)(struct clocksource*);
};

static inline uint32_t cyc2ns(struct clocksource *cs, uint64_t cycles)
{
        uint64_t ret = cycles;
        ret = (ret * cs->mult) >> cs->shift;
        return ret;
}

int init_clock(struct clocksource *);

uint64_t get_time_ns(void);

void clocks_calc_mult_shift(uint32_t *mult, uint32_t *shift, uint32_t from, uint32_t to, uint32_t maxsec);

uint32_t clocksource_hz2mult(uint32_t hz, uint32_t shift_constant);

int is_timeout(uint64_t start_ns, uint64_t time_offset_ns);
int is_timeout_non_interruptible(uint64_t start_ns, uint64_t time_offset_ns);

void ndelay(unsigned long nsecs);
void udelay(unsigned long usecs);
void mdelay(unsigned long msecs);
void mdelay_non_interruptible(unsigned long msecs);

#define SECOND ((uint64_t)(1000 * 1000 * 1000))
#define MSECOND ((uint64_t)(1000 * 1000))
#define USECOND ((uint64_t)(1000))

extern uint64_t time_beginning;

/*
 * Convenience wrapper to implement a typical polling loop with
 * timeout. returns 0 if the condition became true within the
 * timeout or -ETIMEDOUT otherwise
 */
#define wait_on_timeout(timeout, condition) \
({								\
	int __ret = 0;						\
	uint64_t __to_start = get_time_ns();			\
								\
	while (!(condition)) {					\
		if (is_timeout(__to_start, (timeout))) {	\
			__ret = -ETIMEDOUT;			\
			break;					\
		}						\
	}							\
	__ret;							\
})

#endif /* CLOCK_H */
