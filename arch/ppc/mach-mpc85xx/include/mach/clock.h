#ifndef __ASM_ARCH_CLOCKS_H
#define __ASM_ARCH_CLOCKS_H

#include <mach/config_mpc85xx.h>

struct sys_info {
	unsigned long freqProcessor[MAX_CPUS];
	unsigned long freqSystemBus;
	unsigned long freqDDRBus;
	unsigned long freqLocalBus;
};

unsigned long fsl_get_bus_freq(ulong dummy);
unsigned long fsl_get_timebase_clock(void);
unsigned long fsl_get_i2c_freq(void);
void fsl_get_sys_info(struct sys_info *sysInfo);
#endif /* __ASM_ARCH_CLOCKS_H */
