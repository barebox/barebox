#ifndef __MACH_SOCFPGA_GENERIC_H
#define __MACH_SOCFPGA_GENERIC_H

#include <linux/types.h>

struct socfpga_cm_config;

struct socfpga_io_config;

struct arria10_mainpll_cfg;
struct arria10_perpll_cfg;
struct arria10_pinmux_cfg;

void arria10_init(struct arria10_mainpll_cfg *mainpll,
		  struct arria10_perpll_cfg *perpll, uint32_t *pinmux);
void arria10_finish_io(struct arria10_mainpll_cfg *mainpll,
		       struct arria10_perpll_cfg *perpll, uint32_t *pinmux);

void socfpga_lowlevel_init(struct socfpga_cm_config *cm_config,
			   struct socfpga_io_config *io_config);

#if defined(CONFIG_ARCH_SOCFPGA_CYCLONE5)
void socfpga_cyclone5_mmc_init(void);
void socfpga_cyclone5_uart_init(void);
void socfpga_cyclone5_timer_init(void);
void socfpga_cyclone5_qspi_init(void);
#else
static inline void socfpga_cyclone5_mmc_init(void)
{
	return;
}

static inline void socfpga_cyclone5_uart_init(void)
{
	return;
}

static inline void socfpga_cyclone5_timer_init(void)
{
	return;
}

static inline void socfpga_cyclone5_qspi_init(void)
{
	return;
}
#endif
#if defined(CONFIG_ARCH_SOCFPGA_ARRIA10)
void socfpga_arria10_mmc_init(void);
void socfpga_arria10_timer_init(void);
int arria10_prepare_mmc(int barebox, int bitstream);
void arria10_start_image(int offset);
int arria10_load_fpga(int offset, int size);
int arria10_device_init(struct arria10_mainpll_cfg *mainpll,
			struct arria10_perpll_cfg *perpll,
			uint32_t *pinmux);
enum bootsource arria10_get_bootsource(void);
#else
static inline void socfpga_arria10_mmc_init(void)
{
	return;
}

static inline void socfpga_arria10_timer_init(void)
{
	return;
}
static inline void arria10_prepare_mmc(int barebox, int bitstream)
{
	return;
}
static inline void arria10_start_image(int offset)
{
	return;
}
static inline int arria10_load_fpga(int offset, int size)
{
	return 0;
}
static inline int arria10_device_init(struct arria10_mainpll_cfg *mainpll,
			struct arria10_perpll_cfg *perpll,
			uint32_t *pinmux)
{
	return 0;
}
#endif

static inline void __udelay(unsigned us)
{
	volatile unsigned int i;

	for (i = 0; i < us * 3; i++);
}

struct socfpga_barebox_part {
	unsigned int nor_offset;
	unsigned int nor_size;
	const char *mmc_disk;
};

/* Partition/device for xloader to load main bootloader from */
extern const struct socfpga_barebox_part *barebox_part;

#endif /* __MACH_SOCFPGA_GENERIC_H */
