
struct clocksource {
	uint32_t	shift;
	uint32_t	mult;
	uint64_t	(*read)(void);
	uint64_t	cycle_last;
	uint64_t	mask;

};

inline uint32_t cyc2ns(struct clocksource *cs, uint64_t cycles)
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

void mdelay(unsigned long msecs);

