/*
 * Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/processor.h>
#include <mach/early_udelay.h>
#include "ddr.h"

int fsl_ddr_set_memctl_regs(const struct fsl_ddr_info_s *info)
{
	uint32_t i, temp_sdram_cfg;
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
		if (info->memctl_opts.sdram_type == SDRAM_TYPE_DDR3)
			out_be32(ddr + DDR_OFF(CS0_CONFIG_2) + (i << 2),
					regs->cs[i].config_2);
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

	if (info->memctl_opts.sdram_type == SDRAM_TYPE_DDR3) {
		out_be32(ddr + DDR_OFF(TIMING_CFG_4), regs->timing_cfg_4);
		out_be32(ddr + DDR_OFF(TIMING_CFG_5), regs->timing_cfg_5);
		out_be32(ddr + DDR_OFF(ZQ_CNTL), regs->ddr_zq_cntl);
		out_be32(ddr + DDR_OFF(WRLVL_CNTL), regs->ddr_wrlvl_cntl);

		if (regs->ddr_wrlvl_cntl_2)
			out_be32(ddr + DDR_OFF(WRLVL_CNTL_2),
					regs->ddr_wrlvl_cntl_2);
		if (regs->ddr_wrlvl_cntl_3)
			out_be32(ddr + DDR_OFF(WRLVL_CNTL_3),
					regs->ddr_wrlvl_cntl_3);

		out_be32(ddr + DDR_OFF(SR_CNTL), regs->ddr_sr_cntr);
		out_be32(ddr + DDR_OFF(SDRAM_RCW_1), regs->ddr_sdram_rcw_1);
		out_be32(ddr + DDR_OFF(SDRAM_RCW_2), regs->ddr_sdram_rcw_2);
		out_be32(ddr + DDR_OFF(DDRCDR1), regs->ddr_cdr1);
		out_be32(ddr + DDR_OFF(DDRCDR2), regs->ddr_cdr2);
	}

	out_be32(ddr + DDR_OFF(ERR_DISABLE), regs->err_disable);
	out_be32(ddr + DDR_OFF(ERR_INT_EN), regs->err_int_en);

	temp_sdram_cfg = regs->ddr_sdram_cfg;
	temp_sdram_cfg &= ~(SDRAM_CFG_MEM_EN);
	out_be32(ddr + DDR_OFF(SDRAM_CFG), temp_sdram_cfg);

	early_udelay(500);
	/* Make sure all instructions are completed before enabling memory.*/
	asm volatile("sync;isync");
	temp_sdram_cfg = in_be32(ddr + DDR_OFF(SDRAM_CFG)) & ~SDRAM_CFG_BI;
	out_be32(ddr + DDR_OFF(SDRAM_CFG), temp_sdram_cfg | SDRAM_CFG_MEM_EN);
	asm volatile("sync;isync");

	while (in_be32(ddr + DDR_OFF(SDRAM_CFG_2)) & SDRAM_CFG2_D_INIT)
		early_udelay(10000);

	return 0;
}
