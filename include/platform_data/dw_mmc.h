#ifndef __INCLUDE_PLATFORM_DATA_DW_MMC_H
#define __INCLUDE_PLATFORM_DATA_DW_MMC_H

struct dw_mmc_platform_data {
	unsigned int bus_width_caps;
	int ciu_div;
};

#endif /* __INCLUDE_PLATFORM_DATA_DW_MMC_H */
