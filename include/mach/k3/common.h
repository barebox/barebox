#ifndef __MACH_K3_COMMON_H
#define __MACH_K3_COMMON_H

#include <bootsource.h>
#include <linux/sizes.h>
#include <linux/uuid.h>

#define UUID_TI_DM_FW \
        UUID_INIT(0x9e8c2017, 0x8b94, 0x4e2b, 0xa7, 0xb3, 0xa0, 0xf8, 0x8e, 0xab, 0xb8, 0xae)

void am625_get_bootsource(enum bootsource *src, int *instance);
bool k3_boot_is_emmc(void);
u64 am625_sdram_size(void);
void am625_register_dram(void);
void am625_enable_32k_crystal(void);

#define K3_EMMC_BOOTPART_TIBOOT3_BIN_SIZE	SZ_1M

#endif /* __MACH_K3_COMMON_H */
