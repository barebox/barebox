// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2018 NXP
 */

#define pr_fmt(fmt) "imx-ddr: " fmt

#include <common.h>
#include <linux/kernel.h>
#include <soc/imx8m/ddr.h>
#include <firmware.h>

static const u16 *lpddr4_imem_1d;
static size_t lpddr4_imem_1d_size;
static const u16 *lpddr4_dmem_1d;
static size_t lpddr4_dmem_1d_size;
static const u16 *lpddr4_imem_2d;
static size_t lpddr4_imem_2d_size;
static const u16 *lpddr4_dmem_2d;
static size_t lpddr4_dmem_2d_size;

void ddr_get_firmware_lpddr4(void)
{
	get_builtin_firmware(lpddr4_pmu_train_1d_imem_bin, &lpddr4_imem_1d,
			     &lpddr4_imem_1d_size);
	get_builtin_firmware(lpddr4_pmu_train_1d_dmem_bin, &lpddr4_dmem_1d,
			     &lpddr4_dmem_1d_size);
	get_builtin_firmware(lpddr4_pmu_train_2d_imem_bin, &lpddr4_imem_2d,
			     &lpddr4_imem_2d_size);
	get_builtin_firmware(lpddr4_pmu_train_2d_dmem_bin, &lpddr4_dmem_2d,
			     &lpddr4_dmem_2d_size);
}

static const u16 *ddr4_imem_1d;
static size_t ddr4_imem_1d_size;
static const u16 *ddr4_dmem_1d;
static size_t ddr4_dmem_1d_size;
static const u16 *ddr4_imem_2d;
static size_t ddr4_imem_2d_size;
static const u16 *ddr4_dmem_2d;
static size_t ddr4_dmem_2d_size;

void ddr_get_firmware_ddr(void)
{
	get_builtin_firmware(ddr4_imem_1d_bin, &ddr4_imem_1d,
			     &ddr4_imem_1d_size);
	get_builtin_firmware(ddr4_dmem_1d_bin, &ddr4_dmem_1d,
			     &ddr4_dmem_1d_size);
	get_builtin_firmware(ddr4_imem_2d_bin, &ddr4_imem_2d,
			     &ddr4_imem_2d_size);
	get_builtin_firmware(ddr4_dmem_2d_bin, &ddr4_dmem_2d,
			     &ddr4_dmem_2d_size);
}

void ddr_load_train_code(struct dram_controller *dram, enum dram_type dram_type,
			 enum fw_type fw_type)
{
	const u16 *imem, *dmem;
	size_t isize, dsize;

	if (dram_is_lpddr4(dram_type)) {
		if (fw_type == FW_1D_IMAGE) {
			imem = lpddr4_imem_1d;
			isize = lpddr4_imem_1d_size;
			dmem = lpddr4_dmem_1d;
			dsize = lpddr4_dmem_1d_size;
		} else {
			imem = lpddr4_imem_2d;
			isize = lpddr4_imem_2d_size;
			dmem = lpddr4_dmem_2d;
			dsize = lpddr4_dmem_2d_size;
		}
	} else if (dram_is_ddr4(dram_type)) {
		if (fw_type == FW_1D_IMAGE) {
			imem = ddr4_imem_1d;
			isize = ddr4_imem_1d_size;
			dmem = ddr4_dmem_1d;
			dsize = ddr4_dmem_1d_size;
		} else {
			imem = ddr4_imem_2d;
			isize = ddr4_imem_2d_size;
			dmem = ddr4_dmem_2d;
			dsize = ddr4_dmem_2d_size;
		}
	} else {
		panic("No matching DDR PHY firmware found");
	}

	ddrc_phy_load_firmware(dram, DDRC_PHY_IMEM, imem, isize);

	ddrc_phy_load_firmware(dram, DDRC_PHY_DMEM, dmem, dsize);
}

int ddr_cfg_phy(struct dram_controller *dram, struct dram_timing_info *dram_timing)
{
	struct dram_cfg_param *dram_cfg;
	struct dram_fsp_msg *fsp_msg;
	unsigned int num;
	int i = 0;
	int j = 0;
	int ret;

	/* initialize PHY configuration */
	dram_cfg = dram_timing->ddrphy_cfg;
	num  = dram_timing->ddrphy_cfg_num;
	for (i = 0; i < num; i++) {
		/* config phy reg */
		dwc_ddrphy_apb_wr(dram, dram_cfg->reg, dram_cfg->val);
		dram_cfg++;
	}

	/* load the frequency setpoint message block config */
	fsp_msg = dram_timing->fsp_msg;
	for (i = 0; i < dram_timing->fsp_msg_num; i++) {
		pr_debug("DRAM PHY training for %dMTS\n", fsp_msg->drate);
		/* set dram PHY input clocks to desired frequency */
		dram->set_dfi_clk(dram, fsp_msg->drate);

		/* load the dram training firmware image */
		dwc_ddrphy_apb_wr(dram, 0xd0000, 0x0);
		ddr_load_train_code(dram, dram->dram_type, fsp_msg->fw_type);

		/* load the frequency set point message block parameter */
		dram_cfg = fsp_msg->fsp_cfg;
		num = fsp_msg->fsp_cfg_num;
		for (j = 0; j < num; j++) {
			dwc_ddrphy_apb_wr(dram, dram_cfg->reg, dram_cfg->val);
			dram_cfg++;
		}

		/*
		 * -------------------- excute the firmware --------------------
		 * Running the firmware is a simply process to taking the
		 * PMU out of reset and stall, then the firwmare will be run
		 * 1. reset the PMU;
		 * 2. begin the excution;
		 * 3. wait for the training done;
		 * 4. read the message block result.
		 * -------------------------------------------------------------
		 */
		dwc_ddrphy_apb_wr(dram, 0xd0000, 0x1);
		dwc_ddrphy_apb_wr(dram, 0xd0099, 0x9);
		dwc_ddrphy_apb_wr(dram, 0xd0099, 0x1);
		dwc_ddrphy_apb_wr(dram, 0xd0099, 0x0);

		/* Wait for the training firmware to complete */
		ret = wait_ddrphy_training_complete(dram);
		if (ret)
			return ret;

		/* Halt the microcontroller. */
		dwc_ddrphy_apb_wr(dram, 0xd0099, 0x1);

		/* Read the Message Block results */
		dwc_ddrphy_apb_wr(dram, 0xd0000, 0x0);

		if (fsp_msg->fw_type != FW_2D_IMAGE)
			dram->get_trained_CDD(dram, i);

		dwc_ddrphy_apb_wr(dram, 0xd0000, 0x1);

		fsp_msg++;
	}

	/* Load PHY Init Engine Image */
	dram_cfg = dram_timing->ddrphy_pie;
	num = dram_timing->ddrphy_pie_num;
	for (i = 0; i < num; i++) {
		dwc_ddrphy_apb_wr(dram, dram_cfg->reg, dram_cfg->val);
		dram_cfg++;
	}

	/* save the ddr PHY trained CSR in memory for low power use */
	ddrphy_trained_csr_save(dram, ddrphy_trained_csr, ddrphy_trained_csr_num);

	return 0;
}
