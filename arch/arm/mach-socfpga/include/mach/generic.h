#ifndef __MACH_SOCFPGA_GENERIC_H
#define __MACH_SOCFPGA_GENERIC_H

struct socfpga_cm_config;

void socfpga_lowlevel_init(struct socfpga_cm_config *cm_config,
		unsigned long *pinmux, int num_pinmux);

static inline void __udelay(unsigned us)
{
	volatile unsigned int i;

	for (i = 0; i < us * 3; i++);
}

#endif /* __MACH_SOCFPGA_GENERIC_H */
