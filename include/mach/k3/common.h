#ifndef __MACH_K3_COMMON_H
#define __MACH_K3_COMMON_H

#include <bootsource.h>
#include <linux/sizes.h>
#include <linux/uuid.h>

#define UUID_TI_DM_FW \
        UUID_INIT(0x9e8c2017, 0x8b94, 0x4e2b, 0xa7, 0xb3, 0xa0, 0xf8, 0x8e, 0xab, 0xb8, 0xae)

void am62x_get_bootsource(enum bootsource *src, int *instance);
void am62lx_get_bootsource(enum bootsource *src, int *instance);
bool am62x_boot_is_emmc(void);
u64 am62x_sdram_size(void);
void am62x_register_dram(void);
void am62x_enable_32k_crystal(void);
int k3_authenticate_image(void **buf, size_t *size);
int k3_env_init(void);

#define K3_EMMC_BOOTPART_TIBOOT3_BIN_SIZE	SZ_1M

#ifdef CONFIG_BAREBOX_UPDATE
int k3_bbu_emmc_register(const char *name,
			 const char *devicefile,
			 unsigned long flags);
#else
static inline int k3_bbu_emmc_register(const char *name,
				       const char *devicefile,
				       unsigned long flags)
{
	return 0;
}
#endif

#endif /* __MACH_K3_COMMON_H */
