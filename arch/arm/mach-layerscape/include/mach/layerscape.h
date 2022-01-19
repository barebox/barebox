/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_LAYERSCAPE_H
#define __MACH_LAYERSCAPE_H

#define LS1046A_DDR_SDRAM_BASE	0x80000000
#define LS1046A_DDR_FREQ	2100000000

enum bootsource ls1046_bootsource_get(void);

#ifdef CONFIG_ARCH_LAYERSCAPE_PPA
int ls1046a_ppa_init(resource_size_t ppa_start, resource_size_t ppa_size);
#else
static inline int ls1046a_ppa_init(resource_size_t ppa_start,
				   resource_size_t ppa_size)
{
	return -ENOSYS;
}
#endif

#endif /* __MACH_LAYERSCAPE_H */
