/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_LAYERSCAPE_H
#define __MACH_LAYERSCAPE_H

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

enum bootsource ls1046a_bootsource_get(void);
enum bootsource ls1021a_bootsource_get(void);

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

#endif /* __MACH_LAYERSCAPE_H */
