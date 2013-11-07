/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Board specific DDR tuning.
 */

#include <common.h>
#include <mach/fsl_i2c.h>
#include <mach/immap_85xx.h>
#include <mach/clock.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_ddr_dimm_params.h>
#include "product_data.h"

static struct board_info *binfo =
	(struct board_info *)(CFG_INIT_RAM_ADDR + CFG_INIT_RAM_SIZE -
				sizeof(struct board_info));

static u8 spd_addr = 0x50;

static int da923rc_boardinfo_init(void)
{
	void __iomem *i2c = IOMEM(I2C1_BASE_ADDR);
	uint8_t id;
	int ret;

	memset(binfo, 0, sizeof(struct board_info));

	binfo->bid = BOARD_TYPE_UNKNOWN;
	/* Read made from flash, use the DDR I2C API. */
	fsl_i2c_init(i2c, 400000, 0x7f);
	/* Read board id from offset 0. */
	ret = fsl_i2c_read(i2c, 0x3b, 0, 1, &id, sizeof(uint8_t));
	fsl_i2c_stop(i2c);

	if (ret == 0) {
		/*
		 * Board ID:
		 * 0-2 Hardware board
		 * revision
		 * 3-5 Board ID
		 * 000b/010b/100b - DA923, 001 - GBX460
		 * 6-7 Undefined 00
		 */
		binfo->rev = id & 7;
		id &= 0x38;
		id >>= 3;
		switch (id) {
		case 0:
		case 2:
		case 4:
			binfo->bid = BOARD_TYPE_DA923;
			break;
		case 1:
			binfo->bid = BOARD_TYPE_GBX460;
			break;
		default:
			binfo->bid = BOARD_TYPE_NONE;
		}
	}

	return ret;
}

void da923rc_boardinfo_get(struct board_info *bi)
{
	memcpy(bi, binfo, sizeof(struct board_info));
}

void fsl_ddr_board_info(struct ddr_board_info_s *info)
{
	info->fsl_ddr_ver = 0;
	info->ddr_base = IOMEM(MPC85xx_DDR_ADDR);
	/* Actual number of chip select used */
	info->cs_per_ctrl = 1;
	info->dimm_slots_per_ctrl = 1;
	info->i2c_bus = 0;
	info->i2c_slave = 0x7f;
	info->i2c_speed = 400000;
	info->i2c_base = IOMEM(I2C1_BASE_ADDR);
	info->spd_i2c_addr = &spd_addr;
}

void fsl_ddr_board_options(struct memctl_options_s *popts,
			   struct dimm_params_s *pdimm)
{
	da923rc_boardinfo_init();

	/*
	 * Clock adjustment in 1/8-cycle
	 *      0 = Clock is launched aligned with address/command
	 *      ...
	 *      6 = 3/4 cycle late
	 *      7 = 7/8 cycle late
	 *      8 = 1 cycle late
	 */
	popts->clk_adjust = 8;

	/*
	 * /MCAS-to-preamble override. Defines the number of DRAM cycles
	 * between when a read is issued and when the corresponding DQS
	 * preamble is valid for the memory controller.
	 *
	 * Factors to consider for CPO:
	 *      - frequency
	 *      - ddr type
	 */
	popts->cpo_override = 9;

	/*
	 * Write command to write data strobe timing adjustment.
	 * Factors to consider for write data delay:
	 *      - number of DIMMs
	 *
	 * 1 = 1/4 clock delay
	 * 2 = 1/2 clock delay
	 * 3 = 3/4 clock delay
	 * 4 = 1   clock delay
	 * 5 = 5/4 clock delay
	 * 6 = 3/2 clock delay
	 */
	popts->write_data_delay = 3;

	/* 2T timing disabled. */
	popts->twoT_en = 0;
	if (pdimm->registered_dimm != 0)
		hang();

	/*
	 * Factors to consider for half-strength driver enable:
	 *      - number of DIMMs installed
	 */
	popts->half_strength_driver_enable = 1;

	/* Enable additive latency override. */
	popts->additive_latency_override = 1;
	popts->additive_latency_override_value = 1;

	/* 50000ps is valid for a 16-bit wide data bus */
	popts->tFAW_window_four_activates_ps = 50000;

	/* Allow ECC */
	popts->ECC_mode = 1;
	popts->data_init = 0;

	/* DLL reset disable */
	popts->dll_rst_dis = 1;

	/* Powerdown timings in number of tCK.  */
	popts->txard = 2;
	popts->txp = 2;
	popts->taxpd = 8;

	/* Load mode timing in number of tCK. */
	popts->tmrd = 2;

	/* Assert ODT only during writes to CSn */
	popts->cs_local_opts[0].odt_wr_cfg = FSL_DDR_ODT_CS;
}
