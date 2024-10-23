/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_LAYERSCAPE_XLOAD_H
#define __MACH_LAYERSCAPE_XLOAD_H

struct dram_regions_info;

int ls1046a_esdhc_start_image(struct dram_regions_info *dram_info);
int ls1028a_esdhc1_start_image(struct dram_regions_info *dram_info);
int ls1028a_esdhc2_start_image(struct dram_regions_info *dram_info);
int ls1046a_qspi_start_image(struct dram_regions_info *dram_info);
int ls1021a_qspi_start_image(void);
int ls1046a_xload_start_image(struct dram_regions_info *dram_info);
int ls1021a_xload_start_image(void);

void ls1046_start_tfa(void *barebox, struct dram_regions_info *dram_info);

#endif /* __MACH_LAYERSCAPE_XLOAD_H */
