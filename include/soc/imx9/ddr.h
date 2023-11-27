#ifndef __SOC_IMX9_DDR_H
#define __SOC_IMX9_DDR_H

#include <io.h>
#include <asm/types.h>
#include <soc/imx/ddr.h>

int imx9_ddr_init(struct dram_timing_info *dram_timing, enum dram_type dram_type);

static inline int imx93_ddr_init(struct dram_timing_info *dram_timing,
				 enum dram_type dram_type)
{
	ddr_get_firmware(dram_type);

	return imx9_ddr_init(dram_timing, dram_type);
}

#endif /* __SOC_IMX9_DDR_H */
