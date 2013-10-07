/*
Copyright (c) 2012, Altera Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Altera Corporation nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ALTERA CORPORATION BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <common.h>
#include <io.h>
#include <mach/socfpga-regs.h>
#include <mach/sdram.h>
#include <mach/sequencer.h>

static void IOWR_32DIRECT(uint32_t base, uint32_t ofs, uint32_t val)
{
	writel(val, CYCLONE5_SDR_ADDRESS + base + ofs);
}

static uint32_t IORD_32DIRECT(uint32_t base, uint32_t ofs)
{
	return readl(CYCLONE5_SDR_ADDRESS + base + ofs);
}

/* Just to make the debugging code more uniform */
#ifndef RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM
#define RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM 0
#endif

#if HALF_RATE
#define HALF_RATE_MODE 1
#else
#define HALF_RATE_MODE 0
#endif

#if QUARTER_RATE
#define QUARTER_RATE_MODE 1
#else
#define QUARTER_RATE_MODE 0
#endif
#define DELTA_D 1

#define BTFLD_FMT "%x"

#define STATIC_CALIB_STEPS (CALIB_SKIP_FULL_TEST)

/* calibration steps requested by the rtl */
static uint16_t dyn_calib_steps;

static uint32_t vfifo_idx;

/*
 * To make CALIB_SKIP_DELAY_LOOPS a dynamic conditional option
 * instead of static, we use boolean logic to select between
 * non-skip and skip values
 *
 * The mask is set to include all bits when not-skipping, but is
 * zero when skipping
 */

static uint16_t skip_delay_mask;	/* mask off bits when skipping/not-skipping */

#define SKIP_DELAY_LOOP_VALUE_OR_ZERO(non_skip_value) \
	((non_skip_value) & skip_delay_mask)

static gbl_t *gbl;
static param_t *param;

static uint32_t rw_mgr_mem_calibrate_write_test(uint32_t rank_bgn,
	uint32_t write_group, uint32_t use_dm,
	uint32_t all_correct, t_btfld * bit_chk, uint32_t all_ranks);

/*
 * This (TEST_SIZE) is used to test handling of large roms, to make
 * sure we are sizing things correctly
 * Note, the initialized data takes up twice the space in rom, since
 * there needs to be a copy with the initial value and a copy that is
 * written too, since on soft-reset, it needs to have the initial values
 * without reloading the memory from external sources
 */

static void reg_file_set_group(uint32_t set_group)
{
	/* Read the current group and stage */
	uint32_t cur_stage_group = IORD_32DIRECT(REG_FILE_CUR_STAGE, 0);

	/* Clear the group */
	cur_stage_group &= 0x0000FFFF;

	/* Set the group */
	cur_stage_group |= (set_group << 16);

	/* Write the data back */
	IOWR_32DIRECT(REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static void reg_file_set_stage(uint32_t set_stage)
{
	/* Read the current group and stage */
	uint32_t cur_stage_group = IORD_32DIRECT(REG_FILE_CUR_STAGE, 0);

	/* Clear the stage and substage */
	cur_stage_group &= 0xFFFF0000;

	/* Set the stage */
	cur_stage_group |= (set_stage & 0x000000FF);

	/* Write the data back */
	IOWR_32DIRECT(REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static void reg_file_set_sub_stage(uint32_t set_sub_stage)
{
	/* Read the current group and stage */
	uint32_t cur_stage_group = IORD_32DIRECT(REG_FILE_CUR_STAGE, 0);

	/* Clear the substage */
	cur_stage_group &= 0xFFFF00FF;

	/* Set the sub stage */
	cur_stage_group |= ((set_sub_stage << 8) & 0x0000FF00);

	/* Write the data back */
	IOWR_32DIRECT(REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static void initialize(void)
{
	/*
	 * In Hard PHY this is a 2-bit control:
	 * 0: AFI Mux Select
	 * 1: DDIO Mux Select
	 */
	IOWR_32DIRECT(PHY_MGR_MUX_SEL, 0, 0x3);

	/* USER memory clock is not stable we begin initialization  */

	IOWR_32DIRECT(PHY_MGR_RESET_MEM_STBL, 0, 0);

	/* USER calibration status all set to zero */

	IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, 0);
	IOWR_32DIRECT(PHY_MGR_CAL_DEBUG_INFO, 0, 0);

	param->read_correct_mask_vg  = ((t_btfld)1 << (RW_MGR_MEM_DQ_PER_READ_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS)) - 1;
	param->write_correct_mask_vg = ((t_btfld)1 << (RW_MGR_MEM_DQ_PER_READ_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS)) - 1;
	param->read_correct_mask     = ((t_btfld)1 << RW_MGR_MEM_DQ_PER_READ_DQS) - 1;
	param->write_correct_mask    = ((t_btfld)1 << RW_MGR_MEM_DQ_PER_WRITE_DQS) - 1;
}

#if DDR3
static void set_rank_and_odt_mask(uint32_t rank, uint32_t odt_mode)
{
	uint32_t odt_mask_0 = 0;
	uint32_t odt_mask_1 = 0;
	uint32_t cs_and_odt_mask;

	if (odt_mode == RW_MGR_ODT_MODE_READ_WRITE) {
		if (RW_MGR_MEM_NUMBER_OF_RANKS == 1) {
			/*
			 * 1 Rank
			 * Read: ODT = 0
			 * Write: ODT = 1
			 */
			odt_mask_0 = 0x0;
			odt_mask_1 = 0x1;
		} else if (RW_MGR_MEM_NUMBER_OF_RANKS == 2) {
			/* 2 Ranks */
			if (RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 1 ||
			   (RDIMM && RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 2
			   && RW_MGR_MEM_CHIP_SELECT_WIDTH == 4)) {
				/* - Dual-Slot , Single-Rank
				 * (1 chip-select per DIMM)
				 * OR
				 * - RDIMM, 4 total CS (2 CS per DIMM)
				 * means 2 DIMM
				 * Since MEM_NUMBER_OF_RANKS is 2 they are
				 * both single rank
				 * with 2 CS each (special for RDIMM)
				 * Read: Turn on ODT on the opposite rank
				 * Write: Turn on ODT on all ranks
				 */
				odt_mask_0 = 0x3 & ~(1 << rank);
				odt_mask_1 = 0x3;
			} else {
				/*
				 * USER - Single-Slot , Dual-rank DIMMs
				 * (2 chip-selects per DIMM)
				 * USER Read: Turn on ODT off on all ranks
				 * USER Write: Turn on ODT on active rank
				 */
				odt_mask_0 = 0x0;
				odt_mask_1 = 0x3 & (1 << rank);
			}
				} else {
			/* 4 Ranks
			 * Read:
			 * ----------+-----------------------+
			 *           |                       |
			 *           |         ODT           |
			 * Read From +-----------------------+
			 *   Rank    |  3  |  2  |  1  |  0  |
			 * ----------+-----+-----+-----+-----+
			 *     0     |  0  |  1  |  0  |  0  |
			 *     1     |  1  |  0  |  0  |  0  |
			 *     2     |  0  |  0  |  0  |  1  |
			 *     3     |  0  |  0  |  1  |  0  |
			 * ----------+-----+-----+-----+-----+
			 *
			 * Write:
			 * ----------+-----------------------+
			 *           |                       |
			 *           |         ODT           |
			 * Write To  +-----------------------+
			 *   Rank    |  3  |  2  |  1  |  0  |
			 * ----------+-----+-----+-----+-----+
			 *     0     |  0  |  1  |  0  |  1  |
			 *     1     |  1  |  0  |  1  |  0  |
			 *     2     |  0  |  1  |  0  |  1  |
			 *     3     |  1  |  0  |  1  |  0  |
			 * ----------+-----+-----+-----+-----+
			 */
			switch (rank) {
			case 0:
				odt_mask_0 = 0x4;
				odt_mask_1 = 0x5;
				break;
			case 1:
				odt_mask_0 = 0x8;
				odt_mask_1 = 0xA;
				break;
			case 2:
				odt_mask_0 = 0x1;
				odt_mask_1 = 0x5;
				break;
			case 3:
				odt_mask_0 = 0x2;
				odt_mask_1 = 0xA;
				break;
			}
		}
	} else {
		odt_mask_0 = 0x0;
		odt_mask_1 = 0x0;
	}

	if (RDIMM && RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 2
		&& RW_MGR_MEM_CHIP_SELECT_WIDTH == 4
		&& RW_MGR_MEM_NUMBER_OF_RANKS == 2) {
		/* See RDIMM special case above */
		cs_and_odt_mask =
			(0xFF & ~(1 << (2*rank))) |
			((0xFF & odt_mask_0) << 8) |
			((0xFF & odt_mask_1) << 16);
	} else {
		cs_and_odt_mask =
			(0xFF & ~(1 << rank)) |
			((0xFF & odt_mask_0) << 8) |
			((0xFF & odt_mask_1) << 16);
	}

	IOWR_32DIRECT(RW_MGR_SET_CS_AND_ODT_MASK, 0, cs_and_odt_mask);
}
#else
static void set_rank_and_odt_mask(uint32_t rank, uint32_t odt_mode)
{
	uint32_t odt_mask_0 = 0;
	uint32_t odt_mask_1 = 0;
	uint32_t cs_and_odt_mask;

	if (odt_mode == RW_MGR_ODT_MODE_READ_WRITE) {
		if (RW_MGR_MEM_NUMBER_OF_RANKS == 1) {
			/*
			 * 1 Rank
			 * Read: ODT = 0
			 * Write: ODT = 1
			 */
			odt_mask_0 = 0x0;
			odt_mask_1 = 0x1;
		} else if (RW_MGR_MEM_NUMBER_OF_RANKS == 2) {
			/* 2 Ranks */
			if (RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 1) {
				/* USER - Dual-Slot ,
				 * Single-Rank (1 chip-select per DIMM)
				 * OR
				 * - RDIMM, 4 total CS (2 CS per DIMM) means
				 * 2 DIMM
				 * Since MEM_NUMBER_OF_RANKS is 2 they are both
				 * single rank with 2 CS each (special for
				 * RDIMM)
				 * Read/Write: Turn on ODT on the opposite rank
				 */
				odt_mask_0 = 0x3 & ~(1 << rank);
				odt_mask_1 = 0x3 & ~(1 << rank);
			} else {
				/*
				 * USER - Single-Slot , Dual-rank DIMMs
				 * (2 chip-selects per DIMM)
				 * Read: Turn on ODT off on all ranks
				 * Write: Turn on ODT on active rank
				 */
				odt_mask_0 = 0x0;
				odt_mask_1 = 0x3 & (1 << rank);
			}
		} else {
			/*
			 * 4 Ranks
			 * Read/Write:
			 * -----------+-----------------------+
			 *            |                       |
			 *            |         ODT           |
			 * Read/Write |                       |
			 *   From     +-----------------------+
			 *   Rank     |  3  |  2  |  1  |  0  |
			 * -----------+-----+-----+-----+-----+
			 *     0      |  0  |  1  |  0  |  0  |
			 *     1      |  1  |  0  |  0  |  0  |
			 *     2      |  0  |  0  |  0  |  1  |
			 *     3      |  0  |  0  |  1  |  0  |
			 * -----------+-----+-----+-----+-----+
			 */
			switch (rank) {
			case 0:
				odt_mask_0 = 0x4;
				odt_mask_1 = 0x4;
				break;
			case 1:
				odt_mask_0 = 0x8;
				odt_mask_1 = 0x8;
				break;
			case 2:
				odt_mask_0 = 0x1;
				odt_mask_1 = 0x1;
				break;
			case 3:
				odt_mask_0 = 0x2;
				odt_mask_1 = 0x2;
				break;
			}
		}
	} else {
		odt_mask_0 = 0x0;
		odt_mask_1 = 0x0;
	}

	cs_and_odt_mask = (0xFF & ~(1 << rank)) |
		((0xFF & odt_mask_0) << 8) |
		((0xFF & odt_mask_1) << 16);

	IOWR_32DIRECT(RW_MGR_SET_CS_AND_ODT_MASK, 0, cs_and_odt_mask);
}
#endif

static void scc_mgr_initialize(void)
{
	/*
	 * Clear register file for HPS
	 * 16 (2^4) is the size of the full register file in the scc mgr:
	 *	RFILE_DEPTH = log2(MEM_DQ_PER_DQS + 1 + MEM_DM_PER_DQS +
	 * MEM_IF_READ_DQS_WIDTH - 1) + 1;
	 */
	uint32_t i;
	for (i = 0; i < 16; i++) {
		pr_debug("Clearing SCC RFILE index %u\n", i);
		IOWR_32DIRECT(SCC_MGR_HHP_RFILE, i << 2, 0);
	}
}

static void scc_mgr_set_dqs_bus_in_delay(uint32_t read_group, uint32_t delay)
{
	ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQS_IN_DELAY(read_group, delay);
}

static void scc_mgr_set_dqs_io_in_delay(uint32_t write_group,
	uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQS_IO_IN_DELAY(delay);
}

static void scc_mgr_set_dqs_en_phase(uint32_t read_group, uint32_t phase)
{
	ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQS_EN_PHASE(read_group, phase);
}

static void scc_mgr_set_dqs_en_phase_all_ranks (uint32_t read_group, uint32_t phase)
{
	uint32_t r;
	uint32_t update_scan_chains;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS;
		r += NUM_RANKS_PER_SHADOW_REG) {
		/*
		 * USER although the h/w doesn't support different phases per
		 * shadow register, for simplicity our scc manager modeling
		 * keeps different phase settings per shadow reg, and it's
		 * important for us to keep them in sync to match h/w.
		 * for efficiency, the scan chain update should occur only
		 * once to sr0.
		 */
		update_scan_chains = (r == 0) ? 1 : 0;

		scc_mgr_set_dqs_en_phase(read_group, phase);

		if (update_scan_chains) {
			IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, read_group);
			IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
		}
	}
}

static void scc_mgr_set_dqdqs_output_phase(uint32_t write_group,
	uint32_t phase)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQDQS_OUT_PHASE(write_group, phase);
}

static void scc_mgr_set_dqdqs_output_phase_all_ranks (uint32_t write_group,
	uint32_t phase)
{
	uint32_t r;
	uint32_t update_scan_chains;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS;
		r += NUM_RANKS_PER_SHADOW_REG) {
		/*
		 * USER although the h/w doesn't support different phases per
		 * shadow register, for simplicity our scc manager modeling
		 * keeps different phase settings per shadow reg, and it's
		 * important for us to keep them in sync to match h/w.
		 * for efficiency, the scan chain update should occur only
		 * once to sr0.
		 */
		update_scan_chains = (r == 0) ? 1 : 0;

		scc_mgr_set_dqdqs_output_phase(write_group, phase);

		if (update_scan_chains) {
			IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, write_group);
			IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
		}
	}
}

static void scc_mgr_set_dqs_en_delay(uint32_t read_group, uint32_t delay)
{
	ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQS_EN_DELAY(read_group, delay);
}

static void scc_mgr_set_dqs_en_delay_all_ranks (uint32_t read_group, uint32_t delay)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		scc_mgr_set_dqs_en_delay(read_group, delay);

		IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, read_group);

		/*
		 * In shadow register mode, the T11 settings are stored in
		 * registers in the core, which are updated by the DQS_ENA
		 * signals. Not issuing the SCC_MGR_UPD command allows us to
		 * save lots of rank switching overhead, by calling
		 * select_shadow_regs_for_update with update_scan_chains
		 * set to 0.
		 */
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

static void scc_mgr_set_oct_out1_delay(uint32_t write_group, uint32_t delay)
{
	uint32_t read_group;

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	/*
	 * Load the setting in the SCC manager
	 * Although OCT affects only write data, the OCT delay is controlled
	 * by the DQS logic block which is instantiated once per read group.
	 * For protocols where a write group consists of multiple read groups,
	 * the setting must be set multiple times.
	 */
	for (read_group = write_group * RW_MGR_NUM_DQS_PER_WRITE_GROUP;
		read_group < (write_group + 1) * RW_MGR_NUM_DQS_PER_WRITE_GROUP;
		 ++read_group)
		WRITE_SCC_OCT_OUT1_DELAY(read_group, delay);
}

static void scc_mgr_set_oct_out2_delay(uint32_t write_group, uint32_t delay)
{
	uint32_t read_group;

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	/*
	 * Load the setting in the SCC manager
	 * Although OCT affects only write data, the OCT delay is controlled
	 * by the DQS logic block which is instantiated once per read group.
	 * For protocols where a write group consists
	 * of multiple read groups, the setting must be set multiple times.
	 */
	for (read_group = write_group * RW_MGR_NUM_DQS_PER_WRITE_GROUP;
		read_group < (write_group + 1) * RW_MGR_NUM_DQS_PER_WRITE_GROUP;
		 ++read_group)
		WRITE_SCC_OCT_OUT2_DELAY(read_group, delay);
}

static void scc_mgr_set_dq_out1_delay(uint32_t write_group,
	uint32_t dq_in_group, uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQ_OUT1_DELAY(dq_in_group, delay);
}

static void scc_mgr_set_dq_out2_delay(uint32_t write_group,
	uint32_t dq_in_group, uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQ_OUT2_DELAY(dq_in_group, delay);
}

static void scc_mgr_set_dq_in_delay(uint32_t write_group,
	uint32_t dq_in_group, uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQ_IN_DELAY(dq_in_group, delay);
}

static void scc_mgr_set_hhp_extras(void)
{
	/*
	 * Load the fixed setting in the SCC manager
	 * bits: 0:0 = 1'b1   - dqs bypass
	 * bits: 1:1 = 1'b1   - dq bypass
	 * bits: 4:2 = 3'b001   - rfifo_mode
	 * bits: 6:5 = 2'b01  - rfifo clock_select
	 * bits: 7:7 = 1'b0  - separate gating from ungating setting
	 * bits: 8:8 = 1'b0  - separate OE from Output delay setting
	 */
	uint32_t value = (0<<8) | (0<<7) | (1<<5) | (1<<2) | (1<<1) | (1<<0);
	WRITE_SCC_HHP_EXTRAS(value);
}

static void scc_mgr_set_dqs_out1_delay(uint32_t write_group,
	uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQS_IO_OUT1_DELAY(delay);
}

static void scc_mgr_set_dqs_out2_delay(uint32_t write_group, uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DQS_IO_OUT2_DELAY(delay);
}

static void scc_mgr_set_dm_out1_delay(uint32_t write_group,
	uint32_t dm, uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dm < RW_MGR_NUM_DM_PER_WRITE_GROUP);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DM_IO_OUT1_DELAY(dm, delay);
}

static void scc_mgr_set_dm_out2_delay(uint32_t write_group, uint32_t dm,
	uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dm < RW_MGR_NUM_DM_PER_WRITE_GROUP);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DM_IO_OUT2_DELAY(dm, delay);
}

static void scc_mgr_set_dm_in_delay(uint32_t write_group,
	uint32_t dm, uint32_t delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dm < RW_MGR_NUM_DM_PER_WRITE_GROUP);

	/* Load the setting in the SCC manager */
	WRITE_SCC_DM_IO_IN_DELAY(dm, delay);
}

/*
 * USER Zero all DQS config
 * TODO: maybe rename to scc_mgr_zero_dqs_config (or something)
 */
static void scc_mgr_zero_all (void)
{
	uint32_t i, r;

	/*
	 * USER Zero all DQS config settings, across all groups and all
	 * shadow registers
	 */
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
			/*
			 * The phases actually don't exist on a per-rank basis,
			 * but there's no harm updating them several times, so
			 * let's keep the code simple.
			 */
			scc_mgr_set_dqs_bus_in_delay(i, IO_DQS_IN_RESERVE);
			scc_mgr_set_dqs_en_phase(i, 0);
			scc_mgr_set_dqs_en_delay(i, 0);
		}

		for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
			scc_mgr_set_dqdqs_output_phase(i, 0);
#if ARRIAV || CYCLONEV
			/* av/cv don't have out2 */
			scc_mgr_set_oct_out1_delay(i, IO_DQS_OUT_RESERVE);
#else
			scc_mgr_set_oct_out1_delay(i, 0);
			scc_mgr_set_oct_out2_delay(i, IO_DQS_OUT_RESERVE);
#endif
		}

		/* multicast to all DQS group enables */
		IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, 0xff);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

static void scc_set_bypass_mode(uint32_t write_group)
{
	/* only need to set once for all groups, pins, dq, dqs, dm */
	if (write_group == 0) {
		pr_debug("Setting HHP Extras\n");
		scc_mgr_set_hhp_extras();
		pr_debug("Done Setting HHP Extras\n");
	}

	/* multicast to all DQ enables */
	IOWR_32DIRECT(SCC_MGR_DQ_ENA, 0, 0xff);

	IOWR_32DIRECT(SCC_MGR_DM_ENA, 0, 0xff);

	/* update current DQS IO enable */
	IOWR_32DIRECT(SCC_MGR_DQS_IO_ENA, 0, 0);

	/* update the DQS logic */
	IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, write_group);

	/* hit update */
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
}

static void scc_mgr_zero_group (uint32_t write_group, uint32_t test_begin,
	int32_t out_only)
{
	uint32_t i, r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r +=
		NUM_RANKS_PER_SHADOW_REG) {

		/* Zero all DQ config settings */
		for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
			scc_mgr_set_dq_out1_delay(write_group, i, 0);
			scc_mgr_set_dq_out2_delay(write_group, i,
				IO_DQ_OUT_RESERVE);
			if (!out_only) {
				scc_mgr_set_dq_in_delay(write_group, i, 0);
			}
		}

		/* multicast to all DQ enables */
		IOWR_32DIRECT(SCC_MGR_DQ_ENA, 0, 0xff);

		/* Zero all DM config settings */
		for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
			if (!out_only) {
				/* Do we really need this? */
				scc_mgr_set_dm_in_delay(write_group, i, 0);
			}
			scc_mgr_set_dm_out1_delay(write_group, i, 0);
			scc_mgr_set_dm_out2_delay(write_group, i,
				IO_DM_OUT_RESERVE);
		}

		/* multicast to all DM enables */
		IOWR_32DIRECT(SCC_MGR_DM_ENA, 0, 0xff);

		/* zero all DQS io settings */
		if (!out_only) {
			scc_mgr_set_dqs_io_in_delay(write_group, 0);
		}
#if ARRIAV || CYCLONEV
		/* av/cv don't have out2 */
		scc_mgr_set_dqs_out1_delay(write_group, IO_DQS_OUT_RESERVE);
#else
		scc_mgr_set_dqs_out1_delay(write_group, 0);
		scc_mgr_set_dqs_out2_delay(write_group, IO_DQS_OUT_RESERVE);
#endif

		/* multicast to all DQS IO enables (only 1) */
		IOWR_32DIRECT(SCC_MGR_DQS_IO_ENA, 0, 0);

		/* hit update to zero everything */
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

/* load up dqs config settings */

static void scc_mgr_load_dqs (uint32_t dqs)
{
	IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, dqs);
}

static void scc_mgr_load_dqs_for_write_group (uint32_t write_group)
{
	uint32_t read_group;

	/*
	 * Although OCT affects only write data, the OCT delay is controlled
	 * by the DQS logic block which is instantiated once per read group.
	 * For protocols where a write group consists of multiple read groups,
	 * the setting must be scanned multiple times.
	 */
	for (read_group = write_group * RW_MGR_NUM_DQS_PER_WRITE_GROUP;
		read_group < (write_group + 1) * RW_MGR_NUM_DQS_PER_WRITE_GROUP;
		++read_group)
		IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, read_group);
}


/* load up dqs io config settings */

static void scc_mgr_load_dqs_io (void)
{
	IOWR_32DIRECT(SCC_MGR_DQS_IO_ENA, 0, 0);
}

/* load up dq config settings */

static void scc_mgr_load_dq (uint32_t dq_in_group)
{
	IOWR_32DIRECT(SCC_MGR_DQ_ENA, 0, dq_in_group);
}

/* load up dm config settings */

static void scc_mgr_load_dm (uint32_t dm)
{
	IOWR_32DIRECT(SCC_MGR_DM_ENA, 0, dm);
}

/* apply and load a particular input delay for the DQ pins in a group */
/* group_bgn is the index of the first dq pin (in the write group) */

static void scc_mgr_apply_group_dq_in_delay (uint32_t write_group,
	uint32_t group_bgn, uint32_t delay)
{
	uint32_t i, p;

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
		scc_mgr_set_dq_in_delay(write_group, p, delay);
		scc_mgr_load_dq (p);
	}
}

/* apply and load a particular output delay for the DQ pins in a group */

static void scc_mgr_apply_group_dq_out1_delay (uint32_t write_group, uint32_t group_bgn,
	uint32_t delay1)
{
	uint32_t i, p;

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		scc_mgr_set_dq_out1_delay(write_group, i, delay1);
		scc_mgr_load_dq (i);
	}
}

/* apply and load a particular output delay for the DM pins in a group */

static void scc_mgr_apply_group_dm_out1_delay (uint32_t write_group, uint32_t delay1)
{
	uint32_t i;

	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
		scc_mgr_set_dm_out1_delay(write_group, i, delay1);
		scc_mgr_load_dm (i);
	}
}


/* apply and load delay on both DQS and OCT out1 */
static void scc_mgr_apply_group_dqs_io_and_oct_out1 (uint32_t write_group, uint32_t delay)
{
	scc_mgr_set_dqs_out1_delay(write_group, delay);
	scc_mgr_load_dqs_io ();

	scc_mgr_set_oct_out1_delay(write_group, delay);
	scc_mgr_load_dqs_for_write_group (write_group);
}

/* apply a delay to the entire output side: DQ, DM, DQS, OCT */

static void scc_mgr_apply_group_all_out_delay (uint32_t write_group,
	uint32_t group_bgn, uint32_t delay)
{
	/* dq shift */

	scc_mgr_apply_group_dq_out1_delay (write_group, group_bgn, delay);

	/* dm shift */

	scc_mgr_apply_group_dm_out1_delay (write_group, delay);

	/* dqs and oct shift */

	scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, delay);
}

/*
 * USER apply a delay to the entire output side (DQ, DM, DQS, OCT)
 * and to all ranks
 */
static void scc_mgr_apply_group_all_out_delay_all_ranks (uint32_t write_group,
	uint32_t group_bgn, uint32_t delay)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS;
		r += NUM_RANKS_PER_SHADOW_REG) {

		scc_mgr_apply_group_all_out_delay (write_group, group_bgn, delay);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

/* apply a delay to the entire output side: DQ, DM, DQS, OCT */

static void scc_mgr_apply_group_all_out_delay_add (uint32_t write_group,
	uint32_t group_bgn, uint32_t delay)
{
	uint32_t i, p, new_delay;

	/* dq shift */

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {

		new_delay = READ_SCC_DQ_OUT2_DELAY(i);
		new_delay += delay;

		if (new_delay > IO_IO_OUT2_DELAY_MAX) {
			pr_debug("%s(%u, %u, %u) DQ[%u,%u]: %u >"
				" %u => %u\n", __func__, write_group,
				group_bgn, delay, i, p,
				new_delay,
				IO_IO_OUT2_DELAY_MAX,
				IO_IO_OUT2_DELAY_MAX);
			new_delay = IO_IO_OUT2_DELAY_MAX;
		}

		scc_mgr_set_dq_out2_delay(write_group, i, new_delay);
		scc_mgr_load_dq (i);
	}

	/* dm shift */

	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
		new_delay = READ_SCC_DM_IO_OUT2_DELAY(i);
		new_delay += delay;

		if (new_delay > IO_IO_OUT2_DELAY_MAX) {
			pr_debug("%s(%u, %u, %u) DM[%u]: %u > %u => %u\n",
				__func__, write_group, group_bgn, delay, i,
				new_delay,
				IO_IO_OUT2_DELAY_MAX,
				IO_IO_OUT2_DELAY_MAX);
			new_delay = IO_IO_OUT2_DELAY_MAX;
		}

		scc_mgr_set_dm_out2_delay(write_group, i, new_delay);
		scc_mgr_load_dm (i);
	}

	/* dqs shift */

	new_delay = READ_SCC_DQS_IO_OUT2_DELAY();
	new_delay += delay;

	if (new_delay > IO_IO_OUT2_DELAY_MAX) {
		pr_debug("%s(%u, %u, %u) DQS: %u > %d => %d;"
			" adding %u to OUT1\n",
			__func__, write_group, group_bgn, delay,
			new_delay, IO_IO_OUT2_DELAY_MAX, IO_IO_OUT2_DELAY_MAX,
			new_delay - IO_IO_OUT2_DELAY_MAX);
		scc_mgr_set_dqs_out1_delay(write_group, new_delay -
			IO_IO_OUT2_DELAY_MAX);
		new_delay = IO_IO_OUT2_DELAY_MAX;
	}

	scc_mgr_set_dqs_out2_delay(write_group, new_delay);
	scc_mgr_load_dqs_io ();

	/* oct shift */

	new_delay = READ_SCC_OCT_OUT2_DELAY(write_group);
	new_delay += delay;

	if (new_delay > IO_IO_OUT2_DELAY_MAX) {
		pr_debug("%s(%u, %u, %u) DQS: %u > %d => %d;"
			" adding %u to OUT1\n",
			__func__, write_group, group_bgn, delay,
			new_delay, IO_IO_OUT2_DELAY_MAX, IO_IO_OUT2_DELAY_MAX,
			new_delay - IO_IO_OUT2_DELAY_MAX);
		scc_mgr_set_oct_out1_delay(write_group, new_delay -
			IO_IO_OUT2_DELAY_MAX);
		new_delay = IO_IO_OUT2_DELAY_MAX;
	}

	scc_mgr_set_oct_out2_delay(write_group, new_delay);
	scc_mgr_load_dqs_for_write_group(write_group);
}

/*
 * USER apply a delay to the entire output side (DQ, DM, DQS, OCT)
 * and to all ranks
 */
static void scc_mgr_apply_group_all_out_delay_add_all_ranks (uint32_t write_group,
	uint32_t group_bgn, uint32_t delay)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		scc_mgr_apply_group_all_out_delay_add (write_group,
			group_bgn, delay);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

static void scc_mgr_spread_out2_delay_all_ranks (uint32_t write_group,
	uint32_t test_bgn)
{
#if STRATIXV || ARRIAVGZ
	uint32_t found;
	uint32_t i;
	uint32_t p;
	uint32_t d;
	uint32_t r;

	const uint32_t delay_step = IO_IO_OUT2_DELAY_MAX /
		(RW_MGR_MEM_DQ_PER_WRITE_DQS-1);
		/* we start at zero, so have one less dq to devide among */

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS;
		r += NUM_RANKS_PER_SHADOW_REG) {
		for (i = 0, p = test_bgn, d = 0;
			i < RW_MGR_MEM_DQ_PER_WRITE_DQS;
			i++, p++, d += delay_step) {
			pr_debug("rw_mgr_mem_calibrate_vfifo_find"
				"_dqs_en_phase_sweep_dq_in_delay: g=%u r=%u,"
				" i=%u p=%u d=%u\n",
				write_group, r, i, p, d);
			scc_mgr_set_dq_out2_delay(write_group, i, d);
			scc_mgr_load_dq (i);
		}
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
#endif
}

#if DDR3
/* optimization used to recover some slots in ddr3 inst_rom */
/* could be applied to other protocols if we wanted to */
static void set_jump_as_return(void)
{
	/*
	 * to save space, we replace return with jump to special shared
	 * RETURN instruction so we set the counter to large value so that
	 * we always jump
	 */
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0xFF);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_RETURN);

}
#endif

/*
 * should always use constants as argument to ensure all computations are
 * performed at compile time
 */
static void delay_for_n_mem_clocks(const uint32_t clocks)
{
	uint32_t afi_clocks;
	uint8_t inner;
	uint8_t outer;
	uint16_t c_loop;

	afi_clocks = (clocks + AFI_RATE_RATIO-1) / AFI_RATE_RATIO;
	/* scale (rounding up) to get afi clocks */

	/*
	 * Note, we don't bother accounting for being off a little bit
	 * because of a few extra instructions in outer loops
	 * Note, the loops have a test at the end, and do the test before
	 * the decrement, and so always perform the loop
	 * 1 time more than the counter value
	 */
	if (afi_clocks == 0) {
		inner = outer = c_loop = 0;
	} else if (afi_clocks <= 0x100) {
		inner = afi_clocks-1;
		outer = 0;
		c_loop = 0;
	} else if (afi_clocks <= 0x10000) {
		inner = 0xff;
		outer = (afi_clocks-1) >> 8;
		c_loop = 0;
	} else {
		inner = 0xff;
		outer = 0xff;
		c_loop = (afi_clocks-1) >> 16;
	}

	/*
	 * rom instructions are structured as follows:
	 *
	 *    IDLE_LOOP2: jnz cntr0, TARGET_A
	 *    IDLE_LOOP1: jnz cntr1, TARGET_B
	 *                return
	 *
	 * so, when doing nested loops, TARGET_A is set to IDLE_LOOP2, and
	 * TARGET_B is set to IDLE_LOOP2 as well
	 *
	 * if we have no outer loop, though, then we can use IDLE_LOOP1 only,
	 * and set TARGET_B to IDLE_LOOP1 and we skip IDLE_LOOP2 entirely
	 *
	 * a little confusing, but it helps save precious space in the inst_rom
	 * and sequencer rom and keeps the delays more accurate and reduces
	 * overhead
	 */
	if (afi_clocks <= 0x100) {
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0,
			SKIP_DELAY_LOOP_VALUE_OR_ZERO(inner));
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_IDLE_LOOP1);
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_IDLE_LOOP1);
	} else {
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0,
			SKIP_DELAY_LOOP_VALUE_OR_ZERO(inner));
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0,
			SKIP_DELAY_LOOP_VALUE_OR_ZERO(outer));

		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_IDLE_LOOP2);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_IDLE_LOOP2);

		/* hack to get around compiler not being smart enough */
		if (afi_clocks <= 0x10000) {
			/* only need to run once */
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_IDLE_LOOP2);
		} else {
			do {
				IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
					__RW_MGR_IDLE_LOOP2);
			} while (c_loop-- != 0);
		}
	}
}

/* Special routine to recover memory device from illegal state after */
/* ck/dqs relationship is violated. */
static void recover_mem_device_after_ck_dqs_violation(void)
{
	/* Current protocol doesn't require any special recovery */
}

static void rw_mgr_rdimm_initialize(void) { }

#if DDR3


static void rw_mgr_mem_initialize (void)
{
	uint32_t r;


	/* The reset / cke part of initialization is broadcasted to all ranks */
	IOWR_32DIRECT(RW_MGR_SET_CS_AND_ODT_MASK, 0, RW_MGR_RANK_ALL);

	/*
	 * Here's how you load register for a loop
	 * Counters are located @ 0x800
	 * Jump address are located @ 0xC00
	 * For both, registers 0 to 3 are selected using bits 3 and 2, like
	 * in 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	 * I know this ain't pretty, but Avalon bus throws away the 2 least
	 * significant bits
	 */

	/* start with memory RESET activated */

	/* tINIT = 200us */

	/*
	 * 200us @ 266MHz (3.75 ns) ~ 54000 clock cycles
	 * If a and b are the number of iteration in 2 nested loops
	 * it takes the following number of cycles to complete the operation:
	 * number_of_cycles = ((2 + n) * a + 2) * b
	 * where n is the number of instruction in the inner loop
	 * One possible solution is n = 0 , a = 256 , b = 106 => a = FF,
	 * b = 6A
	 */

	/* Load counters */
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0,
		SKIP_DELAY_LOOP_VALUE_OR_ZERO(0xFF));
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0,
		SKIP_DELAY_LOOP_VALUE_OR_ZERO(0x6A));

	/* Load jump address */
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0,
		__RW_MGR_INIT_RESET_0_CKE_0);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0,
		__RW_MGR_INIT_RESET_0_CKE_0_inloop);

	/* Execute count instruction */
	/* IOWR_32DIRECT(BASE_RW_MGR, 0, __RW_MGR_COUNT_REG_0); */
	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_RESET_0_CKE_0);

	/* indicate that memory is stable */
	IOWR_32DIRECT(PHY_MGR_RESET_MEM_STBL, 0, 1);

	/* transition the RESET to high */
	/* Wait for 500us */

	/*
	 * 500us @ 266MHz (3.75 ns) ~ 134000 clock cycles
	 * If a and b are the number of iteration in 2 nested loops
	 * it takes the following number of cycles to complete the operation
	 * number_of_cycles = ((2 + n) * a + 2) * b
	 * where n is the number of instruction in the inner loop
	 * One possible solution is n = 2 , a = 131 , b = 256 => a = 83,
	 * b = FF
	 */

	/* Load counters */
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0,
		SKIP_DELAY_LOOP_VALUE_OR_ZERO(0x83));
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0,
		SKIP_DELAY_LOOP_VALUE_OR_ZERO(0xFF));

	/* Load jump address */
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_RESET_1_CKE_0);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0,
		__RW_MGR_INIT_RESET_1_CKE_0_inloop_1);

	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_RESET_1_CKE_0);

	/* bring up clock enable */

	/* tXRP < 250 ck cycles */
	delay_for_n_mem_clocks(250);

	/*
	 * USER initialize RDIMM buffer so MRS and RZQ Calibrate commands will
	 * USER be propagated to discrete memory devices
	 */
	rw_mgr_rdimm_initialize();


	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		/*
		 * USER Use Mirror-ed commands for odd ranks if address
		 * mirrorring is on
		 */
		if ((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS2_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS3_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS1_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS0_DLL_RESET_MIRR);
		} else {
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS2);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS3);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS1);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS0_DLL_RESET);
		}

		set_jump_as_return();
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_ZQCL);

		/* tZQinit = tDLLK = 512 ck cycles */
		delay_for_n_mem_clocks(512);
	}
}
#endif /* DDR3 */

#if DDR2
static void rw_mgr_mem_initialize (void)
{
	uint32_t r;

	/* *** NOTE *** */
	/* The following STAGE (n) notation refers to the corresponding
	stage in the Micron datasheet */

	/*
	 *Here's how you load register for a loop
	 * Counters are located @ 0x800
	 * Jump address are located @ 0xC00
	 * For both, registers 0 to 3 are selected using bits 3 and 2,
	 like in
	 * 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	 * I know this ain't pretty, but Avalon bus throws away the 2 least
	 significant bits
	 */

	/* *** STAGE (1, 2, 3) *** */

	/* start with CKE low */

	/* tINIT = 200us */

	/* 200us @ 300MHz (3.33 ns) ~ 60000 clock cycles
	* If a and b are the number of iteration in 2 nested loops
	* it takes the following number of cycles to complete the operation:
	* number_of_cycles = ((2 + n) * b + 2) * a
	* where n is the number of instruction in the inner loop
	* One possible solution is n = 0 , a = 256 , b = 118 => a = FF,
	* b = 76
	*/

	/*TODO: Need to manage multi-rank */

	/* Load counters */
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(0xFF));
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(0x76));

	/* Load jump address */
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_CKE_0);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_INIT_CKE_0_inloop);

	/* Execute count instruction */
	/* IOWR_32DIRECT(BASE_RW_MGR, 0, __RW_MGR_COUNT_REG_0); */
	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_CKE_0);

	/* indicate that memory is stable */
	IOWR_32DIRECT(PHY_MGR_RESET_MEM_STBL, 0, 1);

	/* Bring up CKE */
	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_NOP);

	/* *** STAGE (4) */

	/* Wait for 400ns */
	delay_for_n_ns(400);

	/* Multi-rank section begins here */
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		/*
		 * * **** *
		 * * NOTE *
		 * * **** *
		 * The following commands must be spaced by tMRD or tRPA
		 *which are in the order
		 * of 2 to 4 full rate cycles. This is peanuts in the
		 *NIOS domain, so for now
		 * we can avoid redundant wait loops
		 */

		/* Possible FIXME BEN: for HHP, we need to add delay loops
		 * to be sure although, the sequencer write interface by itself
		 * likely has enough delay
		 */

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_PRECHARGE_ALL);

		/* *** STAGE (5) */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR2);

		/* *** STAGE (6) */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR3);

		/* *** STAGE (7) */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR);

		/* *** STAGE (8) */
		/* DLL reset */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_MR_DLL_RESET);

		/* *** STAGE (9) */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_PRECHARGE_ALL);

		/* *** STAGE (10) */

		/* Issue 2 refresh commands spaced by tREF */

		/* First REFRESH */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_REFRESH);

		/* tREF = 200ns */
		delay_for_n_ns(200);

		/* Second REFRESH */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_REFRESH);

		/* Second idle loop */
		delay_for_n_ns(200);

		/* *** STAGE (11) */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_MR_CALIB);

		/* *** STAGE (12) */
		/* OCD defaults */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_EMR_OCD_ENABLE);

		/* *** STAGE (13) */
		/* OCD exit */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR);

		/* *** STAGE (14) */

		/*
		 * The memory is now initialized. Before being able to
		 *use it, we must still
		 * wait for the DLL to lock, 200 clock cycles after it
		 *was reset @ STAGE (8).
		 * Since we cannot keep track of time in any other way,
		 *let's start counting from now
		 */
		delay_for_n_mem_clocks(200);
	}
}
#endif /* DDR2 */

#if LPDDR2
static void rw_mgr_mem_initialize (void)
{
	uint32_t r;

	/* *** NOTE *** */
	/* The following STAGE (n) notation refers to the corresponding
	stage in the Micron datasheet */

	/*
	 *Here's how you load register for a loop
	 * Counters are located @ 0x800
	 * Jump address are located @ 0xC00
	 * For both, registers 0 to 3 are selected using bits 3 and 2,
	 *like in
	 * 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	 *I know this ain't pretty, but Avalon bus throws away the 2 least
	 *significant bits
	 */


	/* *** STAGE (1, 2, 3) *** */

	/* start with CKE low */

	/* tINIT1 = 100ns */

	/*
	 * 100ns @ 300MHz (3.333 ns) ~ 30 cycles
	 * If a is the number of iteration in a loop
	 * it takes the following number of cycles to complete the operation
	 * number_of_cycles = (2 + n) * a
	 * where n is the number of instruction in the inner loop
	 * One possible solution is n = 0 , a = 15 => a = 0x10
	 */

	/* Load counter */
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(0x10));

	/* Load jump address */
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_CKE_0);

	/* Execute count instruction */
	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_CKE_0);

	/* tINIT3 = 200us */
	delay_for_n_ns(200000);

	/* indicate that memory is stable */
	IOWR_32DIRECT(PHY_MGR_RESET_MEM_STBL, 0, 1);

	/* Multi-rank section begins here */
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		/* MRW RESET */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR63_RESET);
	}

	/* tINIT5 = 10us */
	delay_for_n_ns(10000);

	/* Multi-rank section begins here */
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		/* MRW ZQC */
		/* Note: We cannot calibrate other ranks when the current rank
		is calibrating for tZQINIT */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR10_ZQC);

		/* tZQINIT = 1us */
		delay_for_n_ns(1000);

		/*
		 * * **** *
		 * * NOTE *
		 * * **** *
		 * The following commands must be spaced by tMRW which is
		 *in the order
		 * of 3 to 5 full rate cycles. This is peanuts in the NIOS
		 *domain, so for now
		 * we can avoid redundant wait loops
		 */

		/* MRW MR1 */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR1_CALIB);

		/* MRW MR2 */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR2);

		/* MRW MR3 */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR3);
	}
}
#endif /* LPDDR2 */

/*  At the end of calibration we have to program the user settings in, and
  USER  hand off the memory to the user. */

#if DDR3
static void rw_mgr_mem_handoff (void)
{
	uint32_t r;


	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		/* precharge all banks ... */

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		/* load up MR settings specified by user */

		/* Use Mirror-ed commands for odd ranks if address
		mirrorring is on */
		if ((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS2_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS3_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS1_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS0_USER_MIRR);
		} else {
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS2);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS3);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS1);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
				__RW_MGR_MRS0_USER);
		}
		/* USER  need to wait tMOD (12CK or 15ns) time before issuing
		 * other commands, but we will have plenty of NIOS cycles before
		 * actual handoff so its okay.
		 */
	}

}
#endif /* DDR3 */

#if DDR2
static void rw_mgr_mem_handoff (void)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		/* precharge all banks ... */

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_PRECHARGE_ALL);

		/* load up MR settings specified by user */

		/*
		 * FIXME BEN: for HHP, we need to add delay loops to be sure
		 * We can check this with BFM perhaps
		 * Likely enough delay in RW_MGR though
		 */

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR2);

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR3);

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR);

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR_USER);

		/*
		 * USER need to wait tMOD (12CK or 15ns) time before issuing
		 * other commands,
		 * USER but we will have plenty of NIOS cycles before actual
		 * handoff so its okay.
		 */
	}
}
#endif /* DDR2 */

#if LPDDR2
static void rw_mgr_mem_handoff (void)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		/* precharge all banks... */

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_PRECHARGE_ALL);

		/* load up MR settings specified by user */

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR1_USER);

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR2);

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR3);
	}
}
#endif /* LPDDR2 */

/*
 * performs a guaranteed read on the patterns we are going to use during a
 * read test to ensure memory works
 */
static uint32_t rw_mgr_mem_calibrate_read_test_patterns (uint32_t rank_bgn,
	uint32_t group, uint32_t num_tries, t_btfld *bit_chk, uint32_t all_ranks)
{
	uint32_t r, vg;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	uint32_t rank_end = all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS :
		(rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	*bit_chk = param->read_correct_mask;
	correct_mask_vg = param->read_correct_mask_vg;

	for (r = rank_bgn; r < rank_end; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		/* Load up a constant bursts of read commands */

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x20);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0,
			__RW_MGR_GUARANTEED_READ);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x20);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0,
			__RW_MGR_GUARANTEED_READ_CONT);

		tmp_bit_chk = 0;
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS-1; ; vg--) {
			/* reset the fifos to get pointers to known state */

			IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);
			IOWR_32DIRECT(RW_MGR_RESET_READ_DATAPATH, 0, 0);

			tmp_bit_chk = tmp_bit_chk << (RW_MGR_MEM_DQ_PER_READ_DQS
				/ RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS);

			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP,
				((group*RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS +
					vg) << 2), __RW_MGR_GUARANTEED_READ);
			tmp_bit_chk = tmp_bit_chk | (correct_mask_vg &
				~(IORD_32DIRECT(BASE_RW_MGR, 0)));

			if (vg == 0)
				break;
		}
		*bit_chk &= tmp_bit_chk;
	}

	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, (group << 2),
		__RW_MGR_CLEAR_DQS_ENABLE);

	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
	pr_debug("test_load_patterns(%u,ALL) => (%u == %u) => %u\n",
		group, *bit_chk, param->read_correct_mask,
		(*bit_chk == param->read_correct_mask));
	return (*bit_chk == param->read_correct_mask);
}

static uint32_t rw_mgr_mem_calibrate_read_test_patterns_all_ranks
	(uint32_t group, uint32_t num_tries, t_btfld *bit_chk)
{
	return rw_mgr_mem_calibrate_read_test_patterns (0, group,
		num_tries, bit_chk, 1);
}

/* load up the patterns we are going to use during a read test */
static void rw_mgr_mem_calibrate_read_load_patterns (uint32_t rank_bgn,
	uint32_t all_ranks)
{
	uint32_t r;
	uint32_t rank_end = all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS :
		(rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	for (r = rank_bgn; r < rank_end; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		/* Load up a constant bursts */

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x20);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0,
			__RW_MGR_GUARANTEED_WRITE_WAIT0);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x20);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0,
			__RW_MGR_GUARANTEED_WRITE_WAIT1);

#if QUARTER_RATE
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0x01);
#endif
#if HALF_RATE
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0x02);
#endif
#if FULL_RATE
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0x04);
#endif
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0,
			__RW_MGR_GUARANTEED_WRITE_WAIT2);

#if QUARTER_RATE
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_3, 0, 0x01);
#endif
#if HALF_RATE
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_3, 0, 0x02);
#endif
#if FULL_RATE
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_3, 0, 0x04);
#endif
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0,
			__RW_MGR_GUARANTEED_WRITE_WAIT3);

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_GUARANTEED_WRITE);
	}

	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
}

static void rw_mgr_mem_calibrate_read_load_patterns_all_ranks (void)
{
	rw_mgr_mem_calibrate_read_load_patterns (0, 1);
}

/*
 * try a read and see if it returns correct data back. has dummy reads
 * inserted into the mix used to align dqs enable. has more thorough checks
 * than the regular read test.
 */

static uint32_t rw_mgr_mem_calibrate_read_test (uint32_t rank_bgn, uint32_t group,
	uint32_t num_tries, uint32_t all_correct, t_btfld *bit_chk,
	uint32_t all_groups, uint32_t all_ranks)
{
	uint32_t r, vg;
	uint32_t quick_read_mode;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	uint32_t rank_end = all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS :
		(rank_bgn + NUM_RANKS_PER_SHADOW_REG);


	*bit_chk = param->read_correct_mask;
	correct_mask_vg = param->read_correct_mask_vg;

	quick_read_mode = (((STATIC_CALIB_STEPS) &
		CALIB_SKIP_DELAY_SWEEPS) && ENABLE_SUPER_QUICK_CALIBRATION) ||
		BFM_MODE;

	for (r = rank_bgn; r < rank_end; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x10);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0,
			__RW_MGR_READ_B2B_WAIT1);
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0x10);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0,
			__RW_MGR_READ_B2B_WAIT2);

		if (quick_read_mode) {
			IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x1);
			/* need at least two (1+1) reads to capture failures */
		} else if (all_groups) {
			IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x06);
		} else {
			IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x32);
		}
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_READ_B2B);
		if (all_groups) {
			IOWR_32DIRECT(RW_MGR_LOAD_CNTR_3, 0,
				RW_MGR_MEM_IF_READ_DQS_WIDTH *
				RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS - 1);
		} else {
			IOWR_32DIRECT(RW_MGR_LOAD_CNTR_3, 0, 0x0);
		}
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_READ_B2B);

		tmp_bit_chk = 0;
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS-1; ; vg--) {
			/* reset the fifos to get pointers to known state */

			IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);
			IOWR_32DIRECT(RW_MGR_RESET_READ_DATAPATH, 0, 0);

			tmp_bit_chk = tmp_bit_chk << (RW_MGR_MEM_DQ_PER_READ_DQS
				/ RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS);

			IOWR_32DIRECT(all_groups ? RW_MGR_RUN_ALL_GROUPS :
				RW_MGR_RUN_SINGLE_GROUP, ((group *
				RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS+vg)
				<< 2), __RW_MGR_READ_B2B);
			tmp_bit_chk = tmp_bit_chk | (correct_mask_vg &
				~(IORD_32DIRECT(BASE_RW_MGR, 0)));

			if (vg == 0) {
				break;
			}
		}
		*bit_chk &= tmp_bit_chk;
	}

	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, (group << 2),
		__RW_MGR_CLEAR_DQS_ENABLE);

	if (all_correct) {
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		pr_debug("read_test(%u,ALL,%u) => (%u == %u) => %u\n",
			group, all_groups, *bit_chk, param->read_correct_mask,
			(*bit_chk ==
			param->read_correct_mask));
		return (*bit_chk == param->read_correct_mask);
	} else	{
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		pr_debug("read_test(%u,ONE,%u) => (%u != %u) => %u\n",
			group, all_groups, *bit_chk, 0,
			(*bit_chk != 0x00));
		return (*bit_chk != 0x00);
	}
}

static uint32_t rw_mgr_mem_calibrate_read_test_all_ranks (uint32_t group,
	uint32_t num_tries, uint32_t all_correct, t_btfld *bit_chk,
	uint32_t all_groups)
{
	return rw_mgr_mem_calibrate_read_test (0, group, num_tries, all_correct,
		bit_chk, all_groups, 1);
}

static void rw_mgr_incr_vfifo(uint32_t grp, uint32_t *v)
{
	/* fiddle with FIFO */
	if (HARD_PHY) {
		IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_HARD_PHY, 0, grp);
	} else if (QUARTER_RATE_MODE && !HARD_VFIFO) {
		if ((*v & 3) == 3) {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_QR, 0, grp);
		} else if ((*v & 2) == 2) {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_FR_HR, 0, grp);
		} else if ((*v & 1) == 1) {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_HR, 0, grp);
		} else {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_FR, 0, grp);
		}
	} else if (HARD_VFIFO) {
		/* Arria V & Cyclone V have a hard full-rate VFIFO that only
		has a single incr signal */
		IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_FR, 0, grp);
	} else {
		if (!HALF_RATE_MODE || (*v & 1) == 1) {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_HR, 0, grp);
		} else {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_FR, 0, grp);
		}
	}

	(*v)++;
}

static void rw_mgr_decr_vfifo(uint32_t grp, uint32_t *v)
{

	uint32_t i;

	for (i = 0; i < VFIFO_SIZE-1; i++) {
		rw_mgr_incr_vfifo(grp, v);
	}
}

/* find a good dqs enable to use */
static uint32_t rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase (uint32_t grp)
{
	uint32_t i, d, v, p;
	uint32_t max_working_cnt;
	uint32_t fail_cnt;
	t_btfld bit_chk;
	uint32_t dtaps_per_ptap;
	uint32_t found_begin, found_end;
	uint32_t work_bgn, work_mid, work_end, tmp_delay;
	uint32_t test_status;
	uint32_t found_passing_read, found_failing_read, initial_failing_dtap;

	ALTERA_ASSERT(grp < RW_MGR_MEM_IF_READ_DQS_WIDTH);

	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);

	scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);
	scc_mgr_set_dqs_en_phase_all_ranks(grp, 0);

	fail_cnt = 0;

	/* ************************************************************** */
	/* * Step 0 : Determine number of delay taps for each phase tap * */

	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}
	dtaps_per_ptap--;
	ALTERA_ASSERT(dtaps_per_ptap <= IO_DQS_EN_DELAY_MAX);
	tmp_delay = 0;

	/* ********************************************************* */
	/* * Step 1 : First push vfifo until we get a failing read * */
	for (v = 0; v < VFIFO_SIZE; ) {
		pr_debug("find_dqs_en_phase: vfifo %u\n", vfifo_idx);
		test_status = rw_mgr_mem_calibrate_read_test_all_ranks
			(grp, 1, PASS_ONE_BIT, &bit_chk, 0);
		if (!test_status) {
			fail_cnt++;

			if (fail_cnt == 2)
				break;
		}

		/* fiddle with FIFO */
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (v >= VFIFO_SIZE) {
		/* no failing read found!! Something must have gone wrong */
		pr_debug("find_dqs_en_phase: vfifo failed\n");
		return 0;
	}

	max_working_cnt = 0;

	/* ******************************************************** */
	/* * step 2: find first working phase, increment in ptaps * */
	found_begin = 0;
	work_bgn = 0;
	for (d = 0; d <= dtaps_per_ptap; d++, tmp_delay +=
		IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {
		work_bgn = tmp_delay;
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

		for (i = 0; i < VFIFO_SIZE; i++) {
			for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++, work_bgn +=
				IO_DELAY_PER_OPA_TAP) {
				pr_debug("find_dqs_en_phase: begin: vfifo=%u"
					" ptap=%u dtap=%u\n", vfifo_idx, p, d);
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				test_status =
				rw_mgr_mem_calibrate_read_test_all_ranks
				(grp, 1, PASS_ONE_BIT, &bit_chk, 0);

				if (test_status) {
					max_working_cnt = 1;
					found_begin = 1;
					break;
				}
			}

			if (found_begin)
				break;

			if (p > IO_DQS_EN_PHASE_MAX) {
				/* fiddle with FIFO */
				rw_mgr_incr_vfifo(grp, &v);
			}
		}

		if (found_begin)
			break;
	}

	if (i >= VFIFO_SIZE) {
		/* cannot find working solution */
		pr_debug("find_dqs_en_phase: no vfifo/ptap/dtap\n");
		return 0;
	}

	work_end = work_bgn;

	/*  If d is 0 then the working window covers a phase tap and
	we can follow the old procedure otherwise, we've found the beginning,
	and we need to increment the dtaps until we find the end */
	if (d == 0) {
		/* ********************************************************* */
		/* * step 3a: if we have room, back off by one and
		increment in dtaps * */

		/* Special case code for backing up a phase */
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX ;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}
		tmp_delay = work_bgn - IO_DELAY_PER_OPA_TAP;
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

		found_begin = 0;
		for (d = 0; d <= IO_DQS_EN_DELAY_MAX && tmp_delay < work_bgn;
			d++, tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {

			pr_debug("find_dqs_en_phase: begin-2: vfifo=%u "
				"ptap=%u dtap=%u\n", vfifo_idx, p, d);

			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1,
				PASS_ONE_BIT, &bit_chk, 0)) {
				found_begin = 1;
				work_bgn = tmp_delay;
				break;
			}
		}

		/* We have found a working dtap before the ptap found above */
		if (found_begin == 1) {
			max_working_cnt++;
		}

		/* Restore VFIFO to old state before we decremented it
		(if needed) */
		p = p + 1;
		if (p > IO_DQS_EN_PHASE_MAX) {
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}

		scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);

		/* ********************************************************* */
		/* * step 4a: go forward from working phase to non working
		phase, increment in ptaps * */
		p = p + 1;
		work_end += IO_DELAY_PER_OPA_TAP;
		if (p > IO_DQS_EN_PHASE_MAX) {
			/* fiddle with FIFO */
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}

		found_end = 0;
		for (; i < VFIFO_SIZE + 1; i++) {
			for (; p <= IO_DQS_EN_PHASE_MAX; p++, work_end
				+= IO_DELAY_PER_OPA_TAP) {
				pr_debug("find_dqs_en_phase: end: vfifo=%u "
					"ptap=%u dtap=%u\n", vfifo_idx, p, 0);
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				if (!rw_mgr_mem_calibrate_read_test_all_ranks
					(grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
					found_end = 1;
					break;
				}

				max_working_cnt++;
			}

			if (found_end)
				break;

			if (p > IO_DQS_EN_PHASE_MAX) {
				/* fiddle with FIFO */
				rw_mgr_incr_vfifo(grp, &v);
				p = 0;
			}
		}

		if (i >= VFIFO_SIZE + 1) {
			/* cannot see edge of failing read */
			pr_debug("find_dqs_en_phase: end: failed\n");
			return 0;
		}

		/* ********************************************************* */
		/* * step 5a:  back off one from last, increment in dtaps  * */

		/* Special case code for backing up a phase */
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}

		work_end -= IO_DELAY_PER_OPA_TAP;
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

		/* * The actual increment of dtaps is done outside of
		the if/else loop to share code */
		d = 0;

		pr_debug("find_dqs_en_phase: found end v/p: vfifo=%u ptap=%u\n",
				vfifo_idx, p);
	} else {

		/* ******************************************************* */
		/* * step 3-5b:  Find the right edge of the window using
		delay taps   * */

		pr_debug("find_dqs_en_phase: begin found: vfifo=%u ptap=%u "
			"dtap=%u begin=%u\n", vfifo_idx, p, d,
			work_bgn);

		work_end = work_bgn;

		/* * The actual increment of dtaps is done outside of the
		if/else loop to share code */

		/* Only here to counterbalance a subtract later on which is
		not needed if this branch of the algorithm is taken */
		max_working_cnt++;
	}

	/* The dtap increment to find the failing edge is done here */
	for (; d <= IO_DQS_EN_DELAY_MAX; d++, work_end +=
		IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {

			pr_debug("find_dqs_en_phase: end-2: dtap=%u\n", d);
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1,
				PASS_ONE_BIT, &bit_chk, 0)) {
				break;
			}
		}

	/* Go back to working dtap */
	if (d != 0) {
		work_end -= IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}

	pr_debug("find_dqs_en_phase: found end v/p/d: vfifo=%u ptap=%u "
		"dtap=%u end=%u\n", vfifo_idx, p, d-1, work_end);

	if (work_end >= work_bgn) {
		/* we have a working range */
	} else {
		/* nil range */
		pr_debug("find_dqs_en_phase: end-2: failed\n");
		return 0;
	}

	pr_debug("find_dqs_en_phase: found range [%u,%u]\n",
		work_bgn, work_end);

#if USE_DQS_TRACKING
	/* *************************************************************** */
	/*
	 * * We need to calculate the number of dtaps that equal a ptap
	 * * To do that we'll back up a ptap and re-find the edge of the
	 * * window using dtaps
	 */

	pr_debug("find_dqs_en_phase: calculate dtaps_per_ptap for tracking\n");

	/* Special case code for backing up a phase */
	if (p == 0) {
		p = IO_DQS_EN_PHASE_MAX;
		rw_mgr_decr_vfifo(grp, &v);
		pr_debug("find_dqs_en_phase: backed up cycle/phase: "
			"v=%u p=%u\n", vfifo_idx, p);
	} else {
		p = p - 1;
		pr_debug("find_dqs_en_phase: backed up phase only: v=%u "
			"p=%u\n", vfifo_idx, p);
	}

	scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

	/*
	 * Increase dtap until we first see a passing read (in case the
	 * window is smaller than a ptap),
	 * and then a failing read to mark the edge of the window again
	 */

	/* Find a passing read */
	pr_debug("find_dqs_en_phase: find passing read\n");
	found_passing_read = 0;
	found_failing_read = 0;
	initial_failing_dtap = d;
	for (; d <= IO_DQS_EN_DELAY_MAX; d++) {
		pr_debug("find_dqs_en_phase: testing read d=%u\n", d);
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

		if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1,
			PASS_ONE_BIT, &bit_chk, 0)) {
			found_passing_read = 1;
			break;
		}
	}

	if (found_passing_read) {
		/* Find a failing read */
		pr_debug("find_dqs_en_phase: find failing read\n");
		for (d = d + 1; d <= IO_DQS_EN_DELAY_MAX; d++) {
			pr_debug("find_dqs_en_phase: testing read d=%u\n", d);
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks
				(grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				found_failing_read = 1;
				break;
			}
		}
	} else {
		pr_debug("find_dqs_en_phase: failed to calculate dtaps "
			"per ptap. Fall back on static value\n");
	}

	/*
	 * The dynamically calculated dtaps_per_ptap is only valid if we
	 * found a passing/failing read. If we didn't, it means d hit the max
	 * (IO_DQS_EN_DELAY_MAX). Otherwise, dtaps_per_ptap retains its
	 * statically calculated value.
	 */
	if (found_passing_read && found_failing_read) {
		dtaps_per_ptap = d - initial_failing_dtap;
	}

	ALTERA_ASSERT(dtaps_per_ptap <= IO_DQS_EN_DELAY_MAX);
	IOWR_32DIRECT(REG_FILE_DTAPS_PER_PTAP, 0, dtaps_per_ptap);

	pr_debug("find_dqs_en_phase: dtaps_per_ptap=%u - %u = %u\n", d,
		initial_failing_dtap, dtaps_per_ptap);
#endif

	/* ******************************************** */
	/* * step 6:  Find the centre of the window   * */

	work_mid = (work_bgn + work_end) / 2;
	tmp_delay = 0;

	pr_debug("work_bgn=%d work_end=%d work_mid=%d\n", work_bgn,
		work_end, work_mid);
	/* Get the middle delay to be less than a VFIFO delay */
	for (p = 0; p <= IO_DQS_EN_PHASE_MAX;
		p++, tmp_delay += IO_DELAY_PER_OPA_TAP)
		;
	pr_debug("vfifo ptap delay %d\n", tmp_delay);
	while (work_mid > tmp_delay)
		work_mid -= tmp_delay;
	pr_debug("new work_mid %d\n", work_mid);
	tmp_delay = 0;
	for (p = 0; p <= IO_DQS_EN_PHASE_MAX && tmp_delay < work_mid;
		p++, tmp_delay += IO_DELAY_PER_OPA_TAP)
		;
	tmp_delay -= IO_DELAY_PER_OPA_TAP;
	pr_debug("new p %d, tmp_delay=%d\n", p-1, tmp_delay);
	for (d = 0; d <= IO_DQS_EN_DELAY_MAX && tmp_delay < work_mid; d++,
		tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP)
		;
	pr_debug("new d %d, tmp_delay=%d\n", d, tmp_delay);

	scc_mgr_set_dqs_en_phase_all_ranks(grp, p-1);
	scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

	/* push vfifo until we can successfully calibrate. We can do this
	because the largest possible margin in 1 VFIFO cycle */

	for (i = 0; i < VFIFO_SIZE; i++) {
		pr_debug("find_dqs_en_phase: center: vfifo=%u\n", vfifo_idx);
		if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1,
			PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		}

		/* fiddle with FIFO */
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		pr_debug("find_dqs_en_phase: center: failed\n");
		return 0;
	}
	pr_debug("find_dqs_en_phase: center found: vfifo=%u ptap=%u "
		"dtap=%u\n", vfifo_idx, p-1, d);
	return 1;
}

/* Try rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase across different
dq_in_delay values */
static uint32_t
rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay(uint32_t write_group, uint32_t read_group, uint32_t test_bgn)
{
#if STRATIXV || ARRIAV || CYCLONEV || ARRIAVGZ
	uint32_t found;
	uint32_t i;
	uint32_t p;
	uint32_t d;
	uint32_t r;
	const uint32_t delay_step = IO_IO_IN_DELAY_MAX / (RW_MGR_MEM_DQ_PER_READ_DQS - 1);
	/* we start at zero, so have one less dq to devide among */

	/* try different dq_in_delays since the dq path is shorter than dqs */

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		for (i = 0, p = test_bgn, d = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS;
			i++, p++, d += delay_step) {
			pr_debug("rw_mgr_mem_calibrate_vfifo_find_dqs_"
				"en_phase_sweep_dq_in_delay: g=%u/%u "
				"r=%u, i=%u p=%u d=%u\n",
				write_group, read_group, r, i, p, d);
			scc_mgr_set_dq_in_delay(write_group, p, d);
			scc_mgr_load_dq (p);
		}
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}

	found = rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase(read_group);

	pr_debug("rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq"
		"_in_delay: g=%u/%u found=%u; Reseting delay chain to zero\n",
		write_group, read_group, found);

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS;
		r += NUM_RANKS_PER_SHADOW_REG) {
		for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS;
			i++, p++) {
			scc_mgr_set_dq_in_delay(write_group, p, 0);
			scc_mgr_load_dq (p);
		}
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}

	return found;
#else
	return rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase(read_group);
#endif
}

/* per-bit deskew DQ and center */
static uint32_t rw_mgr_mem_calibrate_vfifo_center (uint32_t rank_bgn,
	uint32_t write_group, uint32_t read_group, uint32_t test_bgn,
	uint32_t use_read_test, uint32_t update_fom)
{
	uint32_t i, p, d, min_index;
	/* Store these as signed since there are comparisons with
	signed numbers */
	t_btfld bit_chk;
	t_btfld sticky_bit_chk;
	int32_t left_edge[RW_MGR_MEM_DQ_PER_READ_DQS];
	int32_t right_edge[RW_MGR_MEM_DQ_PER_READ_DQS];
	int32_t final_dq[RW_MGR_MEM_DQ_PER_READ_DQS];
	int32_t mid;
	int32_t orig_mid_min, mid_min;
	int32_t new_dqs, start_dqs, start_dqs_en, shift_dq, final_dqs,
		final_dqs_en;
	int32_t dq_margin, dqs_margin;
	uint32_t stop;

	ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	start_dqs = READ_SCC_DQS_IN_DELAY(read_group);
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		start_dqs_en = READ_SCC_DQS_EN_DELAY(read_group);
	}

	/* per-bit deskew */

	/* set the left and right edge of each bit to an illegal value */
	/* use (IO_IO_IN_DELAY_MAX + 1) as an illegal value */
	sticky_bit_chk = 0;
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		left_edge[i]  = IO_IO_IN_DELAY_MAX + 1;
		right_edge[i] = IO_IO_IN_DELAY_MAX + 1;
	}

	/* Search for the left edge of the window for each bit */
	for (d = 0; d <= IO_IO_IN_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_in_delay (write_group, test_bgn, d);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		/* Stop searching when the read test doesn't pass AND when
		we've seen a passing read on every bit */
		if (use_read_test) {
			stop = !rw_mgr_mem_calibrate_read_test (rank_bgn,
				read_group, NUM_READ_PB_TESTS, PASS_ONE_BIT,
				&bit_chk, 0, 0);
		} else {
			rw_mgr_mem_calibrate_write_test (rank_bgn, write_group,
				0, PASS_ONE_BIT, &bit_chk, 0);
			bit_chk = bit_chk >> (RW_MGR_MEM_DQ_PER_READ_DQS *
				(read_group - (write_group * RW_MGR_NUM_DQS_PER_WRITE_GROUP)));
			stop = (bit_chk == 0);
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->read_correct_mask);
		pr_debug("vfifo_center(left): dtap=%u => " BTFLD_FMT " == "
			BTFLD_FMT " && %u\n", d, sticky_bit_chk,
			param->read_correct_mask, stop);

		if (stop == 1)
			break;
		for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
			if (bit_chk & 1) {
				/* Remember a passing test as the
				left_edge */
				left_edge[i] = d;
			} else {
				/* If a left edge has not been seen yet,
				then a future passing test will mark
				this edge as the right edge */
				if (left_edge[i] ==
					IO_IO_IN_DELAY_MAX + 1) {
					right_edge[i] = -(d + 1);
				}
			}
			pr_debug("vfifo_center[l,d=%u]: "
				"bit_chk_test=%d left_edge[%u]: "
				"%d right_edge[%u]: %d\n",
				d, (int)(bit_chk & 1), i, left_edge[i],
				i, right_edge[i]);
			bit_chk = bit_chk >> 1;
		}
	}

	/* Reset DQ delay chains to 0 */
	scc_mgr_apply_group_dq_in_delay (write_group, test_bgn, 0);
	sticky_bit_chk = 0;
	for (i = RW_MGR_MEM_DQ_PER_READ_DQS - 1;; i--) {

		pr_debug("vfifo_center: left_edge[%u]: %d right_edge[%u]: "
			"%d\n", i, left_edge[i], i, right_edge[i]);

		/* Check for cases where we haven't found the left edge,
		which makes our assignment of the the right edge invalid.
		Reset it to the illegal value. */
		if ((left_edge[i] == IO_IO_IN_DELAY_MAX + 1) && (
			right_edge[i] != IO_IO_IN_DELAY_MAX + 1)) {
			right_edge[i] = IO_IO_IN_DELAY_MAX + 1;
			pr_debug("vfifo_center: reset right_edge[%u]: %d\n",
				i, right_edge[i]);
		}

		/* Reset sticky bit (except for bits where we have seen
		both the left and right edge) */
		sticky_bit_chk = sticky_bit_chk << 1;
		if ((left_edge[i] != IO_IO_IN_DELAY_MAX + 1) &&
			(right_edge[i] != IO_IO_IN_DELAY_MAX + 1)) {
			sticky_bit_chk = sticky_bit_chk | 1;
		}

		if (i == 0)
			break;
	}

	/* Search for the right edge of the window for each bit */
	for (d = 0; d <= IO_DQS_IN_DELAY_MAX - start_dqs; d++) {
		scc_mgr_set_dqs_bus_in_delay(read_group, d + start_dqs);
		if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
			uint32_t delay = d + start_dqs_en;
			if (delay > IO_DQS_EN_DELAY_MAX) {
				delay = IO_DQS_EN_DELAY_MAX;
			}
			scc_mgr_set_dqs_en_delay(read_group, delay);
		}
		scc_mgr_load_dqs (read_group);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		/* Stop searching when the read test doesn't pass AND when
		we've seen a passing read on every bit */
		if (use_read_test) {
			stop = !rw_mgr_mem_calibrate_read_test (rank_bgn,
				read_group, NUM_READ_PB_TESTS, PASS_ONE_BIT,
				&bit_chk, 0, 0);
		} else {
			rw_mgr_mem_calibrate_write_test (rank_bgn, write_group,
				0, PASS_ONE_BIT, &bit_chk, 0);
			bit_chk = bit_chk >> (RW_MGR_MEM_DQ_PER_READ_DQS *
				(read_group - (write_group * RW_MGR_NUM_DQS_PER_WRITE_GROUP)));
			stop = (bit_chk == 0);
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->read_correct_mask);

		pr_debug("vfifo_center(right): dtap=%u => " BTFLD_FMT " == "
			BTFLD_FMT " && %u\n", d, sticky_bit_chk,
			param->read_correct_mask, stop);

		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
				if (bit_chk & 1) {
					/* Remember a passing test as
					the right_edge */
					right_edge[i] = d;
				} else {
					if (d != 0) {
						/* If a right edge has not been
						seen yet, then a future passing
						test will mark this edge as the
						left edge */
						if (right_edge[i] ==
						IO_IO_IN_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}
					} else {
						/* d = 0 failed, but it passed
						when testing the left edge,
						so it must be marginal,
						set it to -1 */
						if (right_edge[i] ==
							IO_IO_IN_DELAY_MAX + 1
							&& left_edge[i] !=
							IO_IO_IN_DELAY_MAX
							+ 1) {
							right_edge[i] = -1;
						}
						/* If a right edge has not been
						seen yet, then a future passing
						test will mark this edge as the
						left edge */
						else if (right_edge[i] ==
							IO_IO_IN_DELAY_MAX +
							1) {
							left_edge[i] = -(d + 1);
						}

					}
				}

				pr_debug("vfifo_center[r,d=%u]: "
					"bit_chk_test=%d left_edge[%u]: %d "
					"right_edge[%u]: %d\n",
					d, (int)(bit_chk & 1), i, left_edge[i],
					i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	/* Store all observed margins */

	/* Check that all bits have a window */
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		pr_debug("vfifo_center: left_edge[%u]: %d right_edge[%u]:"
			" %d\n", i, left_edge[i], i, right_edge[i]);
		if ((left_edge[i] == IO_IO_IN_DELAY_MAX + 1) || (right_edge[i]
			== IO_IO_IN_DELAY_MAX + 1)) {

			/* Restore delay chain settings before letting the loop
			in rw_mgr_mem_calibrate_vfifo to retry different
			dqs/ck relationships */
			scc_mgr_set_dqs_bus_in_delay(read_group, start_dqs);
			if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
				scc_mgr_set_dqs_en_delay(read_group,
					start_dqs_en);
			}
			scc_mgr_load_dqs (read_group);
			IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

			pr_debug("vfifo_center: failed to find edge [%u]: "
				"%d %d\n", i, left_edge[i], right_edge[i]);
			return 0;
		}
	}

	/* Find middle of window for each DQ bit */
	mid_min = left_edge[0] - right_edge[0];
	min_index = 0;
	for (i = 1; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		mid = left_edge[i] - right_edge[i];
		if (mid < mid_min) {
			mid_min = mid;
			min_index = i;
		}
	}

	/*  -mid_min/2 represents the amount that we need to move DQS.
	If mid_min is odd and positive we'll need to add one to
	make sure the rounding in further calculations is correct
	(always bias to the right), so just add 1 for all positive values */
	if (mid_min > 0) {
		mid_min++;
	}
	mid_min = mid_min / 2;

	pr_debug("vfifo_center: mid_min=%d (index=%u)\n", mid_min, min_index);

	/* Determine the amount we can change DQS (which is -mid_min) */
	orig_mid_min = mid_min;
	new_dqs = start_dqs;
	mid_min = 0;

	pr_debug("vfifo_center: start_dqs=%d start_dqs_en=%d "
		"new_dqs=%d mid_min=%d\n",
		start_dqs, IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS ? start_dqs_en : -1,
		new_dqs, mid_min);

	/* Initialize data for export structures */
	dqs_margin = IO_IO_IN_DELAY_MAX + 1;
	dq_margin  = IO_IO_IN_DELAY_MAX + 1;

	/* add delay to bring centre of all DQ windows to the same "level" */
	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
		/* Use values before divide by 2 to reduce round off error */
		shift_dq = (left_edge[i] - right_edge[i] -
			(left_edge[min_index] - right_edge[min_index]))/2  +
			(orig_mid_min - mid_min);

		pr_debug("vfifo_center: before: shift_dq[%u]=%d\n", i,
			shift_dq);

		if (shift_dq + (int32_t)READ_SCC_DQ_IN_DELAY(p) >
			(int32_t)IO_IO_IN_DELAY_MAX) {
			shift_dq = (int32_t)IO_IO_IN_DELAY_MAX -
				READ_SCC_DQ_IN_DELAY(i);
		} else if (shift_dq + (int32_t)READ_SCC_DQ_IN_DELAY(p) < 0) {
			shift_dq = -(int32_t)READ_SCC_DQ_IN_DELAY(p);
		}
		pr_debug("vfifo_center: after: shift_dq[%u]=%d\n", i,
			shift_dq);
		final_dq[i] = READ_SCC_DQ_IN_DELAY(p) + shift_dq;
		scc_mgr_set_dq_in_delay(write_group, p, final_dq[i]);
		scc_mgr_load_dq (p);

		pr_debug("vfifo_center: margin[%u]=[%d,%d]\n", i,
			left_edge[i] - shift_dq + (-mid_min),
			right_edge[i] + shift_dq - (-mid_min));
		/* To determine values for export structures */
		if (left_edge[i] - shift_dq + (-mid_min) < dq_margin) {
			dq_margin = left_edge[i] - shift_dq + (-mid_min);
		}
		if (right_edge[i] + shift_dq - (-mid_min) < dqs_margin) {
			dqs_margin = right_edge[i] + shift_dq - (-mid_min);
		}
	}

#if ENABLE_DQS_IN_CENTERING
	final_dqs = new_dqs;
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		final_dqs_en = start_dqs_en - mid_min;
	}
#else
	final_dqs = start_dqs;
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		final_dqs_en = start_dqs_en;
	}
#endif

	/* Move DQS-en */
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		scc_mgr_set_dqs_en_delay(read_group, final_dqs_en);
		scc_mgr_load_dqs (read_group);
	}

	/* Move DQS */
	scc_mgr_set_dqs_bus_in_delay(read_group, final_dqs);
	scc_mgr_load_dqs (read_group);

	if (update_fom) {
		/* Export values */
		gbl->fom_in += (dq_margin + dqs_margin) / RW_MGR_NUM_DQS_PER_WRITE_GROUP;
	}

	pr_debug("vfifo_center: dq_margin=%d dqs_margin=%d\n",
		dq_margin, dqs_margin);

	/* Do not remove this line as it makes sure all of our decisions
	have been applied */
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	return (dq_margin >= 0) && (dqs_margin >= 0);
}

/*
 * calibrate the read valid prediction FIFO.
 *
 *  - read valid prediction will consist of finding a good DQS enable phase,
 * DQS enable delay, DQS input phase, and DQS input delay.
 *  - we also do a per-bit deskew on the DQ lines.
 */



/* VFIFO Calibration -- Full Calibration */
static uint32_t rw_mgr_mem_calibrate_vfifo (uint32_t read_group, uint32_t test_bgn)
{
	uint32_t p, d, rank_bgn;
	uint32_t dtaps_per_ptap;
	uint32_t tmp_delay;
	t_btfld bit_chk;
	uint32_t grp_calibrated;
	uint32_t write_group, write_test_bgn;

	/* update info for sims */

	reg_file_set_stage(CAL_STAGE_VFIFO);

	write_group = read_group;
	write_test_bgn = test_bgn;

	/* USER Determine number of delay taps for each phase tap */
	dtaps_per_ptap = 0;
	tmp_delay = 0;

	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}
	dtaps_per_ptap--;
	tmp_delay = 0;

	/* update info for sims */

	reg_file_set_group(read_group);

	grp_calibrated = 0;

	reg_file_set_sub_stage(CAL_SUBSTAGE_GUARANTEED_READ);

	for (d = 0; d <= dtaps_per_ptap && grp_calibrated == 0; d += 2) {

		/* In RLDRAMX we may be messing the delay of pins in
		the same write group but outside of the current read
		group, but that's ok because we haven't calibrated the
		output side yet. */
		if (d > 0) {
			scc_mgr_apply_group_all_out_delay_add_all_ranks
			(write_group, write_test_bgn, d);
		}

		for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX && grp_calibrated == 0; p++) {
			/* set a particular dqdqs phase */
			scc_mgr_set_dqdqs_output_phase_all_ranks(
					read_group, p);

			/* Previous iteration may have failed as a result of
			ck/dqs or ck/dk violation, in which case the device may
			require special recovery. */
			if (d != 0 || p != 0)
				recover_mem_device_after_ck_dqs_violation();

			pr_debug("calibrate_vfifo: g=%u p=%u d=%u\n",
				read_group, p, d);

			/* Load up the patterns used by read calibration
			using current DQDQS phase */

			rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();

			if (!(gbl->phy_debug_mode_flags &
				PHY_DEBUG_DISABLE_GUARANTEED_READ)) {
			if (!rw_mgr_mem_calibrate_read_test_patterns_all_ranks
				(read_group, 1, &bit_chk)) {
					pr_debug("Guaranteed read test failed:"
						" g=%u p=%u d=%u\n",
						read_group, p, d);
					break;
				}
			}
/* case:56390 */
			grp_calibrated = 1;

			if (rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay(write_group, read_group, test_bgn)) {
				/* USER Read per-bit deskew can be done on a
				per shadow register basis */
				for (rank_bgn = 0;
						rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS;
						rank_bgn += NUM_RANKS_PER_SHADOW_REG) {
					/* Determine if this set of ranks
					should be skipped entirely */
					/* If doing read after write
					calibration, do not update FOM
					now - do it then */
					if (!rw_mgr_mem_calibrate_vfifo_center(rank_bgn, write_group, read_group, test_bgn, 1, 0)) {
						grp_calibrated = 0;
					}
				}
			} else {
				grp_calibrated = 0;
			}
		}
	}

	/* Reset the delay chains back to zero if they have moved > 1
	(check for > 1 because loop will increase d even when pass in
	first case) */
	if (d > 2)
		scc_mgr_zero_group(write_group, write_test_bgn, 1);

	return 1;
}

/* VFIFO Calibration -- Read Deskew Calibration after write deskew */
static uint32_t rw_mgr_mem_calibrate_vfifo_end (uint32_t read_group, uint32_t test_bgn)
{
	uint32_t rank_bgn;
	uint32_t grp_calibrated;
	uint32_t write_group;

	/* update info for sims */

	reg_file_set_stage(CAL_STAGE_VFIFO_AFTER_WRITES);
	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);

	write_group = read_group;

	/* update info for sims */
	reg_file_set_group(read_group);

	grp_calibrated = 1;

	/* Read per-bit deskew can be done on a per shadow register basis */
	for (rank_bgn = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS;
		rank_bgn += NUM_RANKS_PER_SHADOW_REG) {

		/* This is the last calibration round, update FOM here */
		if (!rw_mgr_mem_calibrate_vfifo_center (rank_bgn,
			write_group, read_group, test_bgn, 0, 1))
				grp_calibrated = 0;
	}

	if (grp_calibrated == 0)
		return 0;

	return 1;
}


/* Calibrate LFIFO to find smallest read latency */

static uint32_t rw_mgr_mem_calibrate_lfifo (void)
{
	uint32_t found_one;
	t_btfld bit_chk;

	/* update info for sims */

	reg_file_set_stage(CAL_STAGE_LFIFO);
	reg_file_set_sub_stage(CAL_SUBSTAGE_READ_LATENCY);

	/* Load up the patterns used by read calibration for all ranks */

	rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();

	found_one = 0;

	do {
		IOWR_32DIRECT(PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
		pr_debug("lfifo: read_lat=%u\n", gbl->curr_read_lat);

		if (!rw_mgr_mem_calibrate_read_test_all_ranks (0,
			NUM_READ_TESTS, PASS_ALL_BITS, &bit_chk, 1)) {
			break;
		}

		found_one = 1;

		/* reduce read latency and see if things are working */
		/* correctly */

		gbl->curr_read_lat--;
	} while (gbl->curr_read_lat > 0);

	/* reset the fifos to get pointers to known state */

	IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);

	if (found_one) {
		/* add a fudge factor to the read latency that was determined */
		gbl->curr_read_lat += 2;
		IOWR_32DIRECT(PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
		pr_debug("lfifo: success: using read_lat=%u\n",
			gbl->curr_read_lat);

		return 1;
	} else {
		pr_debug("lfifo: failed at initial read_lat=%u\n",
			gbl->curr_read_lat);

		return 0;
	}
}

/*
 * issue write test command.
 * two variants are provided. one that just tests a write pattern and
 * another that tests datamask functionality.
 */

static void rw_mgr_mem_calibrate_write_test_issue (uint32_t group, uint32_t test_dm)
{
	uint32_t mcc_instruction;
	uint32_t quick_write_mode = (((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES)
		&& ENABLE_SUPER_QUICK_CALIBRATION) || BFM_MODE;
	uint32_t rw_wl_nop_cycles;

	/*
	 * Set counter and jump addresses for the right
	 * number of NOP cycles.
	 * The number of supported NOP cycles can range from -1 to infinity
	 * Three different cases are handled:
	 *
	 * 1. For a number of NOP cycles greater than 0, the RW Mgr looping
	 *    mechanism will be used to insert the right number of NOPs
	 *
	 * 2. For a number of NOP cycles equals to 0, the micro-instruction
	 *    issuing the write command will jump straight to the
	 *    micro-instruction that turns on DQS (for DDRx), or outputs write
	 *    data (for RLD), skipping
	 *    the NOP micro-instruction all together
	 *
	 * 3. A number of NOP cycles equal to -1 indicates that DQS must be
	 *    turned on in the same micro-instruction that issues the write
	 *    command. Then we need
	 *    to directly jump to the micro-instruction that sends out the data
	 *
	 * NOTE: Implementing this mechanism uses 2 RW Mgr jump-counters
	 *       (2 and 3). One jump-counter (0) is used to perform multiple
	 *       write-read operations.
	 *       one counter left to issue this command in "multiple-group" mode
	 */

#if MULTIPLE_AFI_WLAT
	rw_wl_nop_cycles = gbl->rw_wl_nop_cycles_per_group[group];
#else
	rw_wl_nop_cycles = gbl->rw_wl_nop_cycles;
#endif

	if (rw_wl_nop_cycles == -1) {
		/* CNTR 2 - We want to execute the special write operation that
		turns on DQS right away and then skip directly to the
		instruction that sends out the data. We set the counter to a
		large number so that the jump is always taken */
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0xFF);

		/* CNTR 3 - Not used */
		if (test_dm) {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0_WL_1;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0,
				__RW_MGR_LFSR_WR_RD_DM_BANK_0_DATA);
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0,
				__RW_MGR_LFSR_WR_RD_DM_BANK_0_NOP);
		} else {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0_WL_1;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0,
				__RW_MGR_LFSR_WR_RD_BANK_0_DATA);
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0,
				__RW_MGR_LFSR_WR_RD_BANK_0_NOP);
		}
	} else if (rw_wl_nop_cycles == 0) {
		/* CNTR 2 - We want to skip the NOP operation and go straight to
		the DQS enable instruction. We set the counter to a large number
		so that the jump is always taken */
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0xFF);

		/* CNTR 3 - Not used */
		if (test_dm) {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0,
				__RW_MGR_LFSR_WR_RD_DM_BANK_0_DQS);
		} else {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0,
				__RW_MGR_LFSR_WR_RD_BANK_0_DQS);
		}
	} else {
		/* CNTR 2 - In this case we want to execute the next instruction
		and NOT take the jump. So we set the counter to 0. The jump
		address doesn't count */
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0x0);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, 0x0);

		/* CNTR 3 - Set the nop counter to the number of cycles we
		need to loop for, minus 1 */
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_3, 0, rw_wl_nop_cycles - 1);
		if (test_dm) {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0,
				__RW_MGR_LFSR_WR_RD_DM_BANK_0_NOP);
		} else {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0,
				__RW_MGR_LFSR_WR_RD_BANK_0_NOP);
		}
	}

	IOWR_32DIRECT(RW_MGR_RESET_READ_DATAPATH, 0, 0);

	if (quick_write_mode) {
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x08);
	} else {
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x40);
	}
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, mcc_instruction);

	/* CNTR 1 - This is used to ensure enough time elapses
	for read data to come back. */
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x30);

	if (test_dm) {
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0,
			__RW_MGR_LFSR_WR_RD_DM_BANK_0_WAIT);
	} else {
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0,
			__RW_MGR_LFSR_WR_RD_BANK_0_WAIT);
	}

	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, (group << 2), mcc_instruction);
}

/* Test writes, can check for a single bit pass or multiple bit pass */

static uint32_t rw_mgr_mem_calibrate_write_test (uint32_t rank_bgn,
		uint32_t write_group, uint32_t use_dm, uint32_t all_correct,
		t_btfld *bit_chk, uint32_t all_ranks)
{
	uint32_t r;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	uint32_t vg;
	uint32_t rank_end = all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS :
		(rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	*bit_chk = param->write_correct_mask;
	correct_mask_vg = param->write_correct_mask_vg;

	for (r = rank_bgn; r < rank_end; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		tmp_bit_chk = 0;
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS - 1; ; vg--) {

			/* reset the fifos to get pointers to known state */
			IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);

			tmp_bit_chk = tmp_bit_chk <<
				(RW_MGR_MEM_DQ_PER_WRITE_DQS /
				RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS);
			rw_mgr_mem_calibrate_write_test_issue (write_group *
				RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS + vg,
				use_dm);

			tmp_bit_chk = tmp_bit_chk | (correct_mask_vg &
				~(IORD_32DIRECT(BASE_RW_MGR, 0)));
			pr_debug("write_test(%u,%u,%u) :[%u,%u] "
				BTFLD_FMT " & ~%x => " BTFLD_FMT " => "
				BTFLD_FMT, write_group, use_dm, all_correct,
				r, vg, correct_mask_vg,
				IORD_32DIRECT(BASE_RW_MGR, 0), correct_mask_vg
				& ~IORD_32DIRECT(BASE_RW_MGR, 0),
				tmp_bit_chk);

			if (vg == 0)
				break;
		}
		*bit_chk &= tmp_bit_chk;
	}

	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);

	if (all_correct)
		return (*bit_chk == param->write_correct_mask);
	else
		return (*bit_chk != 0x00);
}

static uint32_t rw_mgr_mem_calibrate_write_test_all_ranks
(uint32_t write_group, uint32_t use_dm, uint32_t all_correct, t_btfld *bit_chk)
{
	return rw_mgr_mem_calibrate_write_test (0, write_group,
		use_dm, all_correct, bit_chk, 1);
}

/* level the write operations */
/* Write Levelling -- Full Calibration */
static uint32_t rw_mgr_mem_calibrate_wlevel (uint32_t g, uint32_t test_bgn)
{
	uint32_t p, d;
	uint32_t num_additional_fr_cycles = 0;
	t_btfld bit_chk;
	uint32_t work_bgn, work_end, work_mid;
	uint32_t tmp_delay;
	uint32_t found_begin;
	uint32_t dtaps_per_ptap;

	/* update info for sims */

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);

	/* maximum phases for the sweep */

#if USE_DQS_TRACKING
	dtaps_per_ptap = IORD_32DIRECT(REG_FILE_DTAPS_PER_PTAP, 0);
#else
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DCHAIN_TAP;
	}
	dtaps_per_ptap--;
	tmp_delay = 0;
#endif

	/* starting phases */

	/* update info for sims */

	reg_file_set_group(g);

	/* starting and end range where writes work */

	scc_mgr_spread_out2_delay_all_ranks (g, test_bgn);

	work_bgn = 0;
	work_end = 0;

	/* step 1: find first working phase, increment in ptaps, and then in
	dtaps if ptaps doesn't find a working phase */
	found_begin = 0;
	tmp_delay = 0;
	for (d = 0; d <= dtaps_per_ptap; d++, tmp_delay +=
		IO_DELAY_PER_DCHAIN_TAP) {
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

		work_bgn = tmp_delay;

		for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX +
			num_additional_fr_cycles*IO_DLL_CHAIN_LENGTH;
			p++, work_bgn += IO_DELAY_PER_OPA_TAP) {
			pr_debug("wlevel: begin-1: p=%u d=%u\n", p, d);
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

			if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0,
				PASS_ONE_BIT, &bit_chk)) {
				found_begin = 1;
				break;
			}
		}

		if (found_begin)
			break;
	}

	if (p > IO_DQDQS_OUT_PHASE_MAX + num_additional_fr_cycles * IO_DLL_CHAIN_LENGTH)
		/* fail, cannot find first working phase */
		return 0;

	pr_debug("wlevel: first valid p=%u d=%u\n", p, d);

	reg_file_set_sub_stage(CAL_SUBSTAGE_LAST_WORKING_DELAY);

	/* If d is 0 then the working window covers a phase tap and we can
	follow the old procedure otherwise, we've found the beginning, and we
	need to increment the dtaps until we find the end */
	if (d == 0) {
		work_end = work_bgn + IO_DELAY_PER_OPA_TAP;

		/* step 2: if we have room, back off by one and increment
		in dtaps */

		if (p > 0) {
			int found = 0;
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);

			tmp_delay = work_bgn - IO_DELAY_PER_OPA_TAP;

			for (d = 0; d <= IO_IO_OUT1_DELAY_MAX &&
				tmp_delay < work_bgn; d++,
				tmp_delay += IO_DELAY_PER_DCHAIN_TAP) {
				pr_debug("wlevel: begin-2: p=%u d=%u\n",
					(p - 1), d);
				scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, d);

				if (rw_mgr_mem_calibrate_write_test_all_ranks(g, 0, PASS_ONE_BIT, &bit_chk)) {
					found = 1;
					work_bgn = tmp_delay;
					break;
				}
			}

			scc_mgr_apply_group_all_out_delay_all_ranks (g,
				test_bgn, 0);
		} else {
			pr_debug("wlevel: found begin-B: p=%u d=%u ps=%u\n",
				p, d, work_bgn);
		}

		/* step 3: go forward from working phase to non working phase,
		increment in ptaps */

		for (p = p + 1; p <= IO_DQDQS_OUT_PHASE_MAX +
			num_additional_fr_cycles * IO_DLL_CHAIN_LENGTH; p++,
			work_end += IO_DELAY_PER_OPA_TAP) {
			pr_debug("wlevel: end-0: p=%u d=%u\n", p,
				0);
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

			if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0,
				PASS_ONE_BIT, &bit_chk)) {
				break;
			}
		}

		/* step 4: back off one from last, increment in dtaps */
		/* The actual increment is done outside the if/else statement
		since it is shared with other code */

		p = p - 1;

		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		work_end -= IO_DELAY_PER_OPA_TAP;
		d = 0;

	} else {
		/* step 5: Window doesn't cover phase tap, just increment
		dtaps until failure */
		/* The actual increment is done outside the if/else statement
		since it is shared with other code */
		work_end = work_bgn;
		pr_debug("wlevel: found begin-C: p=%u d=%u ps=%u\n", p,
			d, work_bgn);
	}

	/* The actual increment until failure */
	for (; d <= IO_IO_OUT1_DELAY_MAX; d++, work_end +=
		IO_DELAY_PER_DCHAIN_TAP) {
		pr_debug("wlevel: end: p=%u d=%u\n", p, d);
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

		if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0,
			PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}
	scc_mgr_zero_group (g, test_bgn, 1);

	work_end -= IO_DELAY_PER_DCHAIN_TAP;

	if (work_end >= work_bgn) {
		/* we have a working range */
	} else {
		/* nil range */
		return 0;
	}

	pr_debug("wlevel: found end: p=%u d=%u; range: [%u,%u]\n", p,
		d-1, work_bgn, work_end);

	/* center */

	work_mid = (work_bgn + work_end) / 2;

	pr_debug("wlevel: work_mid=%d\n", work_mid);

	tmp_delay = 0;

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX  +
		num_additional_fr_cycles * IO_DLL_CHAIN_LENGTH &&
		tmp_delay < work_mid; p++, tmp_delay += IO_DELAY_PER_OPA_TAP)
		;

	if (tmp_delay > work_mid) {
		tmp_delay -= IO_DELAY_PER_OPA_TAP;
		p--;
	}

	while (p > IO_DQDQS_OUT_PHASE_MAX) {
		tmp_delay -= IO_DELAY_PER_OPA_TAP;
		p--;
	}

	scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

	pr_debug("wlevel: p=%u tmp_delay=%u left=%u\n", p, tmp_delay,
		work_mid - tmp_delay);

	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_mid; d++,
		tmp_delay += IO_DELAY_PER_DCHAIN_TAP)
		;

	if (tmp_delay > work_mid) {
		tmp_delay -= IO_DELAY_PER_DCHAIN_TAP;
		d--;
	}

	pr_debug("wlevel: p=%u d=%u tmp_delay=%u left=%u\n", p, d,
		tmp_delay, work_mid - tmp_delay);

	scc_mgr_apply_group_all_out_delay_add_all_ranks (g, test_bgn, d);

	pr_debug("wlevel: found middle: p=%u d=%u\n", p, d);

	return 1;
}

/* center all windows. do per-bit-deskew to possibly increase size of
certain windows */

static uint32_t rw_mgr_mem_calibrate_writes_center (uint32_t rank_bgn,
	uint32_t write_group, uint32_t test_bgn)
{
	uint32_t i, p, min_index;
	int32_t d;
	/* Store these as signed since there are comparisons with
	signed numbers */
	t_btfld bit_chk;
	t_btfld sticky_bit_chk;
	int32_t left_edge[RW_MGR_MEM_DQ_PER_WRITE_DQS];
	int32_t right_edge[RW_MGR_MEM_DQ_PER_WRITE_DQS];
	int32_t mid;
	int32_t mid_min, orig_mid_min;
	int32_t new_dqs, start_dqs, shift_dq;
	int32_t dq_margin, dqs_margin, dm_margin;
	uint32_t stop;
	int32_t bgn_curr;
	int32_t end_curr;
	int32_t bgn_best;
	int32_t end_best;
	int32_t win_best;

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	dm_margin = 0;

	start_dqs = READ_SCC_DQS_IO_OUT1_DELAY();

	/* per-bit deskew */

	/* set the left and right edge of each bit to an illegal value */
	/* use (IO_IO_OUT1_DELAY_MAX + 1) as an illegal value */
	sticky_bit_chk = 0;
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		left_edge[i]  = IO_IO_OUT1_DELAY_MAX + 1;
		right_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
	}

	/* Search for the left edge of the window for each bit */
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_out1_delay (write_group, test_bgn, d);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		/* Stop searching when the read test doesn't pass AND when
		we've seen a passing read on every bit */
		stop = !rw_mgr_mem_calibrate_write_test (rank_bgn, write_group,
			0, PASS_ONE_BIT, &bit_chk, 0);
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->write_correct_mask);
		pr_debug("write_center(left): dtap=%u => " BTFLD_FMT
			" == " BTFLD_FMT " && %u [bit_chk=" BTFLD_FMT "]\n",
			d, sticky_bit_chk, param->write_correct_mask,
			stop, bit_chk);

		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
				if (bit_chk & 1) {
					/* Remember a passing test as the
					left_edge */
					left_edge[i] = d;
				} else {
					/* If a left edge has not been seen yet,
					then a future passing test will mark
					this edge as the right edge */
					if (left_edge[i] ==
						IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[i] = -(d + 1);
					}
				}
				pr_debug("write_center[l,d=%u): "
					"bit_chk_test=%d left_edge[%u]: %d "
					"right_edge[%u]: %d\n",
					d, (int)(bit_chk & 1), i, left_edge[i],
					i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	/* Reset DQ delay chains to 0 */
	scc_mgr_apply_group_dq_out1_delay (write_group, test_bgn, 0);
	sticky_bit_chk = 0;
	for (i = RW_MGR_MEM_DQ_PER_WRITE_DQS - 1;; i--) {

		pr_debug("write_center: left_edge[%u]: %d right_edge[%u]: "
			"%d\n", i, left_edge[i], i, right_edge[i]);

		/* Check for cases where we haven't found the left edge,
		which makes our assignment of the the right edge invalid.
		Reset it to the illegal value. */
		if ((left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) &&
			(right_edge[i] != IO_IO_OUT1_DELAY_MAX + 1)) {
			right_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
			pr_debug("write_center: reset right_edge[%u]: %d\n",
			i, right_edge[i]);
		}

		/* Reset sticky bit (except for bits where we have
		seen the left edge) */
		sticky_bit_chk = sticky_bit_chk << 1;
		if ((left_edge[i] != IO_IO_OUT1_DELAY_MAX + 1))
			sticky_bit_chk = sticky_bit_chk | 1;

		if (i == 0)
			break;
	}

	/* Search for the right edge of the window for each bit */
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX - start_dqs; d++) {
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group,
			d + start_dqs);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		/* Stop searching when the read test doesn't pass AND when
		we've seen a passing read on every bit */
		stop = !rw_mgr_mem_calibrate_write_test (rank_bgn, write_group,
			0, PASS_ONE_BIT, &bit_chk, 0);
		if (stop) {
			recover_mem_device_after_ck_dqs_violation();
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->write_correct_mask);

		pr_debug("write_center (right): dtap=%u => " BTFLD_FMT " == "
			BTFLD_FMT " && %u\n", d, sticky_bit_chk,
			param->write_correct_mask, stop);

		if (stop == 1) {
			if (d == 0) {
				for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS;
					i++) {
					/* d = 0 failed, but it passed when
					testing the left edge, so it must be
					marginal, set it to -1 */
					if (right_edge[i] ==
						IO_IO_OUT1_DELAY_MAX + 1 &&
						left_edge[i] !=
						IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[i] = -1;
					}
				}
			}
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
				if (bit_chk & 1) {
					/* Remember a passing test as
					the right_edge */
					right_edge[i] = d;
				} else {
					if (d != 0) {
						/* If a right edge has not
						been seen yet, then a future
						passing test will mark this
						edge as the left edge */
						if (right_edge[i] ==
							IO_IO_OUT1_DELAY_MAX
							+ 1) {
							left_edge[i] = -(d + 1);
						}
					} else {
						/* d = 0 failed, but it passed
						when testing the left edge, so
						it must be marginal, set it
						to -1 */
						if (right_edge[i] ==
							IO_IO_OUT1_DELAY_MAX +
							1 && left_edge[i] !=
							IO_IO_OUT1_DELAY_MAX +
							1) {
							right_edge[i] = -1;
						}
						/* If a right edge has not been
						seen yet, then a future passing
						test will mark this edge as the
						left edge */
						else if (right_edge[i] ==
							IO_IO_OUT1_DELAY_MAX +
						1) {
							left_edge[i] = -(d + 1);
						}
					}
				}
				pr_debug("write_center[r,d=%u): "
					"bit_chk_test=%d left_edge[%u]: %d "
					"right_edge[%u]: %d\n",
					d, (int)(bit_chk & 1), i, left_edge[i],
					i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	/* Check that all bits have a window */
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		pr_debug("write_center: left_edge[%u]: %d right_edge[%u]: "
			"%d\n", i, left_edge[i], i, right_edge[i]);
		if ((left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) ||
				(right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1))
			return 0;
	}

	/* Find middle of window for each DQ bit */
	mid_min = left_edge[0] - right_edge[0];
	min_index = 0;
	for (i = 1; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		mid = left_edge[i] - right_edge[i];
		if (mid < mid_min) {
			mid_min = mid;
			min_index = i;
		}
	}

	/*  -mid_min/2 represents the amount that we need to move DQS.
	If mid_min is odd and positive we'll need to add one to
	make sure the rounding in further calculations is correct
	(always bias to the right), so just add 1 for all positive values */
	if (mid_min > 0)
		mid_min++;

	mid_min = mid_min / 2;

	pr_debug("write_center: mid_min=%d\n", mid_min);

	/* Determine the amount we can change DQS (which is -mid_min) */
	orig_mid_min = mid_min;
	new_dqs = start_dqs;
	mid_min = 0;

	pr_debug("write_center: start_dqs=%d new_dqs=%d mid_min=%d\n", start_dqs, new_dqs, mid_min);

	/* Initialize data for export structures */
	dqs_margin = IO_IO_OUT1_DELAY_MAX + 1;
	dq_margin  = IO_IO_OUT1_DELAY_MAX + 1;

	/* add delay to bring centre of all DQ windows to the same "level" */
	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		/* Use values before divide by 2 to reduce round off error */
		shift_dq = (left_edge[i] - right_edge[i] -
			(left_edge[min_index] - right_edge[min_index]))/2  +
		(orig_mid_min - mid_min);

		pr_debug("write_center: before: shift_dq[%u]=%d\n", i,
			shift_dq);

		if (shift_dq + (int32_t)READ_SCC_DQ_OUT1_DELAY(i) >
			(int32_t)IO_IO_OUT1_DELAY_MAX) {
			shift_dq = (int32_t)IO_IO_OUT1_DELAY_MAX -
			READ_SCC_DQ_OUT1_DELAY(i);
		} else if (shift_dq + (int32_t)READ_SCC_DQ_OUT1_DELAY(i) < 0) {
			shift_dq = -(int32_t)READ_SCC_DQ_OUT1_DELAY(i);
		}
		pr_debug("write_center: after: shift_dq[%u]=%d\n",
			i, shift_dq);
		scc_mgr_set_dq_out1_delay(write_group, i,
			READ_SCC_DQ_OUT1_DELAY(i) + shift_dq);
		scc_mgr_load_dq (i);

		pr_debug("write_center: margin[%u]=[%d,%d]\n", i,
			left_edge[i] - shift_dq + (-mid_min),
			right_edge[i] + shift_dq - (-mid_min));
		/* To determine values for export structures */
		if (left_edge[i] - shift_dq + (-mid_min) < dq_margin)
			dq_margin = left_edge[i] - shift_dq + (-mid_min);
		if (right_edge[i] + shift_dq - (-mid_min) < dqs_margin)
			dqs_margin = right_edge[i] + shift_dq - (-mid_min);
	}

	/* Move DQS */
	scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, new_dqs);
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

	/* Centre DM */

	pr_debug("write_center: DM\n");

	/* set the left and right edge of each bit to an illegal value */
	/* use (IO_IO_OUT1_DELAY_MAX + 1) as an illegal value */
	left_edge[0]  = IO_IO_OUT1_DELAY_MAX + 1;
	right_edge[0] = IO_IO_OUT1_DELAY_MAX + 1;
	bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
	end_curr = IO_IO_OUT1_DELAY_MAX + 1;
	bgn_best = IO_IO_OUT1_DELAY_MAX + 1;
	end_best = IO_IO_OUT1_DELAY_MAX + 1;
	win_best = 0;

	/* Search for the/part of the window with DM shift */
	for (d = IO_IO_OUT1_DELAY_MAX; d >= 0; d -= DELTA_D) {
		scc_mgr_apply_group_dm_out1_delay (write_group, d);
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		if (rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1,
			PASS_ALL_BITS, &bit_chk, 0)) {

			/*USE Set current end of the window */
			end_curr = -d;
			/* If a starting edge of our window has not been seen
			this is our current start of the DM window */
			if (bgn_curr == IO_IO_OUT1_DELAY_MAX + 1)
				bgn_curr = -d;

			/* If current window is bigger than best seen.
			Set best seen to be current window */
			if ((end_curr-bgn_curr+1) > win_best) {
				win_best = end_curr-bgn_curr+1;
				bgn_best = bgn_curr;
				end_best = end_curr;
			}
		} else {
			/* We just saw a failing test. Reset temp edge */
			bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
			end_curr = IO_IO_OUT1_DELAY_MAX + 1;
		}
	}

	/* Reset DM delay chains to 0 */
	scc_mgr_apply_group_dm_out1_delay (write_group, 0);

	/* Check to see if the current window nudges up aganist 0 delay.
	If so we need to continue the search by shifting DQS otherwise DQS
	search begins as a new search */
	if (end_curr != 0) {
		bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
		end_curr = IO_IO_OUT1_DELAY_MAX + 1;
	}

	/* Search for the/part of the window with DQS shifts */
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX - new_dqs; d += DELTA_D) {
		/* Note: This only shifts DQS, so are we limiting ourselve to */
		/* width of DQ unnecessarily */
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group,
			d + new_dqs);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		if (rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1,
			PASS_ALL_BITS, &bit_chk, 0)) {

			/*USE Set current end of the window */
			end_curr = d;
			/* If a beginning edge of our window has not been seen
			this is our current begin of the DM window */
			if (bgn_curr == IO_IO_OUT1_DELAY_MAX + 1)
				bgn_curr = d;

			/* If current window is bigger than best seen. Set best
			seen to be current window */
			if ((end_curr-bgn_curr+1) > win_best) {
				win_best = end_curr-bgn_curr+1;
				bgn_best = bgn_curr;
				end_best = end_curr;
			}
		} else {
			/* We just saw a failing test. Reset temp edge */
			recover_mem_device_after_ck_dqs_violation();
			bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
			end_curr = IO_IO_OUT1_DELAY_MAX + 1;

			/* Early exit optimization: if ther remaining delay
			chain space is less than already seen largest window
			we can exit */
			if ((win_best - 1) > (IO_IO_OUT1_DELAY_MAX - new_dqs - d))
				break;
		}
	}

	/* assign left and right edge for cal and reporting; */
	left_edge[0] = -1*bgn_best;
	right_edge[0] = end_best;

	pr_debug("dm_calib: left=%d right=%d\n", left_edge[0], right_edge[0]);

	/* Move DQS (back to orig) */
	scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, new_dqs);

	/* Move DM */

	/* Find middle of window for the DM bit */
	mid = (left_edge[0] - right_edge[0]) / 2;

	/* only move right, since we are not moving DQS/DQ */
	if (mid < 0)
		mid = 0;

	/*dm_marign should fail if we never find a window */
	if (win_best == 0) {
		dm_margin = -1;
	} else {
		dm_margin = left_edge[0] - mid;
	}

	scc_mgr_apply_group_dm_out1_delay(write_group, mid);
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

	pr_debug("dm_calib: left=%d right=%d mid=%d dm_margin=%d\n",
		left_edge[0], right_edge[0], mid, dm_margin);

	/* Export values */
	gbl->fom_out += dq_margin + dqs_margin;

	pr_debug("write_center: dq_margin=%d dqs_margin=%d dm_margin=%d\n",
		dq_margin, dqs_margin, dm_margin);

	/* Do not remove this line as it makes sure all of our
	decisions have been applied */
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	return (dq_margin >= 0) && (dqs_margin >= 0) && (dm_margin >= 0);
}

/* calibrate the write operations */

static uint32_t rw_mgr_mem_calibrate_writes (uint32_t rank_bgn, uint32_t g,
	uint32_t test_bgn)
{
	reg_file_set_stage(CAL_STAGE_WRITES);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WRITES_CENTER);

	reg_file_set_group(g);

	return rw_mgr_mem_calibrate_writes_center (rank_bgn, g, test_bgn);
}

/* precharge all banks and activate row 0 in bank "000..." and bank "111..." */
static void mem_precharge_and_activate (void)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		/* set rank */
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		/* precharge all banks ... */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_PRECHARGE_ALL);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x0F);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0,
			__RW_MGR_ACTIVATE_0_AND_1_WAIT1);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x0F);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0,
			__RW_MGR_ACTIVATE_0_AND_1_WAIT2);

		/* activate rows */
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0,
			__RW_MGR_ACTIVATE_0_AND_1);
	}
}

/* perform all refreshes necessary over all ranks */

/* Configure various memory related parameters. */
static void mem_config (void)
{
	uint32_t rlat, wlat;
	uint32_t rw_wl_nop_cycles;
	uint32_t max_latency;

	/* read in write and read latency */

	wlat = IORD_32DIRECT (MEM_T_WL_ADD, 0);
	wlat += IORD_32DIRECT (DATA_MGR_MEM_T_ADD, 0);
	/* WL for hard phy does not include additive latency */

	rlat = IORD_32DIRECT (MEM_T_RL_ADD, 0);

	if (QUARTER_RATE_MODE) {
		/* In Quarter-Rate the WL-to-nop-cycles works like this */
		/* 0,1     -> 0 */
		/* 2,3,4,5 -> 1 */
		/* 6,7,8,9 -> 2 */
		/* etc... */
		rw_wl_nop_cycles = (wlat + 6) / 4 - 1;
	} else if (HALF_RATE_MODE)	{
		/* In Half-Rate the WL-to-nop-cycles works like this */
		/* 0,1 -> -1 */
		/* 2,3 -> 0 */
		/* 4,5 -> 1 */
		/* etc... */
		if (wlat % 2)
			rw_wl_nop_cycles = ((wlat - 1) / 2) - 1;
		else
			rw_wl_nop_cycles = (wlat / 2) - 1;
	} else {
		rw_wl_nop_cycles = wlat - 2;
#if LPDDR2
		rw_wl_nop_cycles = rw_wl_nop_cycles + 1;
#endif
	}
#if MULTIPLE_AFI_WLAT
	for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
		gbl->rw_wl_nop_cycles_per_group[i] = rw_wl_nop_cycles;
	}
#endif
	gbl->rw_wl_nop_cycles = rw_wl_nop_cycles;

#if ARRIAV || CYCLONEV
	/* For AV/CV, lfifo is hardened and always runs at full rate so
	max latency in AFI clocks, used here, is correspondingly smaller */
	if (QUARTER_RATE_MODE) {
		max_latency = (1<<MAX_LATENCY_COUNT_WIDTH)/4 - 1;
	} else if (HALF_RATE_MODE) {
		max_latency = (1<<MAX_LATENCY_COUNT_WIDTH)/2 - 1;
	} else {
		max_latency = (1<<MAX_LATENCY_COUNT_WIDTH)/1 - 1;
	}
#else
	max_latency = (1<<MAX_LATENCY_COUNT_WIDTH) - 1;
#endif
	/* configure for a burst length of 8 */

	if (QUARTER_RATE_MODE) {
		/* write latency */
		wlat = (wlat + 5) / 4 + 1;

		/* set a pretty high read latency initially */
		gbl->curr_read_lat = (rlat + 1) / 4 + 8;
	} else if (HALF_RATE_MODE) {
		/* write latency */
		wlat = (wlat - 1) / 2 + 1;

		/* set a pretty high read latency initially */
		gbl->curr_read_lat = (rlat + 1) / 2 + 8;
	} else {
		/* write latency */
		/* Adjust Write Latency for Hard PHY */
		wlat = wlat + 1;
#if LPDDR2
		/* Add another one in hard for LPDDR2 since this value is raw
		from controller assume tdqss is one */
		wlat = wlat + 1;
#endif

		/* set a pretty high read latency initially */
		gbl->curr_read_lat = rlat + 16;
	}

	if (gbl->curr_read_lat > max_latency)
		gbl->curr_read_lat = max_latency;

	IOWR_32DIRECT(PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);

	/* advertise write latency */
	gbl->curr_write_lat = wlat;
#if MULTIPLE_AFI_WLAT
	for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
		IOWR_32DIRECT(PHY_MGR_AFI_WLAT, i*4, wlat - 2);
	}
#else
	IOWR_32DIRECT(PHY_MGR_AFI_WLAT, 0, wlat - 2);
#endif

	mem_precharge_and_activate ();
}

/* Memory calibration entry point */

static uint32_t mem_calibrate (void)
{
	uint32_t i;
	uint32_t rank_bgn;
	uint32_t write_group, write_test_bgn;
	uint32_t read_group, read_test_bgn;
	uint32_t run_groups, current_run;

	/* Initialize the data settings */
	pr_debug("Preparing to init data\n");
	pr_debug("Init complete\n");

	gbl->error_substage = CAL_SUBSTAGE_NIL;
	gbl->error_stage = CAL_STAGE_NIL;
	gbl->error_group = 0xff;
	gbl->fom_in = 0;
	gbl->fom_out = 0;

	mem_config ();

	if (ARRIAV || CYCLONEV) {
		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
			IOWR_32DIRECT(SCC_MGR_GROUP_COUNTER, 0, i);
			scc_set_bypass_mode(i);
		}
	}

	/* Zero all delay chain/phase settings for all
	groups and all shadow register sets */
	scc_mgr_zero_all ();

	run_groups = ~0;

	for (write_group = 0, write_test_bgn = 0; write_group
		< RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++,
		write_test_bgn += RW_MGR_MEM_DQ_PER_WRITE_DQS) {

		/* Mark the group as being attempted for calibration */

		current_run = run_groups & ((1 << RW_MGR_NUM_DQS_PER_WRITE_GROUP) - 1);
		run_groups = run_groups >> RW_MGR_NUM_DQS_PER_WRITE_GROUP;

		if (current_run == 0)
			continue;

		IOWR_32DIRECT(SCC_MGR_GROUP_COUNTER, 0, write_group);
		scc_mgr_zero_group (write_group, write_test_bgn, 0);

		for (read_group = write_group * RW_MGR_NUM_DQS_PER_WRITE_GROUP,
			read_test_bgn = 0;
			read_group < (write_group + 1) * RW_MGR_NUM_DQS_PER_WRITE_GROUP;
			read_group++, read_test_bgn += RW_MGR_MEM_DQ_PER_READ_DQS) {

			/* Calibrate the VFIFO */
			if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_VFIFO)) {
				if (!rw_mgr_mem_calibrate_vfifo(read_group, read_test_bgn))
					return 0;
			}
		}

		/* level writes (or align DK with CK for RLDRAMX) */
		if (!(ARRIAV || CYCLONEV)) {
			if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WLEVEL)) {
				if (!rw_mgr_mem_calibrate_wlevel(write_group, write_test_bgn))
					return 0;
			}
		}

		/* Calibrate the output side */
		for (rank_bgn = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS;
				rank_bgn += NUM_RANKS_PER_SHADOW_REG) {
			if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES)) {
				if ((STATIC_CALIB_STEPS) & CALIB_SKIP_DELAY_SWEEPS) {
					/* not needed in quick mode! */
				} else {
					/* Determine if this set of
					 * ranks should be skipped
					 * entirely */
					if (!rw_mgr_mem_calibrate_writes(rank_bgn, write_group, write_test_bgn))
						return 0;
				}
			}
		}

		for (read_group = write_group * RW_MGR_NUM_DQS_PER_WRITE_GROUP,
				read_test_bgn = 0;
				read_group < (write_group + 1) * RW_MGR_NUM_DQS_PER_WRITE_GROUP;
				read_group++, read_test_bgn += RW_MGR_MEM_DQ_PER_READ_DQS) {
			if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES)) {
				if (!rw_mgr_mem_calibrate_vfifo_end(read_group, read_test_bgn))
					return 0;
			}
		}
	}

	/* Calibrate the LFIFO */
	if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_LFIFO)) {
		/* If we're skipping groups as part of debug,
		don't calibrate LFIFO */
		if (!rw_mgr_mem_calibrate_lfifo ())
			return 0;
	}

	/* Do not remove this line as it makes sure all of our decisions
	have been applied */
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	return 1;
}

static uint32_t run_mem_calibrate(void)
{
	uint32_t pass;
	uint32_t debug_info;

	/* Initialize the debug status to show that calibration has started. */
	/* This should occur before anything else */
	/* Reset pass/fail status shown on afi_cal_success/fail */
	IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_RESET);

	initialize();
	rw_mgr_mem_initialize ();
	pass = mem_calibrate ();
	mem_precharge_and_activate ();

	IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);

	/* Handoff */

	/* Don't return control of the PHY back to AFI when in debug mode */
	if ((gbl->phy_debug_mode_flags & PHY_DEBUG_IN_DEBUG_MODE) == 0) {
		rw_mgr_mem_handoff ();

		/* In Hard PHY this is a 2-bit control: */
		/* 0: AFI Mux Select */
		/* 1: DDIO Mux Select */
		IOWR_32DIRECT(PHY_MGR_MUX_SEL, 0, 0x2);
	}

	if (pass) {
		pr_debug("CALIBRATION PASSED\n");

		gbl->fom_in /= 2;
		gbl->fom_out /= 2;

		if (gbl->fom_in > 0xff) {
			gbl->fom_in = 0xff;
		}

		if (gbl->fom_out > 0xff) {
			gbl->fom_out = 0xff;
		}

		/* Update the FOM in the register file */
		debug_info = gbl->fom_in;
		debug_info |= gbl->fom_out << 8;
		IOWR_32DIRECT(REG_FILE_FOM, 0, debug_info);

		IOWR_32DIRECT(PHY_MGR_CAL_DEBUG_INFO, 0, debug_info);
		IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_SUCCESS);

	} else {
		pr_debug("CALIBRATION FAILED\n");

		debug_info = gbl->error_stage;
		debug_info |= gbl->error_substage << 8;
		debug_info |= gbl->error_group << 16;


		IOWR_32DIRECT(REG_FILE_FAILING_STAGE, 0, debug_info);
		IOWR_32DIRECT(PHY_MGR_CAL_DEBUG_INFO, 0, debug_info);
		IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_FAIL);

		/* Update the failing group/stage in the register file */
		debug_info = gbl->error_stage;
		debug_info |= gbl->error_substage << 8;
		debug_info |= gbl->error_group << 16;
		IOWR_32DIRECT(REG_FILE_FAILING_STAGE, 0, debug_info);
	}

	/* Set the debug status to show that calibration has ended. */
	/* This should occur after everything else */
	return pass;

}

static void hc_initialize_rom_data(const uint32_t *inst_rom_init, uint32_t inst_rom_init_size,
		const uint32_t *ac_rom_init, uint32_t ac_rom_init_size)
{
	uint32_t i;

	for (i = 0; i < inst_rom_init_size; i++) {
		uint32_t data = inst_rom_init[i];
		IOWR_32DIRECT(RW_MGR_INST_ROM_WRITE, (i << 2), data);
	}

	for (i = 0; i < ac_rom_init_size; i++) {
		uint32_t data = ac_rom_init[i];
		IOWR_32DIRECT(RW_MGR_AC_ROM_WRITE, (i << 2), data);
	}
}

static void initialize_reg_file(void)
{
	/* Initialize the register file with the correct data */
	IOWR_32DIRECT(REG_FILE_SIGNATURE, 0, REG_FILE_INIT_SEQ_SIGNATURE);
	IOWR_32DIRECT(REG_FILE_DEBUG_DATA_ADDR, 0, 0);
	IOWR_32DIRECT(REG_FILE_CUR_STAGE, 0, 0);
	IOWR_32DIRECT(REG_FILE_FOM, 0, 0);
	IOWR_32DIRECT(REG_FILE_FAILING_STAGE, 0, 0);
	IOWR_32DIRECT(REG_FILE_DEBUG1, 0, 0);
	IOWR_32DIRECT(REG_FILE_DEBUG2, 0, 0);
}

static void initialize_hps_phy(void)
{
	/* These may need to be included also: */
	/* wrap_back_en (false) */
	/* atpg_en (false) */
	/* pipelineglobalenable (true) */

	uint32_t reg;
	/* Tracking also gets configured here because it's in the
	same register */
	uint32_t trk_sample_count = 7500;
	uint32_t trk_long_idle_sample_count = (10 << 16) | 100;
	/* Format is number of outer loops in the 16 MSB, sample
	count in 16 LSB. */

	reg = 0;
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ACDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSLOGICDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_RESETDELAYEN_SET(0);
#if LPDDR2
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_LPDDRDIS_SET(0);
#else
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_LPDDRDIS_SET(1);
#endif
	/* Fix for long latency VFIFO */
	/* This field selects the intrinsic latency to RDATA_EN/FULL path.
	00-bypass, 01- add 5 cycles, 10- add 10 cycles, 11- add 15 cycles. */
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ADDLATSEL_SET(0);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_SET(
		trk_sample_count);
	IOWR_32DIRECT(BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_SAMPLECOUNT_31_20_SET(
		trk_sample_count >>
		SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_WIDTH);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_SET(
		trk_long_idle_sample_count);
	IOWR_32DIRECT(BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_LONGIDLESAMPLECOUNT_31_20_SET(
		trk_long_idle_sample_count >>
		SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_WIDTH);
	IOWR_32DIRECT(BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_OFFSET, reg);
}

#if USE_DQS_TRACKING

static void initialize_tracking(void)
{
	uint32_t concatenated_longidle = 0x0;
	uint32_t concatenated_delays = 0x0;
	uint32_t concatenated_rw_addr = 0x0;
	uint32_t concatenated_refresh = 0x0;
	uint32_t dtaps_per_ptap;
	uint32_t tmp_delay;

	/* compute usable version of value in case we skip full
	computation later */
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DCHAIN_TAP;
	}
	dtaps_per_ptap--;

	concatenated_longidle = concatenated_longidle ^ 10;
		/*longidle outer loop */
	concatenated_longidle = concatenated_longidle << 16;
	concatenated_longidle = concatenated_longidle ^ 100;
		/*longidle sample count */

	concatenated_delays = concatenated_delays ^ 243;
		/* trfc, worst case of 933Mhz 4Gb */
	concatenated_delays = concatenated_delays << 8;
	concatenated_delays = concatenated_delays ^ 14;
		/* trcd, worst case */
	concatenated_delays = concatenated_delays << 8;
	concatenated_delays = concatenated_delays ^ 5;
		/* vfifo wait */
	concatenated_delays = concatenated_delays << 8;
	concatenated_delays = concatenated_delays ^ 4;
		/* mux delay */

#if DDR3 || LPDDR2
	concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_IDLE;
	concatenated_rw_addr = concatenated_rw_addr << 8;
	concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_ACTIVATE_1;
	concatenated_rw_addr = concatenated_rw_addr << 8;
	concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_SGLE_READ;
	concatenated_rw_addr = concatenated_rw_addr << 8;
	concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_PRECHARGE_ALL;
#endif

#if DDR3 || LPDDR2
	concatenated_refresh = concatenated_refresh ^ __RW_MGR_REFRESH_ALL;
#else
	concatenated_refresh = concatenated_refresh ^ 0;
#endif
	concatenated_refresh = concatenated_refresh << 24;
	concatenated_refresh = concatenated_refresh ^ 1000; /* trefi */

	/* Initialize the register file with the correct data */
	IOWR_32DIRECT(REG_FILE_DTAPS_PER_PTAP, 0, dtaps_per_ptap);
	IOWR_32DIRECT(REG_FILE_TRK_SAMPLE_COUNT, 0, 7500);
	IOWR_32DIRECT(REG_FILE_TRK_LONGIDLE, 0, concatenated_longidle);
	IOWR_32DIRECT(REG_FILE_DELAYS, 0, concatenated_delays);
	IOWR_32DIRECT(REG_FILE_TRK_RW_MGR_ADDR, 0, concatenated_rw_addr);
	IOWR_32DIRECT(REG_FILE_TRK_READ_DQS_WIDTH, 0,
		RW_MGR_MEM_IF_READ_DQS_WIDTH);
	IOWR_32DIRECT(REG_FILE_TRK_RFSH, 0, concatenated_refresh);
}

#endif	/* USE_DQS_TRACKING */

static int socfpga_sdram_calibration(const uint32_t *inst_rom_init, uint32_t inst_rom_init_size,
		const uint32_t *ac_rom_init, uint32_t ac_rom_init_size)
{
	param_t my_param;
	gbl_t my_gbl;
	uint32_t pass;

	param = &my_param;
	gbl = &my_gbl;

	/* Initialize the debug mode flags */
	gbl->phy_debug_mode_flags = 0;
	/* Set the calibration enabled by default */
	gbl->phy_debug_mode_flags |= PHY_DEBUG_ENABLE_CAL_RPT;

	/* Initialize the register file */
	initialize_reg_file();

	/* Initialize any PHY CSR */
	initialize_hps_phy();

	scc_mgr_initialize();

#if USE_DQS_TRACKING
	initialize_tracking();
#endif
	pr_debug("Preparing to start memory calibration\n");

	pr_debug("%s%s %s ranks=%u cs/dimm=%u dq/dqs=%u,%u vg/dqs=%u,%u "
		"dqs=%u,%u dq=%u dm=%u "
		"ptap_delay=%u dtap_delay=%u dtap_dqsen_delay=%u, dll=%u\n",
		RDIMM ? "r" : (LRDIMM ? "l" : ""),
		DDR2 ? "DDR2" : (DDR3 ? "DDR3" : (QDRII ? "QDRII" : (RLDRAMII ?
		"RLDRAMII" : (RLDRAM3 ? "RLDRAM3" : "??PROTO??")))),
		FULL_RATE ? "FR" : (HALF_RATE ? "HR" : (QUARTER_RATE ?
		"QR" : "??RATE??")),
		RW_MGR_MEM_NUMBER_OF_RANKS,
		RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM,
		RW_MGR_MEM_DQ_PER_READ_DQS,
		RW_MGR_MEM_DQ_PER_WRITE_DQS,
		RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS,
		RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS,
		RW_MGR_MEM_IF_READ_DQS_WIDTH,
		RW_MGR_MEM_IF_WRITE_DQS_WIDTH,
		RW_MGR_MEM_DATA_WIDTH,
		RW_MGR_MEM_DATA_MASK_WIDTH,
		IO_DELAY_PER_OPA_TAP,
		IO_DELAY_PER_DCHAIN_TAP,
		IO_DELAY_PER_DQS_EN_DCHAIN_TAP,
		IO_DLL_CHAIN_LENGTH);
	pr_debug("max values: en_p=%u dqdqs_p=%u en_d=%u dqs_in_d=%u "
		"io_in_d=%u io_out1_d=%u io_out2_d=%u"
		"dqs_in_reserve=%u dqs_out_reserve=%u\n",
		IO_DQS_EN_PHASE_MAX,
		IO_DQDQS_OUT_PHASE_MAX,
		IO_DQS_EN_DELAY_MAX,
		IO_DQS_IN_DELAY_MAX,
		IO_IO_IN_DELAY_MAX,
		IO_IO_OUT1_DELAY_MAX,
		IO_IO_OUT2_DELAY_MAX,
		IO_DQS_IN_RESERVE,
		IO_DQS_OUT_RESERVE);

	hc_initialize_rom_data(inst_rom_init, inst_rom_init_size,
			ac_rom_init, ac_rom_init_size);

	/* update info for sims */
	reg_file_set_stage(CAL_STAGE_NIL);
	reg_file_set_group(0);

	/* Load global needed for those actions that require */
	/* some dynamic calibration support */
	dyn_calib_steps = STATIC_CALIB_STEPS;

	/* Load global to allow dynamic selection of delay loop settings */
	/* based on calibration mode */
	if (!(dyn_calib_steps & CALIB_SKIP_DELAY_LOOPS)) {
		skip_delay_mask = 0xff;
	} else {
		skip_delay_mask = 0x0;
	}

	pass = run_mem_calibrate ();

	pr_debug("Calibration complete\n");
	/* Send the end of transmission character */
	pr_debug("%c\n", 0x4);

	return pass == 0 ? -EINVAL : 0;
}
