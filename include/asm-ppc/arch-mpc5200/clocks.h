#ifndef __ASM_ARCH_CLOCKS_H
#define __ASM_ARCH_CLOCKS_H

unsigned long get_bus_clock(void);
unsigned long get_cpu_clock(void);
unsigned long get_ipb_clock(void);
unsigned long get_pci_clock(void);
unsigned long get_timebase_clock(void);

#endif /* __ASM_ARCH_CLOCKS_H */
