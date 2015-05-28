#ifndef __MACH_SOCFPGA_GENERIC_H
#define __MACH_SOCFPGA_GENERIC_H

struct socfpga_cm_config;

struct socfpga_io_config;

void socfpga_lowlevel_init(struct socfpga_cm_config *cm_config,
			   struct socfpga_io_config *io_config);

static inline void __udelay(unsigned us)
{
	volatile unsigned int i;

	for (i = 0; i < us * 3; i++);
}

struct socfpga_barebox_part {
	unsigned int nor_offset;
	unsigned int nor_size;
};

#endif /* __MACH_SOCFPGA_GENERIC_H */
