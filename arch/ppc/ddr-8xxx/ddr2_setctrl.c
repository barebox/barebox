/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <asm/fsl_ddr_sdram.h>
#include <mach/early_udelay.h>
#include "ddr.h"

int fsl_ddr_set_memctl_regs(const struct fsl_ddr_info_s *info)
{
	uint32_t i;
	void __iomem *ddr;
	const struct fsl_ddr_cfg_regs_s *regs;

	regs = &info->fsl_ddr_config_reg;
	ddr = info->board_info.ddr_base;

	if (in_be32(ddr + DDR_OFF(SDRAM_CFG)) & SDRAM_CFG_MEM_EN)
		return 0;

	for (i = 0; i < info->board_info.cs_per_ctrl; i++) {
		out_be32(ddr + DDR_OFF(CS0_BNDS) + (i << 3), regs->cs[i].bnds);
		out_be32(ddr + DDR_OFF(CS0_CONFIG) + (i << 2),
				regs->cs[i].config);
	}

	out_be32(ddr + DDR_OFF(TIMING_CFG_3), regs->timing_cfg_3);
	out_be32(ddr + DDR_OFF(TIMING_CFG_0), regs->timing_cfg_0);
	out_be32(ddr + DDR_OFF(TIMING_CFG_1), regs->timing_cfg_1);
	out_be32(ddr + DDR_OFF(TIMING_CFG_2), regs->timing_cfg_2);
	out_be32(ddr + DDR_OFF(SDRAM_CFG_2), regs->ddr_sdram_cfg_2);
	out_be32(ddr + DDR_OFF(SDRAM_MODE), regs->ddr_sdram_mode);
	out_be32(ddr + DDR_OFF(SDRAM_MODE_2), regs->ddr_sdram_mode_2);
	out_be32(ddr + DDR_OFF(SDRAM_MD_CNTL), regs->ddr_sdram_md_cntl);
	out_be32(ddr + DDR_OFF(SDRAM_INTERVAL), regs->ddr_sdram_interval);
	out_be32(ddr + DDR_OFF(SDRAM_DATA_INIT), regs->ddr_data_init);
	out_be32(ddr + DDR_OFF(SDRAM_CLK_CNTL), regs->ddr_sdram_clk_cntl);
	out_be32(ddr + DDR_OFF(SDRAM_INIT_ADDR), regs->ddr_init_addr);
	out_be32(ddr + DDR_OFF(SDRAM_INIT_ADDR_EXT), regs->ddr_init_ext_addr);

	early_udelay(200);
	asm volatile("sync;isync");

	out_be32(ddr + DDR_OFF(SDRAM_CFG), regs->ddr_sdram_cfg);

	/* Poll DDR_SDRAM_CFG_2[D_INIT] bit until auto-data init is done.  */
	while (in_be32(ddr + DDR_OFF(SDRAM_CFG_2)) & SDRAM_CFG2_D_INIT)
		early_udelay(10000);

	return 0;
}
