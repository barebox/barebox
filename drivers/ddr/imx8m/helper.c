// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 */

#define pr_fmt(fmt) "imx8m-ddr: " fmt

#include <common.h>
#include <io.h>
#include <errno.h>
#include <soc/imx8m/ddr.h>

#define IMEM_LEN 32768 /* byte */
#define DMEM_LEN 16384 /* byte */
#define IMEM_2D_OFFSET	49152

#define IMEM_OFFSET_ADDR 0x00050000
#define DMEM_OFFSET_ADDR 0x00054000
#define DDR_TRAIN_CODE_BASE_ADDR IP2APB_DDRPHY_IPS_BASE_ADDR(0)

void ddrphy_trained_csr_save(struct dram_cfg_param *ddrphy_csr,
			     unsigned int num)
{
	int i = 0;

	/* enable the ddrphy apb */
	dwc_ddrphy_apb_wr(0xd0000, 0x0);
	dwc_ddrphy_apb_wr(0xc0080, 0x3);
	for (i = 0; i < num; i++) {
		ddrphy_csr->val = dwc_ddrphy_apb_rd(ddrphy_csr->reg);
		ddrphy_csr++;
	}
	/* disable the ddrphy apb */
	dwc_ddrphy_apb_wr(0xc0080, 0x2);
	dwc_ddrphy_apb_wr(0xd0000, 0x1);
}

void dram_config_save(struct dram_timing_info *timing_info,
		      unsigned long saved_timing_base)
{
	int i = 0;
	struct dram_timing_info *saved_timing = (void *)saved_timing_base;
	struct dram_cfg_param *cfg;

	saved_timing->ddrc_cfg_num = timing_info->ddrc_cfg_num;
	saved_timing->ddrphy_cfg_num = timing_info->ddrphy_cfg_num;
	saved_timing->ddrphy_trained_csr_num = ddrphy_trained_csr_num;
	saved_timing->ddrphy_pie_num = timing_info->ddrphy_pie_num;

	/* save the fsp table */
	for (i = 0; i < 4; i++)
		saved_timing->fsp_table[i] = timing_info->fsp_table[i];

	cfg = (struct dram_cfg_param *)(saved_timing_base +
					sizeof(*timing_info));

	/* save ddrc config */
	saved_timing->ddrc_cfg = cfg;
	for (i = 0; i < timing_info->ddrc_cfg_num; i++) {
		cfg->reg = timing_info->ddrc_cfg[i].reg;
		cfg->val = timing_info->ddrc_cfg[i].val;
		cfg++;
	}

	/* save ddrphy config */
	saved_timing->ddrphy_cfg = cfg;
	for (i = 0; i < timing_info->ddrphy_cfg_num; i++) {
		cfg->reg = timing_info->ddrphy_cfg[i].reg;
		cfg->val = timing_info->ddrphy_cfg[i].val;
		cfg++;
	}

	/* save the ddrphy csr */
	saved_timing->ddrphy_trained_csr = cfg;
	for (i = 0; i < ddrphy_trained_csr_num; i++) {
		cfg->reg = ddrphy_trained_csr[i].reg;
		cfg->val = ddrphy_trained_csr[i].val;
		cfg++;
	}

	/* save the ddrphy pie */
	saved_timing->ddrphy_pie = cfg;
	for (i = 0; i < timing_info->ddrphy_pie_num; i++) {
		cfg->reg = timing_info->ddrphy_pie[i].reg;
		cfg->val = timing_info->ddrphy_pie[i].val;
		cfg++;
	}
}
