/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_LAYERSCAPE_H
#define __MACH_LAYERSCAPE_H

#include <linux/sizes.h>

#define LS1046A_DDR_SDRAM_BASE		0x80000000
#define LS1046A_DDR_FREQ		2100000000

#define LS1021A_DDR_SDRAM_BASE		0x80000000
#define LS1021A_DDR_FREQ		1600000000

#define LS1028A_DDR_SDRAM_BASE		0x80000000
#define LS1028A_DDR_SDRAM_LOWMEM_SIZE	0x80000000
#define LS1028A_DDR_SDRAM_HIGHMEM_BASE	0x2080000000
#define LS1028A_SECURE_DRAM_SIZE	SZ_64M
#define LS1028A_SP_SHARED_DRAM_SIZE	SZ_2M
#define LS1028A_TZC400_BASE		0x01100000

#define LS1028A_TFA_SIZE		SZ_64M
#define LS1028A_TFA_SHRD		SZ_2M
#define LS1028A_TFA_RESERVED_SIZE	(LS1028A_TFA_SIZE + LS1028A_TFA_SHRD)
#define LS1028A_TFA_RESERVED_START	(0x100000000 - LS1028A_TFA_RESERVED_SIZE)
#define LS1028A_TFA_START		(0x100000000 - LS1028A_TFA_SIZE)

enum bootsource ls1046a_bootsource_get(void);
enum bootsource ls1021a_bootsource_get(void);

#define LAYERSCAPE_SOC_LS1021A		1021
#define LAYERSCAPE_SOC_LS1028A		1028
#define LAYERSCAPE_SOC_LS1046A		1046

#ifdef CONFIG_ARCH_LAYERSCAPE_PPA
int ls1046a_ppa_init(resource_size_t ppa_start, resource_size_t ppa_size);
#else
static inline int ls1046a_ppa_init(resource_size_t ppa_start,
				   resource_size_t ppa_size)
{
	return -ENOSYS;
}
#endif

struct dram_region_info {
        uint64_t addr;
        uint64_t size;
};
#define NUM_DRAM_REGIONS  3

struct dram_regions_info {
        uint64_t num_dram_regions;
        int64_t total_dram_size;
        struct dram_region_info region[NUM_DRAM_REGIONS];
};

void ls1021a_bootsource_init(void);
void ls1028a_bootsource_init(void);
void ls1046a_bootsource_init(void);
void layerscape_register_pbl_image_handler(void);
void ls102xa_smmu_stream_id_init(void);
void ls1021a_restart_register_feature(void);
void ls1028a_setup_icids(void);
void ls1046a_setup_icids(void);

extern int __layerscape_soc_type;

static inline bool cpu_is_ls1021a(void)
{
	return IS_ENABLED(CONFIG_ARCH_LS1021) &&
		__layerscape_soc_type == LAYERSCAPE_SOC_LS1021A;
}

static inline bool cpu_is_ls1028a(void)
{
	return IS_ENABLED(CONFIG_ARCH_LS1028) &&
		__layerscape_soc_type == LAYERSCAPE_SOC_LS1028A;
}

static inline bool cpu_is_ls1046a(void)
{
	return IS_ENABLED(CONFIG_ARCH_LS1046) &&
		__layerscape_soc_type == LAYERSCAPE_SOC_LS1046A;
}

#endif /* __MACH_LAYERSCAPE_H */
