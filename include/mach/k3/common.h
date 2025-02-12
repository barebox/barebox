#ifndef __MACH_K3_COMMON_H
#define __MACH_K3_COMMON_H

#include <bootsource.h>

void am625_get_bootsource(enum bootsource *src, int *instance);
u64 am625_sdram_size(void);
void am625_register_dram(void);
void am625_enable_32k_crystal(void);

#endif /* __MACH_K3_COMMON_H */
