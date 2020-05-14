#ifndef __ASM_ARCH_CLOCKS_H
#define __ASM_ARCH_CLOCKS_H

unsigned long get_bus_clock(void);
unsigned long get_cpu_clock(void);
unsigned long get_ipb_clock(void);
unsigned long get_pci_clock(void);
unsigned long get_timebase_clock(void);
static inline unsigned long fsl_get_i2c_freq(void)
{
	return get_ipb_clock();
}

#endif /* __ASM_ARCH_CLOCKS_H */
