/*
* Copyright Altera Corporation (C) 2012-2014. All rights reserved
*
* SPDX-License-Identifier:  BSD-3-Clause
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*  * Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the
*  documentation and/or other materials provided with the distribution.
*  * Neither the name of Altera Corporation nor the
*  names of its contributors may be used to endorse or promote products
*  derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL ALTERA CORPORATION BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "system.h"
#include "sdram_io.h"
#include "cyclone5-sequencer.h"
#include "tclrpt.h"

/******************************************************************************
 ******************************************************************************
 ** NOTE: Special Rules for Globale Variables                                **
 **                                                                          **
 ** All global variables that are explicitly initialized (including          **
 ** explicitly initialized to zero), are only initialized once, during       **
 ** configuration time, and not again on reset.  This means that they        **
 ** preserve their current contents across resets, which is needed for some  **
 ** special cases involving communication with external modules.  In         **
 ** addition, this avoids paying the price to have the memory initialized,   **
 ** even for zeroed data, provided it is explicitly set to zero in the code, **
 ** and doesn't rely on implicit initialization.                             **
 ******************************************************************************
 ******************************************************************************/

#ifndef ARMCOMPILER

// Temporary workaround to place the initial stack pointer at a safe offset from end
#define STRINGIFY(s)		STRINGIFY_STR(s)
#define STRINGIFY_STR(s)	#s
asm(".global __alt_stack_pointer");
asm("__alt_stack_pointer = " STRINGIFY(STACK_POINTER));
#endif

#include <mach/cyclone5-sdram.h>

#define NEWVERSION_RDDESKEW 1
#define NEWVERSION_WRDESKEW 1
#define NEWVERSION_GW 1
#define NEWVERSION_WL 1
#define NEWVERSION_DQSEN 1

// Just to make the debugging code more uniform

#define HALF_RATE_MODE 0

#define QUARTER_RATE_MODE 0
#define DELTA_D 1

// case:56390
// VFIFO_CONTROL_WIDTH_PER_DQS is the number of VFIFOs actually instantiated per DQS. This is always one except:
// AV QDRII where it is 2 for x18 and x18w2, and 4 for x36 and x36w2
// RLDRAMII x36 and x36w2 where it is 2.
// In 12.0sp1 we set this to 4 for all of the special cases above to keep it simple.
// In 12.0sp2 or 12.1 this should get moved to generation and unified with the same constant used in the phy mgr

#define VFIFO_CONTROL_WIDTH_PER_DQS 1

// In order to reduce ROM size, most of the selectable calibration steps are
// decided at compile time based on the user's calibration mode selection,
// as captured by the STATIC_CALIB_STEPS selection below.
//
// However, to support simulation-time selection of fast simulation mode, where
// we skip everything except the bare minimum, we need a few of the steps to
// be dynamic.  In those cases, we either use the DYNAMIC_CALIB_STEPS for the
// check, which is based on the rtl-supplied value, or we dynamically compute the
// value to use based on the dynamically-chosen calibration mode

#define BTFLD_FMT "%lx"

// For HPS running on actual hardware

#define DLEVEL 0
#ifdef HPS_HW_SERIAL_SUPPORT
// space around comma is required for varargs macro to remove comma if args is empty
#define DPRINT(level, fmt, args...) 	if (DLEVEL >= (level)) printf("SEQ.C: " fmt "\n" , ## args)
#define IPRINT(fmt, args...) 	        printf("SEQ.C: " fmt "\n" , ## args)
#else
#define DPRINT(level, fmt, args...)
#define IPRINT(fmt, args...)
#endif
#define BFM_GBL_SET(field,value)
#define BFM_GBL_GET(field) 		((long unsigned int)0)
#define BFM_STAGE(stage)
#define BFM_INC_VFIFO
#define COV(label)

#define TRACE_FUNC(fmt, args...) DPRINT(1, "%s[%d]: " fmt, __func__, __LINE__ , ## args)

#define DYNAMIC_CALIB_STEPS (dyn_calib_steps)

#define STATIC_IN_RTL_SIM 0

#define STATIC_SKIP_DELAY_LOOPS 0

#define STATIC_CALIB_STEPS (STATIC_IN_RTL_SIM | CALIB_SKIP_FULL_TEST | STATIC_SKIP_DELAY_LOOPS)

// calibration steps requested by the rtl
static uint16_t dyn_calib_steps = 0;

// To make CALIB_SKIP_DELAY_LOOPS a dynamic conditional option
// instead of static, we use boolean logic to select between
// non-skip and skip values
//
// The mask is set to include all bits when not-skipping, but is
// zero when skipping

static uint16_t skip_delay_mask = 0;	// mask off bits when skipping/not-skipping

#define SKIP_DELAY_LOOP_VALUE_OR_ZERO(non_skip_value) \
	((non_skip_value) & skip_delay_mask)

// TODO: The skip group strategy is completely missing

static gbl_t *gbl = 0;
static param_t *param = 0;

static uint32_t rw_mgr_mem_calibrate_write_test(uint32_t rank_bgn, uint32_t write_group,
						uint32_t use_dm, uint32_t all_correct,
						t_btfld * bit_chk, uint32_t all_ranks);

// This (TEST_SIZE) is used to test handling of large roms, to make
// sure we are sizing things correctly
// Note, the initialized data takes up twice the space in rom, since
// there needs to be a copy with the initial value and a copy that is
// written too, since on soft-reset, it needs to have the initial values
// without reloading the memory from external sources

// #define TEST_SIZE    (6*1024)

#ifdef TEST_SIZE

#define PRE_POST_TEST_SIZE 3

static unsigned int pre_test_size_mem[PRE_POST_TEST_SIZE] = { 1, 2, 3 };

static unsigned int test_size_mem[TEST_SIZE / sizeof(unsigned int)] = { 100, 200, 300 };

static unsigned int post_test_size_mem[PRE_POST_TEST_SIZE] = { 10, 20, 30 };

static void write_test_mem(void)
{
	int i;

	for (i = 0; i < PRE_POST_TEST_SIZE; i++) {
		pre_test_size_mem[i] = (i + 1) * 10;
		post_test_size_mem[i] = (i + 1);
	}

	for (i = 0; i < sizeof(test_size_mem) / sizeof(unsigned int); i++) {
		test_size_mem[i] = i;
	}

}

static int check_test_mem(int start)
{
	int i;

	for (i = 0; i < PRE_POST_TEST_SIZE; i++) {
		if (start) {
			if (pre_test_size_mem[i] != (i + 1)) {
				return 0;
			}
			if (post_test_size_mem[i] != (i + 1) * 10) {
				return 0;
			}
		} else {
			if (pre_test_size_mem[i] != (i + 1) * 10) {
				return 0;
			}
			if (post_test_size_mem[i] != (i + 1)) {
				return 0;
			}
		}
	}

	for (i = 0; i < sizeof(test_size_mem) / sizeof(unsigned int); i++) {
		if (start) {
			if (i < 3) {
				if (test_size_mem[i] != (i + 1) * 100) {
					return 0;
				}
			} else {
				if (test_size_mem[i] != 0) {
					return 0;
				}
			}
		} else {
			if (test_size_mem[i] != i) {
				return 0;
			}
		}
	}

	return 1;
}

#endif // TEST_SIZE

static void set_failing_group_stage(uint32_t group, uint32_t stage, uint32_t substage)
{
	if (gbl->error_stage == CAL_STAGE_NIL) {
		gbl->error_substage = substage;
		gbl->error_stage = stage;
		gbl->error_group = group;

	}

}

static inline void reg_file_set_group(uint32_t set_group)
{
	// Read the current group and stage
	uint32_t cur_stage_group = IORD_32DIRECT(REG_FILE_CUR_STAGE, 0);

	// Clear the group
	cur_stage_group &= 0x0000FFFF;

	// Set the group
	cur_stage_group |= (set_group << 16);

	// Write the data back
	IOWR_32DIRECT(REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static inline void reg_file_set_stage(uint32_t set_stage)
{
	// Read the current group and stage
	uint32_t cur_stage_group = IORD_32DIRECT(REG_FILE_CUR_STAGE, 0);

	// Clear the stage and substage
	cur_stage_group &= 0xFFFF0000;

	// Set the stage
	cur_stage_group |= (set_stage & 0x000000FF);

	// Write the data back
	IOWR_32DIRECT(REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static inline void reg_file_set_sub_stage(uint32_t set_sub_stage)
{
	// Read the current group and stage
	uint32_t cur_stage_group = IORD_32DIRECT(REG_FILE_CUR_STAGE, 0);

	// Clear the substage
	cur_stage_group &= 0xFFFF00FF;

	// Set the sub stage
	cur_stage_group |= ((set_sub_stage << 8) & 0x0000FF00);

	// Write the data back
	IOWR_32DIRECT(REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static inline uint32_t is_write_group_enabled_for_dm(uint32_t write_group)
{
	return 1;
}

static inline void select_curr_shadow_reg_using_rank(uint32_t rank)
{
}

static void initialize(void)
{
	IOWR_32DIRECT(PHY_MGR_MUX_SEL, 0, 0x3);

	//USER memory clock is not stable we begin initialization

	IOWR_32DIRECT(PHY_MGR_RESET_MEM_STBL, 0, 0);

	//USER calibration status all set to zero

	IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, 0);
	IOWR_32DIRECT(PHY_MGR_CAL_DEBUG_INFO, 0, 0);

	if (((DYNAMIC_CALIB_STEPS) & CALIB_SKIP_ALL) != CALIB_SKIP_ALL) {
		param->read_correct_mask_vg =
		    ((t_btfld) 1 <<
		     (RW_MGR_MEM_DQ_PER_READ_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS)) - 1;
		param->write_correct_mask_vg =
		    ((t_btfld) 1 <<
		     (RW_MGR_MEM_DQ_PER_READ_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS)) - 1;
		param->read_correct_mask = ((t_btfld) 1 << RW_MGR_MEM_DQ_PER_READ_DQS) - 1;
		param->write_correct_mask = ((t_btfld) 1 << RW_MGR_MEM_DQ_PER_WRITE_DQS) - 1;
		param->dm_correct_mask =
		    ((t_btfld) 1 << (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH)) - 1;
	}
}

static void set_rank_and_odt_mask(uint32_t rank, uint32_t odt_mode)
{
	uint32_t odt_mask_0 = 0;
	uint32_t odt_mask_1 = 0;
	uint32_t cs_and_odt_mask;

	if (odt_mode == RW_MGR_ODT_MODE_READ_WRITE) {

		if (LRDIMM) {
			// USER LRDIMMs have two cases to consider: single-slot and dual-slot.
			// USER In single-slot, assert ODT for write only.
			// USER In dual-slot, assert ODT for both slots for write,
			// USER and on the opposite slot only for reads.
			// USER
			// USER Further complicating this is that both DIMMs have either 1 or 2 ODT
			// USER inputs, which do the same thing (only one is actually required).
			if ((RW_MGR_MEM_CHIP_SELECT_WIDTH / RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM) == 1) {
				// USER Single-slot case
				if (RW_MGR_MEM_ODT_WIDTH == 1) {
					// USER Read = 0, Write = 1
					odt_mask_0 = 0x0;
					odt_mask_1 = 0x1;
				} else if (RW_MGR_MEM_ODT_WIDTH == 2) {
					// USER Read = 00, Write = 11
					odt_mask_0 = 0x0;
					odt_mask_1 = 0x3;
				}
			} else if ((RW_MGR_MEM_CHIP_SELECT_WIDTH / RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM)
				   == 2) {
				// USER Dual-slot case
				if (RW_MGR_MEM_ODT_WIDTH == 2) {
					// USER Read: asserted for opposite slot, Write: asserted for both
					odt_mask_0 = (rank < 2) ? 0x2 : 0x1;
					odt_mask_1 = 0x3;
				} else if (RW_MGR_MEM_ODT_WIDTH == 4) {
					// USER Read: asserted for opposite slot, Write: asserted for both
					odt_mask_0 = (rank < 2) ? 0xC : 0x3;
					odt_mask_1 = 0xF;
				}
			}
		} else if (RW_MGR_MEM_NUMBER_OF_RANKS == 1) {
			//USER 1 Rank
			//USER Read: ODT = 0
			//USER Write: ODT = 1
			odt_mask_0 = 0x0;
			odt_mask_1 = 0x1;
		} else if (RW_MGR_MEM_NUMBER_OF_RANKS == 2) {
			//USER 2 Ranks
			if (RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 1 ||
			    (RDIMM && RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 2
			     && RW_MGR_MEM_CHIP_SELECT_WIDTH == 4)) {
				//USER - Dual-Slot , Single-Rank (1 chip-select per DIMM)
				//USER OR
				//USER - RDIMM, 4 total CS (2 CS per DIMM) means 2 DIMM
				//USER Since MEM_NUMBER_OF_RANKS is 2 they are both single rank
				//USER with 2 CS each (special for RDIMM)
				//USER Read: Turn on ODT on the opposite rank
				//USER Write: Turn on ODT on all ranks
				odt_mask_0 = 0x3 & ~(1 << rank);
				odt_mask_1 = 0x3;
			} else {
				//USER - Single-Slot , Dual-rank DIMMs (2 chip-selects per DIMM)
				//USER Read: Turn on ODT off on all ranks
				//USER Write: Turn on ODT on active rank
				odt_mask_0 = 0x0;
				odt_mask_1 = 0x3 & (1 << rank);
			}
		} else {
			//USER 4 Ranks
			//USER Read:
			//USER ----------+-----------------------+
			//USER           |                       |
			//USER           |         ODT           |
			//USER Read From +-----------------------+
			//USER   Rank    |  3  |  2  |  1  |  0  |
			//USER ----------+-----+-----+-----+-----+
			//USER     0     |  0  |  1  |  0  |  0  |
			//USER     1     |  1  |  0  |  0  |  0  |
			//USER     2     |  0  |  0  |  0  |  1  |
			//USER     3     |  0  |  0  |  1  |  0  |
			//USER ----------+-----+-----+-----+-----+
			//USER
			//USER Write:
			//USER ----------+-----------------------+
			//USER           |                       |
			//USER           |         ODT           |
			//USER Write To  +-----------------------+
			//USER   Rank    |  3  |  2  |  1  |  0  |
			//USER ----------+-----+-----+-----+-----+
			//USER     0     |  0  |  1  |  0  |  1  |
			//USER     1     |  1  |  0  |  1  |  0  |
			//USER     2     |  0  |  1  |  0  |  1  |
			//USER     3     |  1  |  0  |  1  |  0  |
			//USER ----------+-----+-----+-----+-----+
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
	    && RW_MGR_MEM_CHIP_SELECT_WIDTH == 4 && RW_MGR_MEM_NUMBER_OF_RANKS == 2) {
		//USER See RDIMM special case above
		cs_and_odt_mask =
		    (0xFF & ~(1 << (2 * rank))) |
		    ((0xFF & odt_mask_0) << 8) | ((0xFF & odt_mask_1) << 16);
	} else if (LRDIMM) {
	} else {
		cs_and_odt_mask =
		    (0xFF & ~(1 << rank)) |
		    ((0xFF & odt_mask_0) << 8) | ((0xFF & odt_mask_1) << 16);
	}

	IOWR_32DIRECT(RW_MGR_SET_CS_AND_ODT_MASK, 0, cs_and_odt_mask);
}

//USER Given a rank, select the set of shadow registers that is responsible for the
//USER delays of such rank, so that subsequent SCC updates will go to those shadow
//USER registers.
static void select_shadow_regs_for_update(uint32_t rank, uint32_t group,
					  uint32_t update_scan_chains)
{
}

static void scc_mgr_initialize(void)
{
	// Clear register file for HPS
	// 16 (2^4) is the size of the full register file in the scc mgr:
	//      RFILE_DEPTH = log2(MEM_DQ_PER_DQS + 1 + MEM_DM_PER_DQS + MEM_IF_READ_DQS_WIDTH - 1) + 1;
	uint32_t i;
	for (i = 0; i < 16; i++) {
		DPRINT(1, "Clearing SCC RFILE index %lu", i);
		IOWR_32DIRECT(SCC_MGR_HHP_RFILE, i << 2, 0);
	}
}

static inline void scc_mgr_set_dqs_bus_in_delay(uint32_t read_group, uint32_t delay)
{
	WRITE_SCC_DQS_IN_DELAY(read_group, delay);

}

static inline void scc_mgr_set_dqs_io_in_delay(uint32_t write_group, uint32_t delay)
{
	WRITE_SCC_DQS_IO_IN_DELAY(delay);

}

static inline void scc_mgr_set_dqs_en_phase(uint32_t read_group, uint32_t phase)
{
	WRITE_SCC_DQS_EN_PHASE(read_group, phase);

}

static void scc_mgr_set_dqs_en_phase_all_ranks(uint32_t read_group, uint32_t phase)
{
	uint32_t r;
	uint32_t update_scan_chains;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		//USER although the h/w doesn't support different phases per shadow register,
		//USER for simplicity our scc manager modeling keeps different phase settings per
		//USER shadow reg, and it's important for us to keep them in sync to match h/w.
		//USER for efficiency, the scan chain update should occur only once to sr0.
		update_scan_chains = (r == 0) ? 1 : 0;

		select_shadow_regs_for_update(r, read_group, update_scan_chains);
		scc_mgr_set_dqs_en_phase(read_group, phase);

		if (update_scan_chains) {
			IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, read_group);
			IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
		}
	}
}

static inline void scc_mgr_set_dqdqs_output_phase(uint32_t write_group, uint32_t phase)
{
	WRITE_SCC_DQDQS_OUT_PHASE(write_group, phase);

}

static void scc_mgr_set_dqdqs_output_phase_all_ranks(uint32_t write_group, uint32_t phase)
{
	uint32_t r;
	uint32_t update_scan_chains;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		//USER although the h/w doesn't support different phases per shadow register,
		//USER for simplicity our scc manager modeling keeps different phase settings per
		//USER shadow reg, and it's important for us to keep them in sync to match h/w.
		//USER for efficiency, the scan chain update should occur only once to sr0.
		update_scan_chains = (r == 0) ? 1 : 0;

		select_shadow_regs_for_update(r, write_group, update_scan_chains);
		scc_mgr_set_dqdqs_output_phase(write_group, phase);

		if (update_scan_chains) {
			IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, write_group);
			IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
		}
	}
}

static inline void scc_mgr_set_dqs_en_delay(uint32_t read_group, uint32_t delay)
{
	WRITE_SCC_DQS_EN_DELAY(read_group, delay);

}

static void scc_mgr_set_dqs_en_delay_all_ranks(uint32_t read_group, uint32_t delay)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		select_shadow_regs_for_update(r, read_group, 0);

		scc_mgr_set_dqs_en_delay(read_group, delay);

		IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, read_group);

		// In shadow register mode, the T11 settings are stored in registers
		// in the core, which are updated by the DQS_ENA signals. Not issuing
		// the SCC_MGR_UPD command allows us to save lots of rank switching
		// overhead, by calling select_shadow_regs_for_update with update_scan_chains
		// set to 0.
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

static void scc_mgr_set_oct_out1_delay(uint32_t write_group, uint32_t delay)
{
	uint32_t read_group;

	// Load the setting in the SCC manager
	// Although OCT affects only write data, the OCT delay is controlled by the DQS logic block
	// which is instantiated once per read group. For protocols where a write group consists
	// of multiple read groups, the setting must be set multiple times.
	for (read_group =
	     write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
	     read_group <
	     (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
	     ++read_group) {

		WRITE_SCC_OCT_OUT1_DELAY(read_group, delay);
	}

}

static void scc_mgr_set_oct_out2_delay(uint32_t write_group, uint32_t delay)
{
	uint32_t read_group;

	// Load the setting in the SCC manager
	// Although OCT affects only write data, the OCT delay is controlled by the DQS logic block
	// which is instantiated once per read group. For protocols where a write group consists
	// of multiple read groups, the setting must be set multiple times.
	for (read_group =
	     write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
	     read_group <
	     (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
	     ++read_group) {

		WRITE_SCC_OCT_OUT2_DELAY(read_group, delay);
	}

}

static inline void scc_mgr_set_dqs_bypass(uint32_t write_group, uint32_t bypass)
{
	// Load the setting in the SCC manager
	WRITE_SCC_DQS_BYPASS(write_group, bypass);
}

static inline void scc_mgr_set_dq_out1_delay(uint32_t write_group, uint32_t dq_in_group,
					     uint32_t delay)
{

	// Load the setting in the SCC manager
	WRITE_SCC_DQ_OUT1_DELAY(dq_in_group, delay);

}

static inline void scc_mgr_set_dq_out2_delay(uint32_t write_group, uint32_t dq_in_group,
					     uint32_t delay)
{

	// Load the setting in the SCC manager
	WRITE_SCC_DQ_OUT2_DELAY(dq_in_group, delay);

}

static inline void scc_mgr_set_dq_in_delay(uint32_t write_group, uint32_t dq_in_group,
					   uint32_t delay)
{

	// Load the setting in the SCC manager
	WRITE_SCC_DQ_IN_DELAY(dq_in_group, delay);

}

static inline void scc_mgr_set_dq_bypass(uint32_t write_group, uint32_t dq_in_group,
					 uint32_t bypass)
{
	// Load the setting in the SCC manager
	WRITE_SCC_DQ_BYPASS(dq_in_group, bypass);
}

static inline void scc_mgr_set_rfifo_mode(uint32_t write_group, uint32_t dq_in_group, uint32_t mode)
{
	// Load the setting in the SCC manager
	WRITE_SCC_RFIFO_MODE(dq_in_group, mode);
}

static inline void scc_mgr_set_hhp_extras(void)
{
	// Load the fixed setting in the SCC manager
	// bits: 0:0 = 1'b1   - dqs bypass
	// bits: 1:1 = 1'b1   - dq bypass
	// bits: 4:2 = 3'b001   - rfifo_mode
	// bits: 6:5 = 2'b01  - rfifo clock_select
	// bits: 7:7 = 1'b0  - separate gating from ungating setting
	// bits: 8:8 = 1'b0  - separate OE from Output delay setting
	uint32_t value = (0 << 8) | (0 << 7) | (1 << 5) | (1 << 2) | (1 << 1) | (1 << 0);
	WRITE_SCC_HHP_EXTRAS(value);
}

static inline void scc_mgr_set_hhp_dqse_map(void)
{
	// Load the fixed setting in the SCC manager
	WRITE_SCC_HHP_DQSE_MAP(0);
}

static inline void scc_mgr_set_dqs_out1_delay(uint32_t write_group, uint32_t delay)
{
	WRITE_SCC_DQS_IO_OUT1_DELAY(delay);

}

static inline void scc_mgr_set_dqs_out2_delay(uint32_t write_group, uint32_t delay)
{
	WRITE_SCC_DQS_IO_OUT2_DELAY(delay);

}

static inline void scc_mgr_set_dm_out1_delay(uint32_t write_group, uint32_t dm, uint32_t delay)
{
	WRITE_SCC_DM_IO_OUT1_DELAY(dm, delay);
}

static inline void scc_mgr_set_dm_out2_delay(uint32_t write_group, uint32_t dm, uint32_t delay)
{
	WRITE_SCC_DM_IO_OUT2_DELAY(dm, delay);
}

static inline void scc_mgr_set_dm_in_delay(uint32_t write_group, uint32_t dm, uint32_t delay)
{
	WRITE_SCC_DM_IO_IN_DELAY(dm, delay);
}

static inline void scc_mgr_set_dm_bypass(uint32_t write_group, uint32_t dm, uint32_t bypass)
{
	// Load the setting in the SCC manager
	WRITE_SCC_DM_BYPASS(dm, bypass);
}

//USER Zero all DQS config
// TODO: maybe rename to scc_mgr_zero_dqs_config (or something)
static void scc_mgr_zero_all(void)
{
	uint32_t i, r;

	//USER Zero all DQS config settings, across all groups and all shadow registers
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		// Strictly speaking this should be called once per group to make
		// sure each group's delay chain is refreshed from the SCC register file,
		// but since we're resetting all delay chains anyway, we can save some
		// runtime by calling select_shadow_regs_for_update just once to switch
		// rank.
		select_shadow_regs_for_update(r, 0, 1);

		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
			// The phases actually don't exist on a per-rank basis, but there's
			// no harm updating them several times, so let's keep the code simple.
			scc_mgr_set_dqs_bus_in_delay(i, IO_DQS_IN_RESERVE);
			scc_mgr_set_dqs_en_phase(i, 0);
			scc_mgr_set_dqs_en_delay(i, 0);
		}

		for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
			scc_mgr_set_dqdqs_output_phase(i, 0);
			// av/cv don't have out2
			scc_mgr_set_oct_out1_delay(i, IO_DQS_OUT_RESERVE);
		}

		//USER multicast to all DQS group enables
		IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, 0xff);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

static void scc_set_bypass_mode(uint32_t write_group, uint32_t mode)
{
	// mode = 0 : Do NOT bypass - Half Rate Mode
	// mode = 1 : Bypass - Full Rate Mode

	// only need to set once for all groups, pins, dq, dqs, dm
	if (write_group == 0) {
		DPRINT(1, "Setting HHP Extras");
		scc_mgr_set_hhp_extras();
		DPRINT(1, "Done Setting HHP Extras");
	}

	//USER multicast to all DQ enables
	IOWR_32DIRECT(SCC_MGR_DQ_ENA, 0, 0xff);

	IOWR_32DIRECT(SCC_MGR_DM_ENA, 0, 0xff);

	//USER update current DQS IO enable
	IOWR_32DIRECT(SCC_MGR_DQS_IO_ENA, 0, 0);

	//USER update the DQS logic
	IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, write_group);

	//USER hit update
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
}

// Moving up to avoid warnings
static void scc_mgr_load_dqs_for_write_group(uint32_t write_group)
{
	uint32_t read_group;

	// Although OCT affects only write data, the OCT delay is controlled by the DQS logic block
	// which is instantiated once per read group. For protocols where a write group consists
	// of multiple read groups, the setting must be scanned multiple times.
	for (read_group =
	     write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
	     read_group <
	     (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
	     ++read_group) {

		IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, read_group);
	}
}

static void scc_mgr_zero_group(uint32_t write_group, uint32_t test_begin, int32_t out_only)
{
	uint32_t i, r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		select_shadow_regs_for_update(r, write_group, 1);

		//USER Zero all DQ config settings
		for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
			scc_mgr_set_dq_out1_delay(write_group, i, 0);
			scc_mgr_set_dq_out2_delay(write_group, i, IO_DQ_OUT_RESERVE);
			if (!out_only) {
				scc_mgr_set_dq_in_delay(write_group, i, 0);
			}
		}

		//USER multicast to all DQ enables
		IOWR_32DIRECT(SCC_MGR_DQ_ENA, 0, 0xff);

		//USER Zero all DM config settings
		for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
			if (!out_only) {
				// Do we really need this?
				scc_mgr_set_dm_in_delay(write_group, i, 0);
			}
			scc_mgr_set_dm_out1_delay(write_group, i, 0);
			scc_mgr_set_dm_out2_delay(write_group, i, IO_DM_OUT_RESERVE);
		}

		//USER multicast to all DM enables
		IOWR_32DIRECT(SCC_MGR_DM_ENA, 0, 0xff);

		//USER zero all DQS io settings
		if (!out_only) {
			scc_mgr_set_dqs_io_in_delay(write_group, 0);
		}
		// av/cv don't have out2
		scc_mgr_set_dqs_out1_delay(write_group, IO_DQS_OUT_RESERVE);
		scc_mgr_set_oct_out1_delay(write_group, IO_DQS_OUT_RESERVE);
		scc_mgr_load_dqs_for_write_group(write_group);

		//USER multicast to all DQS IO enables (only 1)
		IOWR_32DIRECT(SCC_MGR_DQS_IO_ENA, 0, 0);

		//USER hit update to zero everything
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

//USER load up dqs config settings

static void scc_mgr_load_dqs(uint32_t dqs)
{
	IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, dqs);
}

//USER load up dqs io config settings

static void scc_mgr_load_dqs_io(void)
{
	IOWR_32DIRECT(SCC_MGR_DQS_IO_ENA, 0, 0);
}

//USER load up dq config settings

static void scc_mgr_load_dq(uint32_t dq_in_group)
{
	IOWR_32DIRECT(SCC_MGR_DQ_ENA, 0, dq_in_group);
}

//USER load up dm config settings

static void scc_mgr_load_dm(uint32_t dm)
{
	IOWR_32DIRECT(SCC_MGR_DM_ENA, 0, dm);
}

//USER apply and load a particular input delay for the DQ pins in a group
//USER group_bgn is the index of the first dq pin (in the write group)

static void scc_mgr_apply_group_dq_in_delay(uint32_t write_group, uint32_t group_bgn,
					    uint32_t delay)
{
	uint32_t i, p;

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
		scc_mgr_set_dq_in_delay(write_group, p, delay);
		scc_mgr_load_dq(p);
	}
}

//USER apply and load a particular output delay for the DQ pins in a group

static void scc_mgr_apply_group_dq_out1_delay(uint32_t write_group, uint32_t group_bgn,
					      uint32_t delay1)
{
	uint32_t i, p;

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		scc_mgr_set_dq_out1_delay(write_group, i, delay1);
		scc_mgr_load_dq(i);
	}
}

//USER apply and load a particular output delay for the DM pins in a group

static void scc_mgr_apply_group_dm_out1_delay(uint32_t write_group, uint32_t delay1)
{
	uint32_t i;

	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
		scc_mgr_set_dm_out1_delay(write_group, i, delay1);
		scc_mgr_load_dm(i);
	}
}

//USER apply and load delay on both DQS and OCT out1
static void scc_mgr_apply_group_dqs_io_and_oct_out1(uint32_t write_group, uint32_t delay)
{
	scc_mgr_set_dqs_out1_delay(write_group, delay);
	scc_mgr_load_dqs_io();

	scc_mgr_set_oct_out1_delay(write_group, delay);
	scc_mgr_load_dqs_for_write_group(write_group);
}

//USER set delay on both DQS and OCT out1 by incrementally changing
//USER the settings one dtap at a time towards the target value, to avoid
//USER breaking the lock of the DLL/PLL on the memory device.
static void scc_mgr_set_group_dqs_io_and_oct_out1_gradual(uint32_t write_group, uint32_t delay)
{
	uint32_t d = READ_SCC_DQS_IO_OUT1_DELAY();

	while (d > delay) {
		--d;
		scc_mgr_apply_group_dqs_io_and_oct_out1(write_group, d);
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
		if (QDRII) {
			rw_mgr_mem_dll_lock_wait();
		}
	}
	while (d < delay) {
		++d;
		scc_mgr_apply_group_dqs_io_and_oct_out1(write_group, d);
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
		if (QDRII) {
			rw_mgr_mem_dll_lock_wait();
		}
	}
}

//USER apply a delay to the entire output side: DQ, DM, DQS, OCT

static void scc_mgr_apply_group_all_out_delay(uint32_t write_group, uint32_t group_bgn,
					      uint32_t delay)
{
	//USER dq shift

	scc_mgr_apply_group_dq_out1_delay(write_group, group_bgn, delay);

	//USER dm shift

	scc_mgr_apply_group_dm_out1_delay(write_group, delay);

	//USER dqs and oct shift

	scc_mgr_apply_group_dqs_io_and_oct_out1(write_group, delay);
}

//USER apply a delay to the entire output side (DQ, DM, DQS, OCT) and to all ranks
static void scc_mgr_apply_group_all_out_delay_all_ranks(uint32_t write_group, uint32_t group_bgn,
							uint32_t delay)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		select_shadow_regs_for_update(r, write_group, 1);

		scc_mgr_apply_group_all_out_delay(write_group, group_bgn, delay);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

//USER apply a delay to the entire output side: DQ, DM, DQS, OCT

static void scc_mgr_apply_group_all_out_delay_add(uint32_t write_group, uint32_t group_bgn,
						  uint32_t delay)
{
	uint32_t i, p, new_delay;

	//USER dq shift

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {

		new_delay = READ_SCC_DQ_OUT2_DELAY(i);
		new_delay += delay;

		if (new_delay > IO_IO_OUT2_DELAY_MAX) {
			DPRINT(1, "%s(%lu, %lu, %lu) DQ[%lu,%lu]: %lu > %lu => %lu",
			       __func__, write_group, group_bgn, delay, i, p,
			       new_delay, (long unsigned int)IO_IO_OUT2_DELAY_MAX,
			       (long unsigned int)IO_IO_OUT2_DELAY_MAX);
			new_delay = IO_IO_OUT2_DELAY_MAX;
		}

		scc_mgr_set_dq_out2_delay(write_group, i, new_delay);
		scc_mgr_load_dq(i);
	}

	//USER dm shift

	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
		new_delay = READ_SCC_DM_IO_OUT2_DELAY(i);
		new_delay += delay;

		if (new_delay > IO_IO_OUT2_DELAY_MAX) {
			DPRINT(1, "%s(%lu, %lu, %lu) DM[%lu]: %lu > %lu => %lu",
			       __func__, write_group, group_bgn, delay, i,
			       new_delay, (long unsigned int)IO_IO_OUT2_DELAY_MAX,
			       (long unsigned int)IO_IO_OUT2_DELAY_MAX);
			new_delay = IO_IO_OUT2_DELAY_MAX;
		}

		scc_mgr_set_dm_out2_delay(write_group, i, new_delay);
		scc_mgr_load_dm(i);
	}

	//USER dqs shift

	new_delay = READ_SCC_DQS_IO_OUT2_DELAY();
	new_delay += delay;

	if (new_delay > IO_IO_OUT2_DELAY_MAX) {
		DPRINT(1, "%s(%lu, %lu, %lu) DQS: %lu > %d => %d; adding %lu to OUT1",
		       __func__, write_group, group_bgn, delay,
		       new_delay, IO_IO_OUT2_DELAY_MAX, IO_IO_OUT2_DELAY_MAX,
		       new_delay - IO_IO_OUT2_DELAY_MAX);
		scc_mgr_set_dqs_out1_delay(write_group, new_delay - IO_IO_OUT2_DELAY_MAX);
		new_delay = IO_IO_OUT2_DELAY_MAX;
	}

	scc_mgr_set_dqs_out2_delay(write_group, new_delay);
	scc_mgr_load_dqs_io();

	//USER oct shift

	new_delay = READ_SCC_OCT_OUT2_DELAY(write_group);
	new_delay += delay;

	if (new_delay > IO_IO_OUT2_DELAY_MAX) {
		DPRINT(1, "%s(%lu, %lu, %lu) DQS: %lu > %d => %d; adding %lu to OUT1",
		       __func__, write_group, group_bgn, delay,
		       new_delay, IO_IO_OUT2_DELAY_MAX, IO_IO_OUT2_DELAY_MAX,
		       new_delay - IO_IO_OUT2_DELAY_MAX);
		scc_mgr_set_oct_out1_delay(write_group, new_delay - IO_IO_OUT2_DELAY_MAX);
		new_delay = IO_IO_OUT2_DELAY_MAX;
	}

	scc_mgr_set_oct_out2_delay(write_group, new_delay);
	scc_mgr_load_dqs_for_write_group(write_group);
}

//USER apply a delay to the entire output side (DQ, DM, DQS, OCT) and to all ranks
static void scc_mgr_apply_group_all_out_delay_add_all_ranks(uint32_t write_group,
							    uint32_t group_bgn, uint32_t delay)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		select_shadow_regs_for_update(r, write_group, 1);

		scc_mgr_apply_group_all_out_delay_add(write_group, group_bgn, delay);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}
}

static inline void scc_mgr_spread_out2_delay_all_ranks(uint32_t write_group, uint32_t test_bgn)
{
}

// optimization used to recover some slots in ddr3 inst_rom
// could be applied to other protocols if we wanted to
static void set_jump_as_return(void)
{
	// to save space, we replace return with jump to special shared RETURN instruction
	// so we set the counter to large value so that we always jump
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0xFF);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_RETURN);

}

// should always use constants as argument to ensure all computations are performed at compile time
static inline void delay_for_n_mem_clocks(const uint32_t clocks)
{
	uint32_t afi_clocks;
	uint8_t inner;
	uint8_t outer;
	uint16_t c_loop;

	afi_clocks = (clocks + AFI_RATE_RATIO - 1) / AFI_RATE_RATIO;	/* scale (rounding up) to get afi clocks */

	// Note, we don't bother accounting for being off a little bit because of a few extra instructions in outer loops
	// Note, the loops have a test at the end, and do the test before the decrement, and so always perform the loop
	// 1 time more than the counter value
	if (afi_clocks == 0) {
		inner = outer = c_loop = 0;
	} else if (afi_clocks <= 0x100) {
		inner = afi_clocks - 1;
		outer = 0;
		c_loop = 0;
	} else if (afi_clocks <= 0x10000) {
		inner = 0xff;
		outer = (afi_clocks - 1) >> 8;
		c_loop = 0;
	} else {
		inner = 0xff;
		outer = 0xff;
		c_loop = (afi_clocks - 1) >> 16;
	}

	// rom instructions are structured as follows:
	//
	//    IDLE_LOOP2: jnz cntr0, TARGET_A
	//    IDLE_LOOP1: jnz cntr1, TARGET_B
	//                return
	//
	// so, when doing nested loops, TARGET_A is set to IDLE_LOOP2, and TARGET_B is
	// set to IDLE_LOOP2 as well
	//
	// if we have no outer loop, though, then we can use IDLE_LOOP1 only, and set
	// TARGET_B to IDLE_LOOP1 and we skip IDLE_LOOP2 entirely
	//
	// a little confusing, but it helps save precious space in the inst_rom and sequencer rom
	// and keeps the delays more accurate and reduces overhead
	if (afi_clocks <= 0x100) {

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(inner));
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_IDLE_LOOP1);
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_IDLE_LOOP1);

	} else {
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(inner));
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(outer));

		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_IDLE_LOOP2);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_IDLE_LOOP2);

		// hack to get around compiler not being smart enough
		if (afi_clocks <= 0x10000) {
			// only need to run once
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_IDLE_LOOP2);
		} else {
			do {
				IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_IDLE_LOOP2);
			} while (c_loop-- != 0);
		}
	}
}

// should always use constants as argument to ensure all computations are performed at compile time
static inline void delay_for_n_ns(const uint32_t nanoseconds)
{
	delay_for_n_mem_clocks((1000 * nanoseconds) / (1000000 / AFI_CLK_FREQ) * AFI_RATE_RATIO);
}

// Special routine to recover memory device from illegal state after
// ck/dqs relationship is violated.
static inline void recover_mem_device_after_ck_dqs_violation(void)
{
	// Current protocol doesn't require any special recovery
}

static void rw_mgr_rdimm_initialize(void)
{
}

static void rw_mgr_mem_initialize(void)
{
	uint32_t r;

	//USER The reset / cke part of initialization is broadcasted to all ranks
	IOWR_32DIRECT(RW_MGR_SET_CS_AND_ODT_MASK, 0, RW_MGR_RANK_ALL);

	// Here's how you load register for a loop
	//USER Counters are located @ 0x800
	//USER Jump address are located @ 0xC00
	//USER For both, registers 0 to 3 are selected using bits 3 and 2, like in
	//USER 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	// I know this ain't pretty, but Avalon bus throws away the 2 least significant bits

	//USER start with memory RESET activated

	//USER tINIT is typically 200us (but can be adjusted in the GUI)
	//USER The total number of cycles required for this nested counter structure to
	//USER complete is defined by:
	//USER        num_cycles = (CTR2 + 1) * [(CTR1 + 1) * (2 * (CTR0 + 1) + 1) + 1] + 1

	//USER Load counters
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR0_VAL));
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR1_VAL));
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR2_VAL));

	//USER Load jump address
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_RESET_0_CKE_0);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_INIT_RESET_0_CKE_0);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_INIT_RESET_0_CKE_0);

	//USER Execute count instruction
	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_RESET_0_CKE_0);

	//USER indicate that memory is stable
	IOWR_32DIRECT(PHY_MGR_RESET_MEM_STBL, 0, 1);

	//USER transition the RESET to high
	//USER Wait for 500us
	//USER        num_cycles = (CTR2 + 1) * [(CTR1 + 1) * (2 * (CTR0 + 1) + 1) + 1] + 1
	//USER Load counters
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TRESET_CNTR0_VAL));
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TRESET_CNTR1_VAL));
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TRESET_CNTR2_VAL));

	//USER Load jump address
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_RESET_1_CKE_0);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_INIT_RESET_1_CKE_0);
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_INIT_RESET_1_CKE_0);

	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_RESET_1_CKE_0);

	//USER bring up clock enable

	//USER tXRP < 250 ck cycles
	delay_for_n_mem_clocks(250);

	// USER initialize RDIMM buffer so MRS and RZQ Calibrate commands will be
	// USER propagated to discrete memory devices
	rw_mgr_rdimm_initialize();

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER Use Mirror-ed commands for odd ranks if address mirrorring is on
		if ((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_DLL_RESET_MIRR);
		} else {
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_DLL_RESET);
		}

		set_jump_as_return();
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_ZQCL);

		//USER tZQinit = tDLLK = 512 ck cycles
		delay_for_n_mem_clocks(512);
	}
}

static void rw_mgr_mem_dll_lock_wait(void)
{
}

//USER  At the end of calibration we have to program the user settings in, and
//USER  hand off the memory to the user.

static void rw_mgr_mem_handoff(void)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}
		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER precharge all banks ...

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		//USER load up MR settings specified by user

		//USER Use Mirror-ed commands for odd ranks if address mirrorring is on
		if ((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_USER_MIRR);
		} else {
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_USER);
		}
		//USER  need to wait tMOD (12CK or 15ns) time before issuing other commands,
		//USER  but we will have plenty of NIOS cycles before actual handoff so its okay.
	}

}

//USER performs a guaranteed read on the patterns we are going to use during a read test to ensure memory works
static uint32_t rw_mgr_mem_calibrate_read_test_patterns(uint32_t rank_bgn, uint32_t group,
							uint32_t num_tries, t_btfld * bit_chk,
							uint32_t all_ranks)
{
	uint32_t r, vg;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	uint32_t rank_end =
	    all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	*bit_chk = param->read_correct_mask;
	correct_mask_vg = param->read_correct_mask_vg;

	for (r = rank_bgn; r < rank_end; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}
		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		//USER Load up a constant bursts of read commands

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x20);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_GUARANTEED_READ);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x20);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_GUARANTEED_READ_CONT);

		tmp_bit_chk = 0;
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS - 1;; vg--) {
			//USER reset the fifos to get pointers to known state

			IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);
			IOWR_32DIRECT(RW_MGR_RESET_READ_DATAPATH, 0, 0);

			tmp_bit_chk =
			    tmp_bit_chk << (RW_MGR_MEM_DQ_PER_READ_DQS /
					    RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS);

			IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP,
				      ((group * RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS + vg) << 2),
				      __RW_MGR_GUARANTEED_READ);
			tmp_bit_chk =
			    tmp_bit_chk | (correct_mask_vg & ~(IORD_32DIRECT(BASE_RW_MGR, 0)));

			if (vg == 0) {
				break;
			}
		}
		*bit_chk &= tmp_bit_chk;
	}

	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, (group << 2), __RW_MGR_CLEAR_DQS_ENABLE);

	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
	DPRINT(2, "test_load_patterns(%lu,ALL) => (%lu == %lu) => %lu", group, *bit_chk,
	       param->read_correct_mask, (long unsigned int)(*bit_chk == param->read_correct_mask));
	return (*bit_chk == param->read_correct_mask);
}

static uint32_t rw_mgr_mem_calibrate_read_test_patterns_all_ranks(uint32_t group,
								  uint32_t num_tries,
								  t_btfld * bit_chk)
{
	if (rw_mgr_mem_calibrate_read_test_patterns(0, group, num_tries, bit_chk, 1)) {
		return 1;
	} else {
		// case:139851 - if guaranteed read fails, we can retry using different dqs enable phases.
		// It is possible that with the initial phase, dqs enable is asserted/deasserted too close
		// to an dqs edge, truncating the read burst.
		uint32_t p;
		for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++) {
			scc_mgr_set_dqs_en_phase_all_ranks(group, p);
			if (rw_mgr_mem_calibrate_read_test_patterns
			    (0, group, num_tries, bit_chk, 1)) {
				return 1;
			}
		}
		return 0;
	}
}

//USER load up the patterns we are going to use during a read test
static void rw_mgr_mem_calibrate_read_load_patterns(uint32_t rank_bgn, uint32_t all_ranks)
{
	uint32_t r;
	uint32_t rank_end =
	    all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	for (r = rank_bgn; r < rank_end; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}
		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		//USER Load up a constant bursts

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x20);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_GUARANTEED_WRITE_WAIT0);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x20);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_GUARANTEED_WRITE_WAIT1);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0x04);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_GUARANTEED_WRITE_WAIT2);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_3, 0, 0x04);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_GUARANTEED_WRITE_WAIT3);

		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_GUARANTEED_WRITE);
	}

	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
}

static inline void rw_mgr_mem_calibrate_read_load_patterns_all_ranks(void)
{
	rw_mgr_mem_calibrate_read_load_patterns(0, 1);
}

// pe checkout pattern for harden managers
//void pe_checkout_pattern (void)
//{
//    // test RW manager
//
//    // do some reads to check load buffer
//      IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x0);
//      IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_READ_B2B_WAIT1);
//
//      IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x0);
//      IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_READ_B2B_WAIT2);
//
//      IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x0);
//      IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_READ_B2B);
//
//      IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x0);
//      IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_READ_B2B);
//
//      // clear error word
//      IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);
//
//      IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_READ_B2B);
//
//      uint32_t readdata;
//
//      // read error word
//      readdata = IORD_32DIRECT(BASE_RW_MGR, 0);
//
//      // read DI buffer
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 0*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 1*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 2*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 3*4, 0);
//
//      IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x0);
//      IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_READ_B2B_WAIT1);
//
//      IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x0);
//      IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_READ_B2B_WAIT2);
//
//      IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x0);
//      IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_READ_B2B);
//
//      IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x0);
//      IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_READ_B2B);
//
//      // clear error word
//      IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);
//
//      // do read
//      IOWR_32DIRECT (RW_MGR_LOOPBACK_MODE, 0, __RW_MGR_READ_B2B);
//
//      // read error word
//      readdata = IORD_32DIRECT(BASE_RW_MGR, 0);
//
//      // error word should be 0x00
//
//      // read DI buffer
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 0*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 1*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 2*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 3*4, 0);
//
//      // clear error word
//      IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);
//
//      // do dm read
//      IOWR_32DIRECT (RW_MGR_LOOPBACK_MODE, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_WL_1);
//
//      // read error word
//      readdata = IORD_32DIRECT(BASE_RW_MGR, 0);
//
//      // error word should be ff
//
//      // read DI buffer
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 0*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 1*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 2*4, 0);
//      readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 3*4, 0);
//
//      // exit loopback mode
//      IOWR_32DIRECT (BASE_RW_MGR, 0, __RW_MGR_IDLE_LOOP2);
//
//      // start of phy manager access
//
//      readdata = IORD_32DIRECT (PHY_MGR_MAX_RLAT_WIDTH, 0);
//      readdata = IORD_32DIRECT (PHY_MGR_MAX_AFI_WLAT_WIDTH, 0);
//      readdata = IORD_32DIRECT (PHY_MGR_MAX_AFI_RLAT_WIDTH, 0);
//      readdata = IORD_32DIRECT (PHY_MGR_CALIB_SKIP_STEPS, 0);
//      readdata = IORD_32DIRECT (PHY_MGR_CALIB_VFIFO_OFFSET, 0);
//      readdata = IORD_32DIRECT (PHY_MGR_CALIB_LFIFO_OFFSET, 0);
//
//      // start of data manager test
//
//      readdata = IORD_32DIRECT (DATA_MGR_DRAM_CFG         , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_MEM_T_WL         , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_MEM_T_ADD    , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_MEM_T_RL         , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_MEM_T_RFC    , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_MEM_T_REFI   , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_MEM_T_WR         , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_MEM_T_MRD    , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_COL_WIDTH    , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_ROW_WIDTH    , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_BANK_WIDTH   , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_CS_WIDTH         , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_ITF_WIDTH    , 0);
//      readdata = IORD_32DIRECT (DATA_MGR_DVC_WIDTH    , 0);
//
//}

//USER  try a read and see if it returns correct data back. has dummy reads inserted into the mix
//USER  used to align dqs enable. has more thorough checks than the regular read test.

static uint32_t rw_mgr_mem_calibrate_read_test(uint32_t rank_bgn, uint32_t group,
					       uint32_t num_tries, uint32_t all_correct,
					       t_btfld * bit_chk, uint32_t all_groups,
					       uint32_t all_ranks)
{
	uint32_t r, vg;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	uint32_t rank_end =
	    all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);
	uint32_t quick_read_mode = (((STATIC_CALIB_STEPS) & CALIB_SKIP_DELAY_SWEEPS)
				    && ENABLE_SUPER_QUICK_CALIBRATION) || BFM_MODE;

	*bit_chk = param->read_correct_mask;
	correct_mask_vg = param->read_correct_mask_vg;

	for (r = rank_bgn; r < rank_end; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}
		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x10);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_READ_B2B_WAIT1);
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0x10);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_READ_B2B_WAIT2);

		if (quick_read_mode) {
			IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x1);	/* need at least two (1+1) reads to capture failures */
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
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS - 1;; vg--) {
			//USER reset the fifos to get pointers to known state

			IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);
			IOWR_32DIRECT(RW_MGR_RESET_READ_DATAPATH, 0, 0);

			tmp_bit_chk =
			    tmp_bit_chk << (RW_MGR_MEM_DQ_PER_READ_DQS /
					    RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS);

			IOWR_32DIRECT(all_groups ? RW_MGR_RUN_ALL_GROUPS : RW_MGR_RUN_SINGLE_GROUP,
				      ((group * RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS + vg) << 2),
				      __RW_MGR_READ_B2B);
			tmp_bit_chk =
			    tmp_bit_chk | (correct_mask_vg & ~(IORD_32DIRECT(BASE_RW_MGR, 0)));

			if (vg == 0) {
				break;
			}
		}
		*bit_chk &= tmp_bit_chk;
	}

	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, (group << 2), __RW_MGR_CLEAR_DQS_ENABLE);

	if (all_correct) {
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		DPRINT(2, "read_test(%lu,ALL,%lu) => (%lu == %lu) => %lu", group, all_groups,
		       *bit_chk, param->read_correct_mask,
		       (long unsigned int)(*bit_chk == param->read_correct_mask));
		return (*bit_chk == param->read_correct_mask);
	} else {
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		DPRINT(2, "read_test(%lu,ONE,%lu) => (%lu != %lu) => %lu", group, all_groups,
		       *bit_chk, (long unsigned int)0, (long unsigned int)(*bit_chk != 0x00));
		return (*bit_chk != 0x00);
	}
}

static inline uint32_t rw_mgr_mem_calibrate_read_test_all_ranks(uint32_t group, uint32_t num_tries,
								uint32_t all_correct,
								t_btfld * bit_chk,
								uint32_t all_groups)
{
	return rw_mgr_mem_calibrate_read_test(0, group, num_tries, all_correct, bit_chk, all_groups,
					      1);
}

static void rw_mgr_incr_vfifo(uint32_t grp, uint32_t * v)
{
	//USER fiddle with FIFO
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
		// Arria V & Cyclone V have a hard full-rate VFIFO that only has a single incr signal
		IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_FR, 0, grp);
	} else {
		if (!HALF_RATE_MODE || (*v & 1) == 1) {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_HR, 0, grp);
		} else {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_FR, 0, grp);
		}
	}

	(*v)++;
	BFM_INC_VFIFO;
}

//Used in quick cal to properly loop through the duplicated VFIFOs in AV QDRII/RLDRAM
static inline void rw_mgr_incr_vfifo_all(uint32_t grp, uint32_t * v)
{
#if VFIFO_CONTROL_WIDTH_PER_DQS == 1
	rw_mgr_incr_vfifo(grp, v);
#else
	uint32_t i;
	for (i = 0; i < VFIFO_CONTROL_WIDTH_PER_DQS; i++) {
		rw_mgr_incr_vfifo(grp * VFIFO_CONTROL_WIDTH_PER_DQS + i, v);
		if (i != 0) {
			(*v)--;
		}
	}
#endif
}

static void rw_mgr_decr_vfifo(uint32_t grp, uint32_t * v)
{

	uint32_t i;

	for (i = 0; i < VFIFO_SIZE - 1; i++) {
		rw_mgr_incr_vfifo(grp, v);
	}
}

//USER find a good dqs enable to use

#if NEWVERSION_DQSEN

// Navid's version

static uint32_t rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase(uint32_t grp)
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

	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);

	scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);
	scc_mgr_set_dqs_en_phase_all_ranks(grp, 0);

	fail_cnt = 0;

	//USER **************************************************************
	//USER * Step 0 : Determine number of delay taps for each phase tap *

	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}
	dtaps_per_ptap--;
	tmp_delay = 0;

	// VFIFO sweep

	//USER *********************************************************
	//USER * Step 1 : First push vfifo until we get a failing read *
	for (v = 0; v < VFIFO_SIZE;) {
		DPRINT(2, "find_dqs_en_phase: vfifo %lu", BFM_GBL_GET(vfifo_idx));
		test_status =
		    rw_mgr_mem_calibrate_read_test_all_ranks(grp, 1, PASS_ONE_BIT, &bit_chk, 0);
		if (!test_status) {
			fail_cnt++;

			if (fail_cnt == 2) {
				break;
			}
		}
		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (v >= VFIFO_SIZE) {
		//USER no failing read found!! Something must have gone wrong
		DPRINT(2, "find_dqs_en_phase: vfifo failed");
		return 0;
	}

	max_working_cnt = 0;

	//USER ********************************************************
	//USER * step 2: find first working phase, increment in ptaps *
	found_begin = 0;
	work_bgn = 0;
	for (d = 0; d <= dtaps_per_ptap; d++, tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {
		work_bgn = tmp_delay;
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

		for (i = 0; i < VFIFO_SIZE; i++) {
			for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++, work_bgn += IO_DELAY_PER_OPA_TAP) {
				DPRINT(2, "find_dqs_en_phase: begin: vfifo=%lu ptap=%lu dtap=%lu",
				       BFM_GBL_GET(vfifo_idx), p, d);
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				test_status =
				    rw_mgr_mem_calibrate_read_test_all_ranks(grp, 1, PASS_ONE_BIT,
									     &bit_chk, 0);

				if (test_status) {
					max_working_cnt = 1;
					found_begin = 1;
					break;
				}
			}

			if (found_begin) {
				break;
			}

			if (p > IO_DQS_EN_PHASE_MAX) {
				//USER fiddle with FIFO
				rw_mgr_incr_vfifo(grp, &v);
			}
		}

		if (found_begin) {
			break;
		}
	}

	if (i >= VFIFO_SIZE) {
		//USER cannot find working solution
		DPRINT(2, "find_dqs_en_phase: no vfifo/ptap/dtap");
		return 0;
	}

	work_end = work_bgn;

	//USER  If d is 0 then the working window covers a phase tap and we can follow the old procedure
	//USER  otherwise, we've found the beginning, and we need to increment the dtaps until we find the end
	if (d == 0) {
		//USER ********************************************************************
		//USER * step 3a: if we have room, back off by one and increment in dtaps *
		COV(EN_PHASE_PTAP_OVERLAP);

		//USER Special case code for backing up a phase
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}
		tmp_delay = work_bgn - IO_DELAY_PER_OPA_TAP;
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

		found_begin = 0;
		for (d = 0; d <= IO_DQS_EN_DELAY_MAX && tmp_delay < work_bgn;
		     d++, tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {

			DPRINT(2, "find_dqs_en_phase: begin-2: vfifo=%lu ptap=%lu dtap=%lu",
			       BFM_GBL_GET(vfifo_idx), p, d);

			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (rw_mgr_mem_calibrate_read_test_all_ranks
			    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				found_begin = 1;
				work_bgn = tmp_delay;
				break;
			}
		}

		//USER We have found a working dtap before the ptap found above
		if (found_begin == 1) {
			max_working_cnt++;
		}
		//USER Restore VFIFO to old state before we decremented it (if needed)
		p = p + 1;
		if (p > IO_DQS_EN_PHASE_MAX) {
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}

		scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);

		//USER ***********************************************************************************
		//USER * step 4a: go forward from working phase to non working phase, increment in ptaps *
		p = p + 1;
		work_end += IO_DELAY_PER_OPA_TAP;
		if (p > IO_DQS_EN_PHASE_MAX) {
			//USER fiddle with FIFO
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}

		found_end = 0;
		for (; i < VFIFO_SIZE + 1; i++) {
			for (; p <= IO_DQS_EN_PHASE_MAX; p++, work_end += IO_DELAY_PER_OPA_TAP) {
				DPRINT(2, "find_dqs_en_phase: end: vfifo=%lu ptap=%lu dtap=%lu",
				       BFM_GBL_GET(vfifo_idx), p, (long unsigned int)0);
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				if (!rw_mgr_mem_calibrate_read_test_all_ranks
				    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
					found_end = 1;
					break;
				} else {
					max_working_cnt++;
				}
			}

			if (found_end) {
				break;
			}

			if (p > IO_DQS_EN_PHASE_MAX) {
				//USER fiddle with FIFO
				rw_mgr_incr_vfifo(grp, &v);
				p = 0;
			}
		}

		if (i >= VFIFO_SIZE + 1) {
			//USER cannot see edge of failing read
			DPRINT(2, "find_dqs_en_phase: end: failed");
			return 0;
		}
		//USER *********************************************************
		//USER * step 5a:  back off one from last, increment in dtaps  *

		//USER Special case code for backing up a phase
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}

		work_end -= IO_DELAY_PER_OPA_TAP;
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

		//USER * The actual increment of dtaps is done outside of the if/else loop to share code
		d = 0;

		DPRINT(2, "find_dqs_en_phase: found end v/p: vfifo=%lu ptap=%lu",
		       BFM_GBL_GET(vfifo_idx), p);
	} else {

		//USER ********************************************************************
		//USER * step 3-5b:  Find the right edge of the window using delay taps   *
		COV(EN_PHASE_PTAP_NO_OVERLAP);

		DPRINT(2, "find_dqs_en_phase: begin found: vfifo=%lu ptap=%lu dtap=%lu begin=%lu",
		       BFM_GBL_GET(vfifo_idx), p, d, work_bgn);
		BFM_GBL_SET(dqs_enable_left_edge[grp].v, BFM_GBL_GET(vfifo_idx));
		BFM_GBL_SET(dqs_enable_left_edge[grp].p, p);
		BFM_GBL_SET(dqs_enable_left_edge[grp].d, d);
		BFM_GBL_SET(dqs_enable_left_edge[grp].ps, work_bgn);

		work_end = work_bgn;

		//USER * The actual increment of dtaps is done outside of the if/else loop to share code

		//USER Only here to counterbalance a subtract later on which is not needed if this branch
		//USER  of the algorithm is taken
		max_working_cnt++;
	}

	//USER The dtap increment to find the failing edge is done here
	for (; d <= IO_DQS_EN_DELAY_MAX; d++, work_end += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {

		DPRINT(2, "find_dqs_en_phase: end-2: dtap=%lu", d);
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

		if (!rw_mgr_mem_calibrate_read_test_all_ranks(grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		}
	}

	//USER Go back to working dtap
	if (d != 0) {
		work_end -= IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}

	DPRINT(2, "find_dqs_en_phase: found end v/p/d: vfifo=%lu ptap=%lu dtap=%lu end=%lu",
	       BFM_GBL_GET(vfifo_idx), p, d - 1, work_end);
	BFM_GBL_SET(dqs_enable_right_edge[grp].v, BFM_GBL_GET(vfifo_idx));
	BFM_GBL_SET(dqs_enable_right_edge[grp].p, p);
	BFM_GBL_SET(dqs_enable_right_edge[grp].d, d - 1);
	BFM_GBL_SET(dqs_enable_right_edge[grp].ps, work_end);

	if (work_end >= work_bgn) {
		//USER we have a working range
	} else {
		//USER nil range
		DPRINT(2, "find_dqs_en_phase: end-2: failed");
		return 0;
	}

	DPRINT(2, "find_dqs_en_phase: found range [%lu,%lu]", work_bgn, work_end);

	// ***************************************************************
	//USER * We need to calculate the number of dtaps that equal a ptap
	//USER * To do that we'll back up a ptap and re-find the edge of the
	//USER * window using dtaps

	DPRINT(2, "find_dqs_en_phase: calculate dtaps_per_ptap for tracking");

	//USER Special case code for backing up a phase
	if (p == 0) {
		p = IO_DQS_EN_PHASE_MAX;
		rw_mgr_decr_vfifo(grp, &v);
		DPRINT(2, "find_dqs_en_phase: backed up cycle/phase: v=%lu p=%lu",
		       BFM_GBL_GET(vfifo_idx), p);
	} else {
		p = p - 1;
		DPRINT(2, "find_dqs_en_phase: backed up phase only: v=%lu p=%lu",
		       BFM_GBL_GET(vfifo_idx), p);
	}

	scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

	//USER Increase dtap until we first see a passing read (in case the window is smaller than a ptap),
	//USER and then a failing read to mark the edge of the window again

	//USER Find a passing read
	DPRINT(2, "find_dqs_en_phase: find passing read");
	found_passing_read = 0;
	found_failing_read = 0;
	initial_failing_dtap = d;
	for (; d <= IO_DQS_EN_DELAY_MAX; d++) {
		DPRINT(2, "find_dqs_en_phase: testing read d=%lu", d);
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

		if (rw_mgr_mem_calibrate_read_test_all_ranks(grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			found_passing_read = 1;
			break;
		}
	}

	if (found_passing_read) {
		//USER Find a failing read
		DPRINT(2, "find_dqs_en_phase: find failing read");
		for (d = d + 1; d <= IO_DQS_EN_DELAY_MAX; d++) {
			DPRINT(2, "find_dqs_en_phase: testing read d=%lu", d);
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks
			    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				found_failing_read = 1;
				break;
			}
		}
	} else {
		DPRINT(1,
		       "find_dqs_en_phase: failed to calculate dtaps per ptap. Fall back on static value");
	}

	//USER The dynamically calculated dtaps_per_ptap is only valid if we found a passing/failing read
	//USER If we didn't, it means d hit the max (IO_DQS_EN_DELAY_MAX).
	//USER Otherwise, dtaps_per_ptap retains its statically calculated value.
	if (found_passing_read && found_failing_read) {
		dtaps_per_ptap = d - initial_failing_dtap;
	}

	IOWR_32DIRECT(REG_FILE_DTAPS_PER_PTAP, 0, dtaps_per_ptap);

	DPRINT(2, "find_dqs_en_phase: dtaps_per_ptap=%lu - %lu = %lu", d, initial_failing_dtap,
	       dtaps_per_ptap);

	//USER ********************************************
	//USER * step 6:  Find the centre of the window   *

	work_mid = (work_bgn + work_end) / 2;
	tmp_delay = 0;

	DPRINT(2, "work_bgn=%ld work_end=%ld work_mid=%ld", work_bgn, work_end, work_mid);
	//USER Get the middle delay to be less than a VFIFO delay
	for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++, tmp_delay += IO_DELAY_PER_OPA_TAP) ;
	DPRINT(2, "vfifo ptap delay %ld", tmp_delay);
	while (work_mid > tmp_delay)
		work_mid -= tmp_delay;
	DPRINT(2, "new work_mid %ld", work_mid);
	tmp_delay = 0;
	for (p = 0; p <= IO_DQS_EN_PHASE_MAX && tmp_delay < work_mid;
	     p++, tmp_delay += IO_DELAY_PER_OPA_TAP) ;
	tmp_delay -= IO_DELAY_PER_OPA_TAP;
	DPRINT(2, "new p %ld, tmp_delay=%ld", p - 1, tmp_delay);
	for (d = 0; d <= IO_DQS_EN_DELAY_MAX && tmp_delay < work_mid;
	     d++, tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) ;
	DPRINT(2, "new d %ld, tmp_delay=%ld", d, tmp_delay);

	scc_mgr_set_dqs_en_phase_all_ranks(grp, p - 1);
	scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

	//USER push vfifo until we can successfully calibrate. We can do this because
	//USER the largest possible margin in 1 VFIFO cycle

	for (i = 0; i < VFIFO_SIZE; i++) {
		DPRINT(2, "find_dqs_en_phase: center: vfifo=%lu", BFM_GBL_GET(vfifo_idx));
		if (rw_mgr_mem_calibrate_read_test_all_ranks(grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		}
		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		DPRINT(2, "find_dqs_en_phase: center: failed");
		return 0;
	}
	DPRINT(2, "find_dqs_en_phase: center found: vfifo=%li ptap=%lu dtap=%lu",
	       BFM_GBL_GET(vfifo_idx), p - 1, d);
	BFM_GBL_SET(dqs_enable_mid[grp].v, BFM_GBL_GET(vfifo_idx));
	BFM_GBL_SET(dqs_enable_mid[grp].p, p - 1);
	BFM_GBL_SET(dqs_enable_mid[grp].d, d);
	BFM_GBL_SET(dqs_enable_mid[grp].ps, work_mid);
	return 1;
}

#if 0
// Ryan's algorithm

static uint32_t rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase(uint32_t grp)
{
	uint32_t i, d, v, p;
	uint32_t min_working_p, max_working_p, min_working_d, max_working_d, max_working_cnt;
	uint32_t fail_cnt;
	t_btfld bit_chk;
	uint32_t dtaps_per_ptap;
	uint32_t found_begin, found_end;
	uint32_t tmp_delay;

	TRACE_FUNC("%lu", grp);

	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);

	scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);
	scc_mgr_set_dqs_en_phase_all_ranks(grp, 0);

	fail_cnt = 0;

	//USER **************************************************************
	//USER * Step 0 : Determine number of delay taps for each phase tap *

	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}
	dtaps_per_ptap--;

	//USER *********************************************************
	//USER * Step 1 : First push vfifo until we get a failing read *
	for (v = 0; v < VFIFO_SIZE;) {
		if (!rw_mgr_mem_calibrate_read_test_all_ranks(grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			fail_cnt++;

			if (fail_cnt == 2) {
				break;
			}
		}
		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		//USER no failing read found!! Something must have gone wrong
		return 0;
	}

	max_working_cnt = 0;
	min_working_p = 0;

	//USER ********************************************************
	//USER * step 2: find first working phase, increment in ptaps *
	found_begin = 0;
	for (d = 0; d <= dtaps_per_ptap; d++) {
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

		for (i = 0; i < VFIFO_SIZE; i++) {
			for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++) {
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				if (rw_mgr_mem_calibrate_read_test_all_ranks
				    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
					max_working_cnt = 1;
					found_begin = 1;
					break;
				}
			}

			if (found_begin) {
				break;
			}

			if (p > IO_DQS_EN_PHASE_MAX) {
				//USER fiddle with FIFO
				rw_mgr_incr_vfifo(grp, &v);
			}
		}

		if (found_begin) {
			break;
		}
	}

	if (i >= VFIFO_SIZE) {
		//USER cannot find working solution
		return 0;
	}

	min_working_p = p;

	//USER  If d is 0 then the working window covers a phase tap and we can follow the old procedure
	//USER  otherwise, we've found the beginning, and we need to increment the dtaps until we find the end
	if (d == 0) {
		//USER ********************************************************************
		//USER * step 3a: if we have room, back off by one and increment in dtaps *
		min_working_d = 0;

		//USER Special case code for backing up a phase
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

		found_begin = 0;
		for (d = 0; d <= dtaps_per_ptap; d++) {
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (rw_mgr_mem_calibrate_read_test_all_ranks
			    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				found_begin = 1;
				min_working_d = d;
				break;
			}
		}

		//USER We have found a working dtap before the ptap found above
		if (found_begin == 1) {
			min_working_p = p;
			max_working_cnt++;
		}
		//USER Restore VFIFO to old state before we decremented it
		p = p + 1;
		if (p > IO_DQS_EN_PHASE_MAX) {
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}

		scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);

		//USER ***********************************************************************************
		//USER * step 4a: go forward from working phase to non working phase, increment in ptaps *
		p = p + 1;
		if (p > IO_DQS_EN_PHASE_MAX) {
			//USER fiddle with FIFO
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}

		found_end = 0;
		for (; i < VFIFO_SIZE + 1; i++) {
			for (; p <= IO_DQS_EN_PHASE_MAX; p++) {
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				if (!rw_mgr_mem_calibrate_read_test_all_ranks
				    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
					found_end = 1;
					break;
				} else {
					max_working_cnt++;
				}
			}

			if (found_end) {
				break;
			}

			if (p > IO_DQS_EN_PHASE_MAX) {
				//USER fiddle with FIFO
				rw_mgr_incr_vfifo(grp, &v);
				p = 0;
			}
		}

		if (i >= VFIFO_SIZE + 1) {
			//USER cannot see edge of failing read
			return 0;
		}
		//USER *********************************************************
		//USER * step 5a:  back off one from last, increment in dtaps  *
		max_working_d = 0;

		//USER Special case code for backing up a phase
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}

		max_working_p = p;
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

		for (d = 0; d <= IO_DQS_EN_DELAY_MAX; d++) {
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks
			    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				break;
			}
		}

		//USER Go back to working dtap
		if (d != 0) {
			max_working_d = d - 1;
		}

	} else {

		//USER ********************************************************************
		//USER * step 3-5b:  Find the right edge of the window using delay taps   *

		max_working_p = min_working_p;
		min_working_d = d;

		for (; d <= IO_DQS_EN_DELAY_MAX; d++) {
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks
			    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				break;
			}
		}

		//USER Go back to working dtap
		if (d != 0) {
			max_working_d = d - 1;
		}
		//USER Only here to counterbalance a subtract later on which is not needed if this branch
		//USER of the algorithm is taken
		max_working_cnt++;
	}

	//USER ********************************************
	//USER * step 6:  Find the centre of the window   *

	//USER If the number of working phases is even we will step back a phase and find the
	//USER  edge with a larger delay chain tap
	if ((max_working_cnt & 1) == 0) {
		p = min_working_p + (max_working_cnt - 1) / 2;

		//USER Special case code for backing up a phase
		if (max_working_p == 0) {
			max_working_p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			max_working_p = max_working_p - 1;
		}

		scc_mgr_set_dqs_en_phase_all_ranks(grp, max_working_p);

		//USER Code to determine at which dtap we should start searching again for a failure
		//USER If we've moved back such that the max and min p are the same, we should start searching
		//USER from where the window actually exists
		if (max_working_p == min_working_p) {
			d = min_working_d;
		} else {
			d = max_working_d;
		}

		for (; d <= IO_DQS_EN_DELAY_MAX; d++) {
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks
			    (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				break;
			}
		}

		//USER Go back to working dtap
		if (d != 0) {
			max_working_d = d - 1;
		}
	} else {
		p = min_working_p + (max_working_cnt) / 2;
	}

	while (p > IO_DQS_EN_PHASE_MAX) {
		p -= (IO_DQS_EN_PHASE_MAX + 1);
	}

	d = (min_working_d + max_working_d) / 2;

	scc_mgr_set_dqs_en_phase_all_ranks(grp, p);
	scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

	//USER push vfifo until we can successfully calibrate

	for (i = 0; i < VFIFO_SIZE; i++) {
		if (rw_mgr_mem_calibrate_read_test_all_ranks(grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		}
		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		return 0;
	}

	return 1;
}

#endif

#else
// Val's original version

static uint32_t rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase(uint32_t grp)
{
	uint32_t i, j, v, d;
	uint32_t min_working_d, max_working_cnt;
	uint32_t fail_cnt;
	t_btfld bit_chk;
	uint32_t delay_per_ptap_mid;

	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);

	scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);
	scc_mgr_set_dqs_en_phase_all_ranks(grp, 0);

	fail_cnt = 0;

	//USER first push vfifo until we get a failing read
	v = 0;
	for (i = 0; i < VFIFO_SIZE; i++) {
		if (!rw_mgr_mem_calibrate_read_test_all_ranks(grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			fail_cnt++;

			if (fail_cnt == 2) {
				break;
			}
		}
		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (v >= VFIFO_SIZE) {
		//USER no failing read found!! Something must have gone wrong

		return 0;
	}

	max_working_cnt = 0;
	min_working_d = 0;

	for (i = 0; i < VFIFO_SIZE + 1; i++) {
		for (d = 0; d <= IO_DQS_EN_PHASE_MAX; d++) {
			scc_mgr_set_dqs_en_phase_all_ranks(grp, d);

			rw_mgr_mem_calibrate_read_test_all_ranks(grp, NUM_READ_PB_TESTS,
								 PASS_ONE_BIT, &bit_chk, 0);
			if (bit_chk) {
				//USER passing read

				if (max_working_cnt == 0) {
					min_working_d = d;
				}

				max_working_cnt++;
			} else {
				if (max_working_cnt > 0) {
					//USER already have one working value
					break;
				}
			}
		}

		if (d > IO_DQS_EN_PHASE_MAX) {
			//USER fiddle with FIFO
			rw_mgr_incr_vfifo(grp, &v);
		} else {
			//USER found working solution!

			d = min_working_d + (max_working_cnt - 1) / 2;

			while (d > IO_DQS_EN_PHASE_MAX) {
				d -= (IO_DQS_EN_PHASE_MAX + 1);
			}

			break;
		}
	}

	if (i >= VFIFO_SIZE + 1) {
		//USER cannot find working solution or cannot see edge of failing read

		return 0;
	}
	//USER in the case the number of working steps is even, use 50ps taps to further center the window

	if ((max_working_cnt & 1) == 0) {
		delay_per_ptap_mid = IO_DELAY_PER_OPA_TAP / 2;

		//USER increment in 50ps taps until we reach the required amount

		for (i = 0, j = 0; i <= IO_DQS_EN_DELAY_MAX && j < delay_per_ptap_mid;
		     i++, j += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) ;

		scc_mgr_set_dqs_en_delay_all_ranks(grp, i - 1);
	}

	scc_mgr_set_dqs_en_phase_all_ranks(grp, d);

	//USER push vfifo until we can successfully calibrate

	for (i = 0; i < VFIFO_SIZE; i++) {
		if (rw_mgr_mem_calibrate_read_test_all_ranks
		    (grp, NUM_READ_PB_TESTS, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		}
		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		return 0;
	}

	return 1;
}

#endif

// Try rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase across different dq_in_delay values
static inline uint32_t rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay(uint32_t
										      write_group,
										      uint32_t
										      read_group,
										      uint32_t
										      test_bgn)
{
	uint32_t found;
	uint32_t i;
	uint32_t p;
	uint32_t d;
	uint32_t r;

	const uint32_t delay_step = IO_IO_IN_DELAY_MAX / (RW_MGR_MEM_DQ_PER_READ_DQS - 1);

	// try different dq_in_delays since the dq path is shorter than dqs

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		select_shadow_regs_for_update(r, write_group, 1);
		for (i = 0, p = test_bgn, d = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS;
		     i++, p++, d += delay_step) {
			DPRINT(1,
			       "rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay: g=%lu/%lu r=%lu, i=%lu p=%lu d=%lu",
			       write_group, read_group, r, i, p, d);
			scc_mgr_set_dq_in_delay(write_group, p, d);
			scc_mgr_load_dq(p);
		}
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}

	found = rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase(read_group);

	DPRINT(1,
	       "rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay: g=%lu/%lu found=%lu; Reseting delay chain to zero",
	       write_group, read_group, found);

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		select_shadow_regs_for_update(r, write_group, 1);
		for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
			scc_mgr_set_dq_in_delay(write_group, p, 0);
			scc_mgr_load_dq(p);
		}
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}

	return found;
}

//USER per-bit deskew DQ and center

#if NEWVERSION_RDDESKEW

static uint32_t rw_mgr_mem_calibrate_vfifo_center(uint32_t rank_bgn, uint32_t write_group,
						  uint32_t read_group, uint32_t test_bgn,
						  uint32_t use_read_test, uint32_t update_fom)
{
	uint32_t i, p, d, min_index;
	//USER Store these as signed since there are comparisons with signed numbers
	t_btfld bit_chk;
	t_btfld sticky_bit_chk;
	int32_t left_edge[RW_MGR_MEM_DQ_PER_READ_DQS];
	int32_t right_edge[RW_MGR_MEM_DQ_PER_READ_DQS];
	int32_t final_dq[RW_MGR_MEM_DQ_PER_READ_DQS];
	int32_t mid;
	int32_t orig_mid_min, mid_min;
	int32_t new_dqs, start_dqs, start_dqs_en, shift_dq, final_dqs, final_dqs_en;
	int32_t dq_margin, dqs_margin;
	uint32_t stop;

	start_dqs = READ_SCC_DQS_IN_DELAY(read_group);
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		start_dqs_en = READ_SCC_DQS_EN_DELAY(read_group);
	}

	select_curr_shadow_reg_using_rank(rank_bgn);

	//USER per-bit deskew

	//USER set the left and right edge of each bit to an illegal value
	//USER use (IO_IO_IN_DELAY_MAX + 1) as an illegal value
	sticky_bit_chk = 0;
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		left_edge[i] = IO_IO_IN_DELAY_MAX + 1;
		right_edge[i] = IO_IO_IN_DELAY_MAX + 1;
	}

	//USER Search for the left edge of the window for each bit
	for (d = 0; d <= IO_IO_IN_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_in_delay(write_group, test_bgn, d);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		//USER Stop searching when the read test doesn't pass AND when we've seen a passing read on every bit
		if (use_read_test) {
			stop =
			    !rw_mgr_mem_calibrate_read_test(rank_bgn, read_group, NUM_READ_PB_TESTS,
							    PASS_ONE_BIT, &bit_chk, 0, 0);
		} else {
			rw_mgr_mem_calibrate_write_test(rank_bgn, write_group, 0, PASS_ONE_BIT,
							&bit_chk, 0);
			bit_chk =
			    bit_chk >> (RW_MGR_MEM_DQ_PER_READ_DQS *
					(read_group -
					 (write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH /
					  RW_MGR_MEM_IF_WRITE_DQS_WIDTH)));
			stop = (bit_chk == 0);
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->read_correct_mask);
		DPRINT(2, "vfifo_center(left): dtap=%lu => " BTFLD_FMT " == " BTFLD_FMT " && %lu",
		       d, sticky_bit_chk, param->read_correct_mask, stop);

		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
				if (bit_chk & 1) {
					//USER Remember a passing test as the left_edge
					left_edge[i] = d;
				} else {
					//USER If a left edge has not been seen yet, then a future passing test will mark this edge as the right edge
					if (left_edge[i] == IO_IO_IN_DELAY_MAX + 1) {
						right_edge[i] = -(d + 1);
					}
				}
				DPRINT(2,
				       "vfifo_center[l,d=%lu]: bit_chk_test=%d left_edge[%lu]: %ld right_edge[%lu]: %ld",
				       d, (int)(bit_chk & 1), i, left_edge[i], i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	//USER Reset DQ delay chains to 0
	scc_mgr_apply_group_dq_in_delay(write_group, test_bgn, 0);
	sticky_bit_chk = 0;
	for (i = RW_MGR_MEM_DQ_PER_READ_DQS - 1;; i--) {

		DPRINT(2, "vfifo_center: left_edge[%lu]: %ld right_edge[%lu]: %ld", i, left_edge[i],
		       i, right_edge[i]);

		//USER Check for cases where we haven't found the left edge, which makes our assignment of the the
		//USER right edge invalid.  Reset it to the illegal value.
		if ((left_edge[i] == IO_IO_IN_DELAY_MAX + 1)
		    && (right_edge[i] != IO_IO_IN_DELAY_MAX + 1)) {
			right_edge[i] = IO_IO_IN_DELAY_MAX + 1;
			DPRINT(2, "vfifo_center: reset right_edge[%lu]: %ld", i, right_edge[i]);
		}
		//USER Reset sticky bit (except for bits where we have seen both the left and right edge)
		sticky_bit_chk = sticky_bit_chk << 1;
		if ((left_edge[i] != IO_IO_IN_DELAY_MAX + 1)
		    && (right_edge[i] != IO_IO_IN_DELAY_MAX + 1)) {
			sticky_bit_chk = sticky_bit_chk | 1;
		}

		if (i == 0) {
			break;
		}
	}

	//USER Search for the right edge of the window for each bit
	for (d = 0; d <= IO_DQS_IN_DELAY_MAX - start_dqs; d++) {
		scc_mgr_set_dqs_bus_in_delay(read_group, d + start_dqs);
		if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
			uint32_t delay = d + start_dqs_en;
			if (delay > IO_DQS_EN_DELAY_MAX) {
				delay = IO_DQS_EN_DELAY_MAX;
			}
			scc_mgr_set_dqs_en_delay(read_group, delay);
		}
		scc_mgr_load_dqs(read_group);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		//USER Stop searching when the read test doesn't pass AND when we've seen a passing read on every bit
		if (use_read_test) {
			stop =
			    !rw_mgr_mem_calibrate_read_test(rank_bgn, read_group, NUM_READ_PB_TESTS,
							    PASS_ONE_BIT, &bit_chk, 0, 0);
		} else {
			rw_mgr_mem_calibrate_write_test(rank_bgn, write_group, 0, PASS_ONE_BIT,
							&bit_chk, 0);
			bit_chk =
			    bit_chk >> (RW_MGR_MEM_DQ_PER_READ_DQS *
					(read_group -
					 (write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH /
					  RW_MGR_MEM_IF_WRITE_DQS_WIDTH)));
			stop = (bit_chk == 0);
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->read_correct_mask);

		DPRINT(2, "vfifo_center(right): dtap=%lu => " BTFLD_FMT " == " BTFLD_FMT " && %lu",
		       d, sticky_bit_chk, param->read_correct_mask, stop);

		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
				if (bit_chk & 1) {
					//USER Remember a passing test as the right_edge
					right_edge[i] = d;
				} else {
					if (d != 0) {
						//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge
						if (right_edge[i] == IO_IO_IN_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}
					} else {
						//USER d = 0 failed, but it passed when testing the left edge, so it must be marginal, set it to -1
						if (right_edge[i] == IO_IO_IN_DELAY_MAX + 1
						    && left_edge[i] != IO_IO_IN_DELAY_MAX + 1) {
							right_edge[i] = -1;
						}
						//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge
						else if (right_edge[i] == IO_IO_IN_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}

					}
				}

				DPRINT(2,
				       "vfifo_center[r,d=%lu]: bit_chk_test=%d left_edge[%lu]: %ld right_edge[%lu]: %ld",
				       d, (int)(bit_chk & 1), i, left_edge[i], i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	// Store all observed margins

	//USER Check that all bits have a window
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		DPRINT(2, "vfifo_center: left_edge[%lu]: %ld right_edge[%lu]: %ld", i, left_edge[i],
		       i, right_edge[i]);
		BFM_GBL_SET(dq_read_left_edge[read_group][i], left_edge[i]);
		BFM_GBL_SET(dq_read_right_edge[read_group][i], right_edge[i]);
		if ((left_edge[i] == IO_IO_IN_DELAY_MAX + 1)
		    || (right_edge[i] == IO_IO_IN_DELAY_MAX + 1)) {

			//USER Restore delay chain settings before letting the loop in
			//USER rw_mgr_mem_calibrate_vfifo to retry different dqs/ck relationships
			scc_mgr_set_dqs_bus_in_delay(read_group, start_dqs);
			if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
				scc_mgr_set_dqs_en_delay(read_group, start_dqs_en);
			}
			scc_mgr_load_dqs(read_group);
			IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

			DPRINT(1, "vfifo_center: failed to find edge [%lu]: %ld %ld", i,
			       left_edge[i], right_edge[i]);
			if (use_read_test) {
				set_failing_group_stage(read_group * RW_MGR_MEM_DQ_PER_READ_DQS + i,
							CAL_STAGE_VFIFO, CAL_SUBSTAGE_VFIFO_CENTER);
			} else {
				set_failing_group_stage(read_group * RW_MGR_MEM_DQ_PER_READ_DQS + i,
							CAL_STAGE_VFIFO_AFTER_WRITES,
							CAL_SUBSTAGE_VFIFO_CENTER);
			}
			return 0;
		}
	}

	//USER Find middle of window for each DQ bit
	mid_min = left_edge[0] - right_edge[0];
	min_index = 0;
	for (i = 1; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		mid = left_edge[i] - right_edge[i];
		if (mid < mid_min) {
			mid_min = mid;
			min_index = i;
		}
	}

	//USER  -mid_min/2 represents the amount that we need to move DQS.  If mid_min is odd and positive we'll need to add one to
	//USER make sure the rounding in further calculations is correct (always bias to the right), so just add 1 for all positive values
	if (mid_min > 0) {
		mid_min++;
	}
	mid_min = mid_min / 2;

	DPRINT(1, "vfifo_center: mid_min=%ld (index=%lu)", mid_min, min_index);

	//USER Determine the amount we can change DQS (which is -mid_min)
	orig_mid_min = mid_min;
	new_dqs = start_dqs - mid_min;
	if (new_dqs > IO_DQS_IN_DELAY_MAX) {
		new_dqs = IO_DQS_IN_DELAY_MAX;
	} else if (new_dqs < 0) {
		new_dqs = 0;
	}
	mid_min = start_dqs - new_dqs;
	DPRINT(1, "vfifo_center: new mid_min=%ld new_dqs=%ld", mid_min, new_dqs);

	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		if (start_dqs_en - mid_min > IO_DQS_EN_DELAY_MAX) {
			mid_min += start_dqs_en - mid_min - IO_DQS_EN_DELAY_MAX;
		} else if (start_dqs_en - mid_min < 0) {
			mid_min += start_dqs_en - mid_min;
		}
	}
	new_dqs = start_dqs - mid_min;

	DPRINT(1, "vfifo_center: start_dqs=%ld start_dqs_en=%ld new_dqs=%ld mid_min=%ld",
	       start_dqs, IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS ? start_dqs_en : -1, new_dqs, mid_min);

	//USER Initialize data for export structures
	dqs_margin = IO_IO_IN_DELAY_MAX + 1;
	dq_margin = IO_IO_IN_DELAY_MAX + 1;

	//USER add delay to bring centre of all DQ windows to the same "level"
	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
		//USER Use values before divide by 2 to reduce round off error
		shift_dq =
		    (left_edge[i] - right_edge[i] -
		     (left_edge[min_index] - right_edge[min_index])) / 2 + (orig_mid_min - mid_min);

		DPRINT(2, "vfifo_center: before: shift_dq[%lu]=%ld", i, shift_dq);

		if (shift_dq + (int32_t) READ_SCC_DQ_IN_DELAY(p) > (int32_t) IO_IO_IN_DELAY_MAX) {
			shift_dq = (int32_t) IO_IO_IN_DELAY_MAX - READ_SCC_DQ_IN_DELAY(i);
		} else if (shift_dq + (int32_t) READ_SCC_DQ_IN_DELAY(p) < 0) {
			shift_dq = -(int32_t) READ_SCC_DQ_IN_DELAY(p);
		}
		DPRINT(2, "vfifo_center: after: shift_dq[%lu]=%ld", i, shift_dq);
		final_dq[i] = READ_SCC_DQ_IN_DELAY(p) + shift_dq;
		scc_mgr_set_dq_in_delay(write_group, p, final_dq[i]);
		scc_mgr_load_dq(p);

		DPRINT(2, "vfifo_center: margin[%lu]=[%ld,%ld]", i,
		       left_edge[i] - shift_dq + (-mid_min), right_edge[i] + shift_dq - (-mid_min));
		//USER To determine values for export structures
		if (left_edge[i] - shift_dq + (-mid_min) < dq_margin) {
			dq_margin = left_edge[i] - shift_dq + (-mid_min);
		}
		if (right_edge[i] + shift_dq - (-mid_min) < dqs_margin) {
			dqs_margin = right_edge[i] + shift_dq - (-mid_min);
		}
	}

	final_dqs = new_dqs;
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		final_dqs_en = start_dqs_en - mid_min;
	}
	//USER Move DQS-en
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		scc_mgr_set_dqs_en_delay(read_group, final_dqs_en);
		scc_mgr_load_dqs(read_group);
	}
	//USER Move DQS
	scc_mgr_set_dqs_bus_in_delay(read_group, final_dqs);
	scc_mgr_load_dqs(read_group);

	if (update_fom) {
		//USER Export values
		gbl->fom_in +=
		    (dq_margin +
		     dqs_margin) / (RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	}

	DPRINT(2, "vfifo_center: dq_margin=%ld dqs_margin=%ld", dq_margin, dqs_margin);

	//USER Do not remove this line as it makes sure all of our decisions have been applied
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	return (dq_margin >= 0) && (dqs_margin >= 0);
}

#else

static uint32_t rw_mgr_mem_calibrate_vfifo_center(uint32_t rank_bgn, uint32_t grp,
						  uint32_t test_bgn, uint32_t use_read_test)
{
	uint32_t i, p, d;
	uint32_t mid;
	t_btfld bit_chk;
	uint32_t max_working_dq[RW_MGR_MEM_DQ_PER_READ_DQS];
	uint32_t dq_margin, dqs_margin;
	uint32_t start_dqs;

	//USER per-bit deskew.
	//USER start of the per-bit sweep with the minimum working delay setting for
	//USER all bits.

	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		max_working_dq[i] = 0;
	}

	for (d = 1; d <= IO_IO_IN_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_in_delay(write_group, test_bgn, d);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		if (!rw_mgr_mem_calibrate_read_test
		    (rank_bgn, grp, NUM_READ_PB_TESTS, PASS_ONE_BIT, &bit_chk, 0, 0)) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
				if (bit_chk & 1) {
					max_working_dq[i] = d;
				}
				bit_chk = bit_chk >> 1;
			}
		}
	}

	//USER determine minimum working value for DQ

	dq_margin = IO_IO_IN_DELAY_MAX;

	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		if (max_working_dq[i] < dq_margin) {
			dq_margin = max_working_dq[i];
		}
	}

	//USER add delay to bring all DQ windows to the same "level"

	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
		if (max_working_dq[i] > dq_margin) {
			scc_mgr_set_dq_in_delay(write_group, i, max_working_dq[i] - dq_margin);
		} else {
			scc_mgr_set_dq_in_delay(write_group, i, 0);
		}

		scc_mgr_load_dq(p, p);
	}

	//USER sweep DQS window, may potentially have more window due to per-bit-deskew that was done
	//USER in the previous step.

	start_dqs = READ_SCC_DQS_IN_DELAY(grp);

	for (d = start_dqs + 1; d <= IO_DQS_IN_DELAY_MAX; d++) {
		scc_mgr_set_dqs_bus_in_delay(grp, d);
		scc_mgr_load_dqs(grp);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		if (!rw_mgr_mem_calibrate_read_test
		    (rank_bgn, grp, NUM_READ_TESTS, PASS_ALL_BITS, &bit_chk, 0, 0)) {
			break;
		}
	}

	scc_mgr_set_dqs_bus_in_delay(grp, start_dqs);

	//USER margin on the DQS pin

	dqs_margin = d - start_dqs - 1;

	//USER find mid point, +1 so that we don't go crazy pushing DQ

	mid = (dq_margin + dqs_margin + 1) / 2;

	gbl->fom_in += dq_margin + dqs_margin;
//      TCLRPT_SET(debug_summary_report->fom_in, debug_summary_report->fom_in + (dq_margin + dqs_margin));
//      TCLRPT_SET(debug_cal_report->cal_status_per_group[grp].fom_in, (dq_margin + dqs_margin));

	//USER center DQS ... if the headroom is setup properly we shouldn't need to

	if (dqs_margin > mid) {
		scc_mgr_set_dqs_bus_in_delay(grp, READ_SCC_DQS_IN_DELAY(grp) + dqs_margin - mid);

		if (DDRX) {
			uint32_t delay = READ_SCC_DQS_EN_DELAY(grp) + dqs_margin - mid;

			if (delay > IO_DQS_EN_DELAY_MAX) {
				delay = IO_DQS_EN_DELAY_MAX;
			}

			scc_mgr_set_dqs_en_delay(grp, delay);
		}
	}

	scc_mgr_load_dqs(grp);

	//USER center DQ

	if (dq_margin > mid) {
		for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
			scc_mgr_set_dq_in_delay(write_group, i,
						READ_SCC_DQ_IN_DELAY(i) + dq_margin - mid);
			scc_mgr_load_dq(p, p);
		}

		dqs_margin += dq_margin - mid;
		dq_margin -= dq_margin - mid;
	}

	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

	return (dq_margin + dqs_margin) > 0;
}

#endif

//USER calibrate the read valid prediction FIFO.
//USER
//USER  - read valid prediction will consist of finding a good DQS enable phase, DQS enable delay, DQS input phase, and DQS input delay.
//USER  - we also do a per-bit deskew on the DQ lines.

#if NEWVERSION_GW

//USER VFIFO Calibration -- Full Calibration
static uint32_t rw_mgr_mem_calibrate_vfifo(uint32_t read_group, uint32_t test_bgn)
{
	uint32_t p, d, rank_bgn, sr;
	uint32_t dtaps_per_ptap;
	uint32_t tmp_delay;
	t_btfld bit_chk;
	uint32_t grp_calibrated;
	uint32_t write_group, write_test_bgn;
	uint32_t failed_substage;
	uint32_t dqs_in_dtaps, orig_start_dqs;

	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_VFIFO);

	if (DDRX) {
		write_group = read_group;
		write_test_bgn = test_bgn;
	} else {
		write_group =
		    read_group / (RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
		write_test_bgn = read_group * RW_MGR_MEM_DQ_PER_READ_DQS;
	}

	// USER Determine number of delay taps for each phase tap
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	if (!QDRII) {
		while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
			dtaps_per_ptap++;
			tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
		}
		dtaps_per_ptap--;
		tmp_delay = 0;
	}
	//USER update info for sims

	reg_file_set_group(read_group);

	grp_calibrated = 0;

	reg_file_set_sub_stage(CAL_SUBSTAGE_GUARANTEED_READ);
	failed_substage = CAL_SUBSTAGE_GUARANTEED_READ;

	for (d = 0; d <= dtaps_per_ptap && grp_calibrated == 0; d += 2) {

		if (DDRX || RLDRAMX) {
			// In RLDRAMX we may be messing the delay of pins in the same write group but outside of
			// the current read group, but that's ok because we haven't calibrated the output side yet.
			if (d > 0) {
				scc_mgr_apply_group_all_out_delay_add_all_ranks(write_group,
										write_test_bgn, d);
			}
		}

		for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX && grp_calibrated == 0; p++) {
			//USER set a particular dqdqs phase
			if (DDRX) {
				scc_mgr_set_dqdqs_output_phase_all_ranks(read_group, p);
			}
			//USER Previous iteration may have failed as a result of ck/dqs or ck/dk violation,
			//USER in which case the device may require special recovery.
			if (DDRX || RLDRAMX) {
				if (d != 0 || p != 0) {
					recover_mem_device_after_ck_dqs_violation();
				}
			}

			DPRINT(1, "calibrate_vfifo: g=%lu p=%lu d=%lu", read_group, p, d);
			BFM_GBL_SET(gwrite_pos[read_group].p, p);
			BFM_GBL_SET(gwrite_pos[read_group].d, d);

			//USER Load up the patterns used by read calibration using current DQDQS phase

			rw_mgr_mem_calibrate_read_load_patterns_all_ranks();

			if (!(gbl->phy_debug_mode_flags & PHY_DEBUG_DISABLE_GUARANTEED_READ)) {
				if (!rw_mgr_mem_calibrate_read_test_patterns_all_ranks
				    (read_group, 1, &bit_chk)) {
					DPRINT(1, "Guaranteed read test failed: g=%lu p=%lu d=%lu",
					       read_group, p, d);
					break;
				}
			}
			// Loop over different DQS in delay chains for the purpose of DQS Enable calibration finding one bit working
			orig_start_dqs = READ_SCC_DQS_IN_DELAY(read_group);
			for (dqs_in_dtaps = orig_start_dqs;
			     dqs_in_dtaps <= IO_DQS_IN_DELAY_MAX && grp_calibrated == 0;
			     dqs_in_dtaps++) {

				for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS;
				     rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {

					if (!param->skip_shadow_regs[sr]) {

						//USER Select shadow register set
						select_shadow_regs_for_update(rank_bgn, read_group,
									      1);

						WRITE_SCC_DQS_IN_DELAY(read_group, dqs_in_dtaps);
						scc_mgr_load_dqs(read_group);
						IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
					}
				}

// case:56390
				grp_calibrated = 1;
				if (rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay
				    (write_group, read_group, test_bgn)) {
					// USER Read per-bit deskew can be done on a per shadow register basis
					for (rank_bgn = 0, sr = 0;
					     rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS;
					     rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {
						//USER Determine if this set of ranks should be skipped entirely
						if (!param->skip_shadow_regs[sr]) {

							//USER Select shadow register set
							select_shadow_regs_for_update(rank_bgn,
										      read_group,
										      1);

							// Before doing read deskew, set DQS in back to the reserve value
							WRITE_SCC_DQS_IN_DELAY(read_group,
									       orig_start_dqs);
							scc_mgr_load_dqs(read_group);
							IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

							// If doing read after write calibration, do not update FOM now - do it then
							if (!rw_mgr_mem_calibrate_vfifo_center
							    (rank_bgn, write_group, read_group,
							     test_bgn, 1, 0)) {
								grp_calibrated = 0;
								failed_substage =
								    CAL_SUBSTAGE_VFIFO_CENTER;
							}
						}
					}
				} else {
					grp_calibrated = 0;
					failed_substage = CAL_SUBSTAGE_DQS_EN_PHASE;
				}
			}

		}
	}

	if (grp_calibrated == 0) {
		set_failing_group_stage(write_group, CAL_STAGE_VFIFO, failed_substage);

		return 0;
	}
	//USER Reset the delay chains back to zero if they have moved > 1 (check for > 1 because loop will increase d even when pass in first case)
	if (DDRX || RLDRAMII) {
		if (d > 2) {
			scc_mgr_zero_group(write_group, write_test_bgn, 1);
		}
	}

	return 1;
}

#else

//USER VFIFO Calibration -- Full Calibration
static uint32_t rw_mgr_mem_calibrate_vfifo(uint32_t g, uint32_t test_bgn)
{
	uint32_t p, rank_bgn, sr;
	uint32_t grp_calibrated;
	uint32_t failed_substage;

	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_VFIFO);

	reg_file_set_sub_stage(CAL_SUBSTAGE_GUARANTEED_READ);

	failed_substage = CAL_SUBSTAGE_GUARANTEED_READ;

	//USER update info for sims

	reg_file_set_group(g);

	grp_calibrated = 0;

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX && grp_calibrated == 0; p++) {
		//USER set a particular dqdqs phase
		if (DDRX) {
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);
		}
		//USER Load up the patterns used by read calibration using current DQDQS phase

		rw_mgr_mem_calibrate_read_load_patterns_all_ranks();
		if (!(gbl->phy_debug_mode_flags & PHY_DEBUG_DISABLE_GUARANTEED_READ)) {
			if (!rw_mgr_mem_calibrate_read_test_patterns_all_ranks
			    (read_group, 1, &bit_chk)) {
				break;
			}
		}

		grp_calibrated = 1;
		if (rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay(g, g, test_bgn)) {
			// USER Read per-bit deskew can be done on a per shadow register basis
			for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS;
			     rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {

				//USER Determine if this set of ranks should be skipped entirely
				if (!param->skip_shadow_regs[sr]) {

					//USER Select shadow register set
					select_shadow_regs_for_update(rank_bgn, read_group, 1);

					if (!rw_mgr_mem_calibrate_vfifo_center
					    (rank_bgn, g, test_bgn, 1)) {
						grp_calibrated = 0;
						failed_substage = CAL_SUBSTAGE_VFIFO_CENTER;
					}
				}
			}
		} else {
			grp_calibrated = 0;
			failed_substage = CAL_SUBSTAGE_DQS_EN_PHASE;
		}
	}

	if (grp_calibrated == 0) {
		set_failing_group_stage(g, CAL_STAGE_VFIFO, failed_substage);
		return 0;
	}

	return 1;
}

#endif

//USER VFIFO Calibration -- Read Deskew Calibration after write deskew
static uint32_t rw_mgr_mem_calibrate_vfifo_end(uint32_t read_group, uint32_t test_bgn)
{
	uint32_t rank_bgn, sr;
	uint32_t grp_calibrated;
	uint32_t write_group;

	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_VFIFO_AFTER_WRITES);
	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);

	if (DDRX) {
		write_group = read_group;
	} else {
		write_group =
		    read_group / (RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	}

	//USER update info for sims
	reg_file_set_group(read_group);

	grp_calibrated = 1;
	// USER Read per-bit deskew can be done on a per shadow register basis
	for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS;
	     rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {

		//USER Determine if this set of ranks should be skipped entirely
		if (!param->skip_shadow_regs[sr]) {

			//USER Select shadow register set
			select_shadow_regs_for_update(rank_bgn, read_group, 1);

			// This is the last calibration round, update FOM here
			if (!rw_mgr_mem_calibrate_vfifo_center
			    (rank_bgn, write_group, read_group, test_bgn, 0, 1)) {
				grp_calibrated = 0;
			}
		}
	}

	if (grp_calibrated == 0) {
		set_failing_group_stage(write_group, CAL_STAGE_VFIFO_AFTER_WRITES,
					CAL_SUBSTAGE_VFIFO_CENTER);
		return 0;
	}

	return 1;
}

//USER Calibrate LFIFO to find smallest read latency

static uint32_t rw_mgr_mem_calibrate_lfifo(void)
{
	uint32_t found_one;
	t_btfld bit_chk;

	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_LFIFO);
	reg_file_set_sub_stage(CAL_SUBSTAGE_READ_LATENCY);

	//USER Load up the patterns used by read calibration for all ranks

	rw_mgr_mem_calibrate_read_load_patterns_all_ranks();

	found_one = 0;

	do {
		IOWR_32DIRECT(PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
		DPRINT(2, "lfifo: read_lat=%lu", gbl->curr_read_lat);

		if (!rw_mgr_mem_calibrate_read_test_all_ranks
		    (0, NUM_READ_TESTS, PASS_ALL_BITS, &bit_chk, 1)) {
			break;
		}

		found_one = 1;

		//USER reduce read latency and see if things are working
		//USER correctly

		gbl->curr_read_lat--;
	} while (gbl->curr_read_lat > 0);

	//USER reset the fifos to get pointers to known state

	IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);

	if (found_one) {
		//USER add a fudge factor to the read latency that was determined
		gbl->curr_read_lat += 2;
		IOWR_32DIRECT(PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);

		DPRINT(2, "lfifo: success: using read_lat=%lu", gbl->curr_read_lat);

		return 1;
	} else {
		set_failing_group_stage(0xff, CAL_STAGE_LFIFO, CAL_SUBSTAGE_READ_LATENCY);

		DPRINT(2, "lfifo: failed at initial read_lat=%lu", gbl->curr_read_lat);

		return 0;
	}
}

//USER issue write test command.
//USER two variants are provided. one that just tests a write pattern and another that
//USER tests datamask functionality.

static void rw_mgr_mem_calibrate_write_test_issue(uint32_t group, uint32_t test_dm)
{
	uint32_t mcc_instruction;
	uint32_t quick_write_mode = (((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES)
				     && ENABLE_SUPER_QUICK_CALIBRATION) || BFM_MODE;
	uint32_t rw_wl_nop_cycles;

	//USER Set counter and jump addresses for the right
	//USER number of NOP cycles.
	//USER The number of supported NOP cycles can range from -1 to infinity
	//USER Three different cases are handled:
	//USER
	//USER 1. For a number of NOP cycles greater than 0, the RW Mgr looping
	//USER    mechanism will be used to insert the right number of NOPs
	//USER
	//USER 2. For a number of NOP cycles equals to 0, the micro-instruction
	//USER    issuing the write command will jump straight to the micro-instruction
	//USER    that turns on DQS (for DDRx), or outputs write data (for RLD), skipping
	//USER    the NOP micro-instruction all together
	//USER
	//USER 3. A number of NOP cycles equal to -1 indicates that DQS must be turned
	//USER    on in the same micro-instruction that issues the write command. Then we need
	//USER    to directly jump to the micro-instruction that sends out the data
	//USER
	//USER NOTE: Implementing this mechanism uses 2 RW Mgr jump-counters (2 and 3). One
	//USER       jump-counter (0) is used to perform multiple write-read operations.
	//USER       one counter left to issue this command in "multiple-group" mode.

	rw_wl_nop_cycles = gbl->rw_wl_nop_cycles;

	if (rw_wl_nop_cycles == -1) {
		//USER CNTR 2 - We want to execute the special write operation that
		//USER turns on DQS right away and then skip directly to the instruction that
		//USER sends out the data. We set the counter to a large number so that the
		//USER jump is always taken
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0xFF);

		//USER CNTR 3 - Not used
		if (test_dm) {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0_WL_1;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0,
				      __RW_MGR_LFSR_WR_RD_DM_BANK_0_DATA);
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_NOP);
		} else {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0_WL_1;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_BANK_0_DATA);
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_LFSR_WR_RD_BANK_0_NOP);
		}

	} else if (rw_wl_nop_cycles == 0) {
		//USER CNTR 2 - We want to skip the NOP operation and go straight to
		//USER the DQS enable instruction. We set the counter to a large number so that the
		//USER jump is always taken
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0xFF);

		//USER CNTR 3 - Not used
		if (test_dm) {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_DQS);
		} else {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_BANK_0_DQS);
		}

	} else {
		//USER CNTR 2 - In this case we want to execute the next instruction and NOT
		//USER take the jump. So we set the counter to 0. The jump address doesn't count
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_2, 0, 0x0);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_2, 0, 0x0);

		//USER CNTR 3 - Set the nop counter to the number of cycles we need to loop for, minus 1
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_3, 0, rw_wl_nop_cycles - 1);
		if (test_dm) {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_NOP);
		} else {
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0;
			IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_LFSR_WR_RD_BANK_0_NOP);
		}
	}

	IOWR_32DIRECT(RW_MGR_RESET_READ_DATAPATH, 0, 0);

	if (quick_write_mode) {
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x08);
	} else {
		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x40);
	}
	IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, mcc_instruction);

	//USER CNTR 1 - This is used to ensure enough time elapses for read data to come back.
	IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x30);

	if (test_dm) {
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_WAIT);
	} else {
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_LFSR_WR_RD_BANK_0_WAIT);
	}

	IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, (group << 2), mcc_instruction);

}

//USER Test writes, can check for a single bit pass or multiple bit pass

static uint32_t rw_mgr_mem_calibrate_write_test(uint32_t rank_bgn, uint32_t write_group,
						uint32_t use_dm, uint32_t all_correct,
						t_btfld * bit_chk, uint32_t all_ranks)
{
	uint32_t r;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	uint32_t vg;
	uint32_t rank_end =
	    all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	*bit_chk = param->write_correct_mask;
	correct_mask_vg = param->write_correct_mask_vg;

	for (r = rank_bgn; r < rank_end; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}
		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		tmp_bit_chk = 0;
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS - 1;; vg--) {

			//USER reset the fifos to get pointers to known state
			IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);

			tmp_bit_chk =
			    tmp_bit_chk << (RW_MGR_MEM_DQ_PER_WRITE_DQS /
					    RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS);
			rw_mgr_mem_calibrate_write_test_issue(write_group *
							      RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS
							      + vg, use_dm);

			tmp_bit_chk =
			    tmp_bit_chk | (correct_mask_vg & ~(IORD_32DIRECT(BASE_RW_MGR, 0)));
			DPRINT(2,
			       "write_test(%lu,%lu,%lu) :[%lu,%lu] " BTFLD_FMT " & ~%x => "
			       BTFLD_FMT " => " BTFLD_FMT, write_group, use_dm, all_correct, r, vg,
			       correct_mask_vg, IORD_32DIRECT(BASE_RW_MGR, 0),
			       correct_mask_vg & ~IORD_32DIRECT(BASE_RW_MGR, 0), tmp_bit_chk);

			if (vg == 0) {
				break;
			}
		}
		*bit_chk &= tmp_bit_chk;
	}

	if (all_correct) {
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		DPRINT(2, "write_test(%lu,%lu,ALL) : " BTFLD_FMT " == " BTFLD_FMT " => %lu",
		       write_group, use_dm, *bit_chk, param->write_correct_mask,
		       (long unsigned int)(*bit_chk == param->write_correct_mask));
		return (*bit_chk == param->write_correct_mask);
	} else {
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		DPRINT(2, "write_test(%lu,%lu,ONE) : " BTFLD_FMT " != " BTFLD_FMT " => %lu",
		       write_group, use_dm, *bit_chk, (long unsigned int)0,
		       (long unsigned int)(*bit_chk != 0));
		return (*bit_chk != 0x00);
	}
}

static inline uint32_t rw_mgr_mem_calibrate_write_test_all_ranks(uint32_t write_group,
								 uint32_t use_dm,
								 uint32_t all_correct,
								 t_btfld * bit_chk)
{
	return rw_mgr_mem_calibrate_write_test(0, write_group, use_dm, all_correct, bit_chk, 1);
}

//USER level the write operations

#if NEWVERSION_WL

//USER Write Levelling -- Full Calibration
static uint32_t rw_mgr_mem_calibrate_wlevel(uint32_t g, uint32_t test_bgn)
{
	uint32_t p, d;

	uint32_t num_additional_fr_cycles = 0;

	t_btfld bit_chk;
	uint32_t work_bgn, work_end, work_mid;
	uint32_t tmp_delay;
	uint32_t found_begin;
	uint32_t dtaps_per_ptap;

	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);

	//USER maximum phases for the sweep

	dtaps_per_ptap = IORD_32DIRECT(REG_FILE_DTAPS_PER_PTAP, 0);

	//USER starting phases

	//USER update info for sims

	reg_file_set_group(g);

	//USER starting and end range where writes work

	scc_mgr_spread_out2_delay_all_ranks(g, test_bgn);

	work_bgn = 0;
	work_end = 0;

	//USER step 1: find first working phase, increment in ptaps, and then in dtaps if ptaps doesn't find a working phase
	found_begin = 0;
	tmp_delay = 0;
	for (d = 0; d <= dtaps_per_ptap; d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP) {
		scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, d);

		work_bgn = tmp_delay;

		for (p = 0;
		     p <= IO_DQDQS_OUT_PHASE_MAX + num_additional_fr_cycles * IO_DLL_CHAIN_LENGTH;
		     p++, work_bgn += IO_DELAY_PER_OPA_TAP) {
			DPRINT(2, "wlevel: begin-1: p=%lu d=%lu", p, d);
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

			if (rw_mgr_mem_calibrate_write_test_all_ranks(g, 0, PASS_ONE_BIT, &bit_chk)) {
				found_begin = 1;
				break;
			}
		}

		if (found_begin) {
			break;
		}
	}

	if (p > IO_DQDQS_OUT_PHASE_MAX + num_additional_fr_cycles * IO_DLL_CHAIN_LENGTH) {
		//USER fail, cannot find first working phase

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}

	DPRINT(2, "wlevel: first valid p=%lu d=%lu", p, d);

	reg_file_set_sub_stage(CAL_SUBSTAGE_LAST_WORKING_DELAY);

	//USER If d is 0 then the working window covers a phase tap and we can follow the old procedure
	//USER  otherwise, we've found the beginning, and we need to increment the dtaps until we find the end
	if (d == 0) {
		COV(WLEVEL_PHASE_PTAP_OVERLAP);
		work_end = work_bgn + IO_DELAY_PER_OPA_TAP;

		//USER step 2: if we have room, back off by one and increment in dtaps

		if (p > 0) {
			int found = 0;
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);

			tmp_delay = work_bgn - IO_DELAY_PER_OPA_TAP;

			for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_bgn;
			     d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP) {
				DPRINT(2, "wlevel: begin-2: p=%lu d=%lu", (p - 1), d);
				scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, d);

				if (rw_mgr_mem_calibrate_write_test_all_ranks
				    (g, 0, PASS_ONE_BIT, &bit_chk)) {
					found = 1;
					work_bgn = tmp_delay;
					break;
				}
			}

			{
				uint32_t d2;
				uint32_t p2;
				if (found) {
					d2 = d;
					p2 = p - 1;
				} else {
					d2 = 0;
					p2 = p;
				}

				DPRINT(2, "wlevel: found begin-A: p=%lu d=%lu ps=%lu", p2, d2,
				       work_bgn);

				BFM_GBL_SET(dqs_wlevel_left_edge[g].p, p2);
				BFM_GBL_SET(dqs_wlevel_left_edge[g].d, d2);
				BFM_GBL_SET(dqs_wlevel_left_edge[g].ps, work_bgn);
			}

			scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, 0);
		} else {
			DPRINT(2, "wlevel: found begin-B: p=%lu d=%lu ps=%lu", p, d, work_bgn);

			BFM_GBL_SET(dqs_wlevel_left_edge[g].p, p);
			BFM_GBL_SET(dqs_wlevel_left_edge[g].d, d);
			BFM_GBL_SET(dqs_wlevel_left_edge[g].ps, work_bgn);
		}

		//USER step 3: go forward from working phase to non working phase, increment in ptaps

		for (p = p + 1;
		     p <= IO_DQDQS_OUT_PHASE_MAX + num_additional_fr_cycles * IO_DLL_CHAIN_LENGTH;
		     p++, work_end += IO_DELAY_PER_OPA_TAP) {
			DPRINT(2, "wlevel: end-0: p=%lu d=%lu", p, (long unsigned int)0);
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

			if (!rw_mgr_mem_calibrate_write_test_all_ranks
			    (g, 0, PASS_ONE_BIT, &bit_chk)) {
				break;
			}
		}

		//USER step 4: back off one from last, increment in dtaps
		//USER The actual increment is done outside the if/else statement since it is shared with other code

		p = p - 1;

		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		work_end -= IO_DELAY_PER_OPA_TAP;
		d = 0;

	} else {
		//USER step 5: Window doesn't cover phase tap, just increment dtaps until failure
		//USER The actual increment is done outside the if/else statement since it is shared with other code
		COV(WLEVEL_PHASE_PTAP_NO_OVERLAP);
		work_end = work_bgn;
		DPRINT(2, "wlevel: found begin-C: p=%lu d=%lu ps=%lu", p, d, work_bgn);
		BFM_GBL_SET(dqs_wlevel_left_edge[g].p, p);
		BFM_GBL_SET(dqs_wlevel_left_edge[g].d, d);
		BFM_GBL_SET(dqs_wlevel_left_edge[g].ps, work_bgn);

	}

	//USER The actual increment until failure
	for (; d <= IO_IO_OUT1_DELAY_MAX; d++, work_end += IO_DELAY_PER_DCHAIN_TAP) {
		DPRINT(2, "wlevel: end: p=%lu d=%lu", p, d);
		scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, d);

		if (!rw_mgr_mem_calibrate_write_test_all_ranks(g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}
	scc_mgr_zero_group(g, test_bgn, 1);

	work_end -= IO_DELAY_PER_DCHAIN_TAP;

	if (work_end >= work_bgn) {
		//USER we have a working range
	} else {
		//USER nil range

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_LAST_WORKING_DELAY);

		return 0;
	}

	DPRINT(2, "wlevel: found end: p=%lu d=%lu; range: [%lu,%lu]", p, d - 1, work_bgn, work_end);
	BFM_GBL_SET(dqs_wlevel_right_edge[g].p, p);
	BFM_GBL_SET(dqs_wlevel_right_edge[g].d, d - 1);
	BFM_GBL_SET(dqs_wlevel_right_edge[g].ps, work_end);

	//USER center

	work_mid = (work_bgn + work_end) / 2;

	DPRINT(2, "wlevel: work_mid=%ld", work_mid);

	tmp_delay = 0;

	for (p = 0;
	     p <= IO_DQDQS_OUT_PHASE_MAX + num_additional_fr_cycles * IO_DLL_CHAIN_LENGTH
	     && tmp_delay < work_mid; p++, tmp_delay += IO_DELAY_PER_OPA_TAP) ;

	if (tmp_delay > work_mid) {
		tmp_delay -= IO_DELAY_PER_OPA_TAP;
		p--;
	}

	while (p > IO_DQDQS_OUT_PHASE_MAX) {
		tmp_delay -= IO_DELAY_PER_OPA_TAP;
		p--;
	}

	scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

	DPRINT(2, "wlevel: p=%lu tmp_delay=%lu left=%lu", p, tmp_delay, work_mid - tmp_delay);

	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_mid;
	     d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP) ;

	if (tmp_delay > work_mid) {
		tmp_delay -= IO_DELAY_PER_DCHAIN_TAP;
		d--;
	}

	DPRINT(2, "wlevel: p=%lu d=%lu tmp_delay=%lu left=%lu", p, d, tmp_delay,
	       work_mid - tmp_delay);

	scc_mgr_apply_group_all_out_delay_add_all_ranks(g, test_bgn, d);

	DPRINT(2, "wlevel: found middle: p=%lu d=%lu", p, d);
	BFM_GBL_SET(dqs_wlevel_mid[g].p, p);
	BFM_GBL_SET(dqs_wlevel_mid[g].d, d);
	BFM_GBL_SET(dqs_wlevel_mid[g].ps, work_mid);

	return 1;
}

#else

//USER Write Levelling -- Full Calibration
static uint32_t rw_mgr_mem_calibrate_wlevel(uint32_t g, uint32_t test_bgn)
{
	uint32_t p, d;
	t_btfld bit_chk;
	uint32_t work_bgn, work_end, work_mid;
	uint32_t tmp_delay;

	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);

	//USER maximum phases for the sweep

	//USER starting phases

	//USER update info for sims

	reg_file_set_group(g);

	//USER starting and end range where writes work

	work_bgn = 0;
	work_end = 0;

	//USER step 1: find first working phase, increment in ptaps

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX; p++, work_bgn += IO_DELAY_PER_OPA_TAP) {
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		if (rw_mgr_mem_calibrate_write_test_all_ranks(g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	if (p > IO_DQDQS_OUT_PHASE_MAX) {
		//USER fail, cannot find first working phase

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}

	work_end = work_bgn + IO_DELAY_PER_OPA_TAP;

	reg_file_set_sub_stage(CAL_SUBSTAGE_LAST_WORKING_DELAY);

	//USER step 2: if we have room, back off by one and increment in dtaps

	if (p > 0) {
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);

		tmp_delay = work_bgn - IO_DELAY_PER_OPA_TAP;

		for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_bgn;
		     d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP) {
			scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, d);

			if (rw_mgr_mem_calibrate_write_test_all_ranks(g, 0, PASS_ONE_BIT, &bit_chk)) {
				work_bgn = tmp_delay;
				break;
			}
		}

		scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, 0);
	}
	//USER step 3: go forward from working phase to non working phase, increment in ptaps

	for (p = p + 1; p <= IO_DQDQS_OUT_PHASE_MAX; p++, work_end += IO_DELAY_PER_OPA_TAP) {
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		if (!rw_mgr_mem_calibrate_write_test_all_ranks(g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	//USER step 4: back off one from last, increment in dtaps

	scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);

	work_end -= IO_DELAY_PER_OPA_TAP;

	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++, work_end += IO_DELAY_PER_DCHAIN_TAP) {
		scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, d);

		if (!rw_mgr_mem_calibrate_write_test_all_ranks(g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	scc_mgr_apply_group_all_out_delay_all_ranks(g, test_bgn, 0);

	if (work_end > work_bgn) {
		//USER we have a working range
	} else {
		//USER nil range

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_LAST_WORKING_DELAY);

		return 0;
	}

	//USER center

	work_mid = (work_bgn + work_end) / 2;

	tmp_delay = 0;

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX && tmp_delay < work_mid;
	     p++, tmp_delay += IO_DELAY_PER_OPA_TAP) ;

	tmp_delay -= IO_DELAY_PER_OPA_TAP;

	scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);

	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_mid;
	     d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP) ;

	scc_mgr_apply_group_all_out_delay_add_all_ranks(g, test_bgn, d - 1);

	return 1;
}

#endif

//USER center all windows. do per-bit-deskew to possibly increase size of certain windows

#if NEWVERSION_WRDESKEW

static uint32_t rw_mgr_mem_calibrate_writes_center(uint32_t rank_bgn, uint32_t write_group,
						   uint32_t test_bgn)
{
	uint32_t i, p, min_index;
	int32_t d;
	//USER Store these as signed since there are comparisons with signed numbers
	t_btfld bit_chk;
	t_btfld sticky_bit_chk;
	int32_t left_edge[RW_MGR_MEM_DQ_PER_WRITE_DQS];
	int32_t right_edge[RW_MGR_MEM_DQ_PER_WRITE_DQS];
	int32_t mid;
	int32_t mid_min, orig_mid_min;
	int32_t new_dqs, start_dqs, shift_dq;
	int32_t dq_margin, dqs_margin, dm_margin;
	uint32_t stop;
	int32_t bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
	int32_t end_curr = IO_IO_OUT1_DELAY_MAX + 1;
	int32_t bgn_best = IO_IO_OUT1_DELAY_MAX + 1;
	int32_t end_best = IO_IO_OUT1_DELAY_MAX + 1;
	int32_t win_best = 0;

	dm_margin = 0;

	start_dqs = READ_SCC_DQS_IO_OUT1_DELAY();

	select_curr_shadow_reg_using_rank(rank_bgn);

	//USER per-bit deskew

	//USER set the left and right edge of each bit to an illegal value
	//USER use (IO_IO_OUT1_DELAY_MAX + 1) as an illegal value
	sticky_bit_chk = 0;
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		left_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
		right_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
	}

	//USER Search for the left edge of the window for each bit
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_out1_delay(write_group, test_bgn, d);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		//USER Stop searching when the read test doesn't pass AND when we've seen a passing read on every bit
		stop =
		    !rw_mgr_mem_calibrate_write_test(rank_bgn, write_group, 0, PASS_ONE_BIT,
						     &bit_chk, 0);
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->write_correct_mask);
		DPRINT(2,
		       "write_center(left): dtap=%lu => " BTFLD_FMT " == " BTFLD_FMT
		       " && %lu [bit_chk=" BTFLD_FMT "]", d, sticky_bit_chk,
		       param->write_correct_mask, stop, bit_chk);

		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
				if (bit_chk & 1) {
					//USER Remember a passing test as the left_edge
					left_edge[i] = d;
				} else {
					//USER If a left edge has not been seen yet, then a future passing test will mark this edge as the right edge
					if (left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[i] = -(d + 1);
					}
				}
				DPRINT(2,
				       "write_center[l,d=%lu): bit_chk_test=%d left_edge[%lu]: %ld right_edge[%lu]: %ld",
				       d, (int)(bit_chk & 1), i, left_edge[i], i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	//USER Reset DQ delay chains to 0
	scc_mgr_apply_group_dq_out1_delay(write_group, test_bgn, 0);
	sticky_bit_chk = 0;
	for (i = RW_MGR_MEM_DQ_PER_WRITE_DQS - 1;; i--) {

		DPRINT(2, "write_center: left_edge[%lu]: %ld right_edge[%lu]: %ld", i, left_edge[i],
		       i, right_edge[i]);

		//USER Check for cases where we haven't found the left edge, which makes our assignment of the the
		//USER right edge invalid.  Reset it to the illegal value.
		if ((left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1)
		    && (right_edge[i] != IO_IO_OUT1_DELAY_MAX + 1)) {
			right_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
			DPRINT(2, "write_center: reset right_edge[%lu]: %ld", i, right_edge[i]);
		}
		//USER Reset sticky bit (except for bits where we have seen the left edge)
		sticky_bit_chk = sticky_bit_chk << 1;
		if ((left_edge[i] != IO_IO_OUT1_DELAY_MAX + 1)) {
			sticky_bit_chk = sticky_bit_chk | 1;
		}

		if (i == 0) {
			break;
		}
	}

	//USER Search for the right edge of the window for each bit
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX - start_dqs; d++) {
		scc_mgr_apply_group_dqs_io_and_oct_out1(write_group, d + start_dqs);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
		if (QDRII) {
			rw_mgr_mem_dll_lock_wait();
		}
		//USER Stop searching when the read test doesn't pass AND when we've seen a passing read on every bit
		stop =
		    !rw_mgr_mem_calibrate_write_test(rank_bgn, write_group, 0, PASS_ONE_BIT,
						     &bit_chk, 0);
		if (stop) {
			recover_mem_device_after_ck_dqs_violation();
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->write_correct_mask);

		DPRINT(2, "write_center (right): dtap=%lu => " BTFLD_FMT " == " BTFLD_FMT " && %lu",
		       d, sticky_bit_chk, param->write_correct_mask, stop);

		if (stop == 1) {
			if (d == 0) {
				for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
					//USER d = 0 failed, but it passed when testing the left edge, so it must be marginal, set it to -1
					if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1
					    && left_edge[i] != IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[i] = -1;
					}
				}
			}
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
				if (bit_chk & 1) {
					//USER Remember a passing test as the right_edge
					right_edge[i] = d;
				} else {
					if (d != 0) {
						//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge
						if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}
					} else {
						//USER d = 0 failed, but it passed when testing the left edge, so it must be marginal, set it to -1
						if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1
						    && left_edge[i] != IO_IO_OUT1_DELAY_MAX + 1) {
							right_edge[i] = -1;
						}
						//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge
						else if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}
					}
				}
				DPRINT(2,
				       "write_center[r,d=%lu): bit_chk_test=%d left_edge[%lu]: %ld right_edge[%lu]: %ld",
				       d, (int)(bit_chk & 1), i, left_edge[i], i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	//USER Check that all bits have a window
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		DPRINT(2, "write_center: left_edge[%lu]: %ld right_edge[%lu]: %ld", i, left_edge[i],
		       i, right_edge[i]);
		BFM_GBL_SET(dq_write_left_edge[write_group][i], left_edge[i]);
		BFM_GBL_SET(dq_write_right_edge[write_group][i], right_edge[i]);
		if ((left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1)
		    || (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1)) {
			set_failing_group_stage(test_bgn + i, CAL_STAGE_WRITES,
						CAL_SUBSTAGE_WRITES_CENTER);
			return 0;
		}
	}

	//USER Find middle of window for each DQ bit
	mid_min = left_edge[0] - right_edge[0];
	min_index = 0;
	for (i = 1; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		mid = left_edge[i] - right_edge[i];
		if (mid < mid_min) {
			mid_min = mid;
			min_index = i;
		}
	}

	//USER  -mid_min/2 represents the amount that we need to move DQS.  If mid_min is odd and positive we'll need to add one to
	//USER make sure the rounding in further calculations is correct (always bias to the right), so just add 1 for all positive values
	if (mid_min > 0) {
		mid_min++;
	}
	mid_min = mid_min / 2;

	DPRINT(1, "write_center: mid_min=%ld", mid_min);

	//USER Determine the amount we can change DQS (which is -mid_min)
	orig_mid_min = mid_min;
	new_dqs = start_dqs;
	mid_min = 0;

	DPRINT(1, "write_center: start_dqs=%ld new_dqs=%ld mid_min=%ld", start_dqs, new_dqs,
	       mid_min);

	//USER Initialize data for export structures
	dqs_margin = IO_IO_OUT1_DELAY_MAX + 1;
	dq_margin = IO_IO_OUT1_DELAY_MAX + 1;

	//USER add delay to bring centre of all DQ windows to the same "level"
	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		//USER Use values before divide by 2 to reduce round off error
		shift_dq =
		    (left_edge[i] - right_edge[i] -
		     (left_edge[min_index] - right_edge[min_index])) / 2 + (orig_mid_min - mid_min);

		DPRINT(2, "write_center: before: shift_dq[%lu]=%ld", i, shift_dq);

		if (shift_dq + (int32_t) READ_SCC_DQ_OUT1_DELAY(i) > (int32_t) IO_IO_OUT1_DELAY_MAX) {
			shift_dq = (int32_t) IO_IO_OUT1_DELAY_MAX - READ_SCC_DQ_OUT1_DELAY(i);
		} else if (shift_dq + (int32_t) READ_SCC_DQ_OUT1_DELAY(i) < 0) {
			shift_dq = -(int32_t) READ_SCC_DQ_OUT1_DELAY(i);
		}
		DPRINT(2, "write_center: after: shift_dq[%lu]=%ld", i, shift_dq);
		scc_mgr_set_dq_out1_delay(write_group, i, READ_SCC_DQ_OUT1_DELAY(i) + shift_dq);
		scc_mgr_load_dq(i);

		DPRINT(2, "write_center: margin[%lu]=[%ld,%ld]", i,
		       left_edge[i] - shift_dq + (-mid_min), right_edge[i] + shift_dq - (-mid_min));
		//USER To determine values for export structures
		if (left_edge[i] - shift_dq + (-mid_min) < dq_margin) {
			dq_margin = left_edge[i] - shift_dq + (-mid_min);
		}
		if (right_edge[i] + shift_dq - (-mid_min) < dqs_margin) {
			dqs_margin = right_edge[i] + shift_dq - (-mid_min);
		}
	}

	//USER Move DQS
	if (QDRII) {
		scc_mgr_set_group_dqs_io_and_oct_out1_gradual(write_group, new_dqs);
	} else {
		scc_mgr_apply_group_dqs_io_and_oct_out1(write_group, new_dqs);
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}

	DPRINT(2, "write_center: DM");

	//USER set the left and right edge of each bit to an illegal value
	//USER use (IO_IO_OUT1_DELAY_MAX + 1) as an illegal value
	left_edge[0] = IO_IO_OUT1_DELAY_MAX + 1;
	right_edge[0] = IO_IO_OUT1_DELAY_MAX + 1;

	//USER Search for the/part of the window with DM shift
	for (d = IO_IO_OUT1_DELAY_MAX; d >= 0; d -= DELTA_D) {
		scc_mgr_apply_group_dm_out1_delay(write_group, d);
		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		if (rw_mgr_mem_calibrate_write_test
		    (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0)) {

			//USE Set current end of the window
			end_curr = -d;
			//USER If a starting edge of our window has not been seen this is our current start of the DM window
			if (bgn_curr == IO_IO_OUT1_DELAY_MAX + 1) {
				bgn_curr = -d;
			}
			//USER If current window is bigger than best seen. Set best seen to be current window
			if ((end_curr - bgn_curr + 1) > win_best) {
				win_best = end_curr - bgn_curr + 1;
				bgn_best = bgn_curr;
				end_best = end_curr;
			}
		} else {
			//USER We just saw a failing test. Reset temp edge
			bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
			end_curr = IO_IO_OUT1_DELAY_MAX + 1;
		}

	}

	//USER Reset DM delay chains to 0
	scc_mgr_apply_group_dm_out1_delay(write_group, 0);

	//USER Check to see if the current window nudges up aganist 0 delay. If so we need to continue the search by shifting DQS otherwise DQS search begins as a new search
	if (end_curr != 0) {
		bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
		end_curr = IO_IO_OUT1_DELAY_MAX + 1;
	}
	//USER Search for the/part of the window with DQS shifts
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX - new_dqs; d += DELTA_D) {
		// Note: This only shifts DQS, so are we limiting ourselve to
		// width of DQ unnecessarily
		scc_mgr_apply_group_dqs_io_and_oct_out1(write_group, d + new_dqs);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		if (rw_mgr_mem_calibrate_write_test
		    (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0)) {

			//USE Set current end of the window
			end_curr = d;
			//USER If a beginning edge of our window has not been seen this is our current begin of the DM window
			if (bgn_curr == IO_IO_OUT1_DELAY_MAX + 1) {
				bgn_curr = d;
			}
			//USER If current window is bigger than best seen. Set best seen to be current window
			if ((end_curr - bgn_curr + 1) > win_best) {
				win_best = end_curr - bgn_curr + 1;
				bgn_best = bgn_curr;
				end_best = end_curr;
			}
		} else {
			//USER We just saw a failing test. Reset temp edge
			recover_mem_device_after_ck_dqs_violation();
			bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
			end_curr = IO_IO_OUT1_DELAY_MAX + 1;

			//USER Early exit optimization: if ther remaining delay chain space is less than already seen largest window we can exit
			if ((win_best - 1) > (IO_IO_OUT1_DELAY_MAX - new_dqs - d)) {
				break;
			}

		}
	}

	//USER assign left and right edge for cal and reporting;
	left_edge[0] = -1 * bgn_best;
	right_edge[0] = end_best;

	DPRINT(2, "dm_calib: left=%ld right=%ld", left_edge[0], right_edge[0]);
	BFM_GBL_SET(dm_left_edge[write_group][0], left_edge[0]);
	BFM_GBL_SET(dm_right_edge[write_group][0], right_edge[0]);

	//USER Move DQS (back to orig)
	scc_mgr_apply_group_dqs_io_and_oct_out1(write_group, new_dqs);

	//USER Move DM

	//USER Find middle of window for the DM bit
	mid = (left_edge[0] - right_edge[0]) / 2;

	//USER only move right, since we are not moving DQS/DQ
	if (mid < 0) {
		mid = 0;
	}
	//dm_marign should fail if we never find a window
	if (win_best == 0) {
		dm_margin = -1;
	} else {
		dm_margin = left_edge[0] - mid;
	}

	scc_mgr_apply_group_dm_out1_delay(write_group, mid);
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

	DPRINT(2, "dm_calib: left=%ld right=%ld mid=%ld dm_margin=%ld",
	       left_edge[0], right_edge[0], mid, dm_margin);

	//USER Export values
	gbl->fom_out += dq_margin + dqs_margin;

	DPRINT(2, "write_center: dq_margin=%ld dqs_margin=%ld dm_margin=%ld", dq_margin, dqs_margin,
	       dm_margin);

	//USER Do not remove this line as it makes sure all of our decisions have been applied
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	return (dq_margin >= 0) && (dqs_margin >= 0) && (dm_margin >= 0);
}

#else // !NEWVERSION_WRDESKEW

static uint32_t rw_mgr_mem_calibrate_writes_center(uint32_t rank_bgn, uint32_t write_group,
						   uint32_t test_bgn)
{
	uint32_t i, p, d;
	uint32_t mid;
	t_btfld bit_chk, sticky_bit_chk;
	uint32_t max_working_dq[RW_MGR_MEM_DQ_PER_WRITE_DQS];
	uint32_t max_working_dm[RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	uint32_t dq_margin, dqs_margin, dm_margin;
	uint32_t start_dqs;
	uint32_t stop;

	//USER per-bit deskew

	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		max_working_dq[i] = 0;
	}

	for (d = 1; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_out1_delay(write_group, test_bgn, d);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		if (!rw_mgr_mem_calibrate_write_test
		    (rank_bgn, write_group, 0, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
				if (bit_chk & 1) {
					max_working_dq[i] = d;
				}
				bit_chk = bit_chk >> 1;
			}
		}
	}

	scc_mgr_apply_group_dq_out1_delay(write_group, test_bgn, 0);

	//USER determine minimum of maximums

	dq_margin = IO_IO_OUT1_DELAY_MAX;

	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		if (max_working_dq[i] < dq_margin) {
			dq_margin = max_working_dq[i];
		}
	}

	//USER add delay to center DQ windows

	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		if (max_working_dq[i] > dq_margin) {
			scc_mgr_set_dq_out1_delay(write_group, i, max_working_dq[i] - dq_margin);
		} else {
			scc_mgr_set_dq_out1_delay(write_group, i, 0);
		}

		scc_mgr_load_dq(p, i);
	}

	//USER sweep DQS window, may potentially have more window due to per-bit-deskew

	start_dqs = READ_SCC_DQS_IO_OUT1_DELAY();

	for (d = start_dqs + 1; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_dqs_io_and_oct_out1(write_group, d);

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

		if (QDRII) {
			rw_mgr_mem_dll_lock_wait();
		}

		if (!rw_mgr_mem_calibrate_write_test
		    (rank_bgn, write_group, 0, PASS_ALL_BITS, &bit_chk, 0)) {
			break;
		}
	}

	scc_mgr_set_dqs_out1_delay(write_group, start_dqs);
	scc_mgr_set_oct_out1_delay(write_group, start_dqs);

	dqs_margin = d - start_dqs - 1;

	//USER time to center, +1 so that we don't go crazy centering DQ

	mid = (dq_margin + dqs_margin + 1) / 2;

	gbl->fom_out += dq_margin + dqs_margin;

	scc_mgr_load_dqs_io();
	scc_mgr_load_dqs_for_write_group(write_group);

	//USER center dq

	if (dq_margin > mid) {
		for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
			scc_mgr_set_dq_out1_delay(write_group, i,
						  READ_SCC_DQ_OUT1_DELAY(i) + dq_margin - mid);
			scc_mgr_load_dq(p, i);
		}
		dqs_margin += dq_margin - mid;
		dq_margin -= dq_margin - mid;
	}
	//USER do dm centering

	if (!RLDRAMX) {
		dm_margin = IO_IO_OUT1_DELAY_MAX;

		if (QDRII) {
			sticky_bit_chk = 0;
			for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
			     i++) {
				max_working_dm[i] = 0;
			}
		}

		for (d = 1; d <= IO_IO_OUT1_DELAY_MAX; d++) {
			scc_mgr_apply_group_dm_out1_delay(write_group, d);
			IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

			if (DDRX) {
				if (rw_mgr_mem_calibrate_write_test
				    (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0)) {
					max_working_dm[0] = d;
				} else {
					break;
				}
			} else {
				stop =
				    !rw_mgr_mem_calibrate_write_test(rank_bgn, write_group, 1,
								     PASS_ALL_BITS, &bit_chk, 0);
				sticky_bit_chk = sticky_bit_chk | bit_chk;
				stop = stop && (sticky_bit_chk == param->read_correct_mask);

				if (stop == 1) {
					break;
				} else {
					for (i = 0;
					     i <
					     RW_MGR_MEM_DATA_MASK_WIDTH /
					     RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
						if ((bit_chk & param->dm_correct_mask) ==
						    param->dm_correct_mask) {
							max_working_dm[i] = d;
						}
						bit_chk =
						    bit_chk >> (RW_MGR_MEM_DATA_WIDTH /
								RW_MGR_MEM_DATA_MASK_WIDTH);
					}
				}
			}
		}

		i = 0;
		for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
			if (max_working_dm[i] > mid) {
				scc_mgr_set_dm_out1_delay(write_group, i, max_working_dm[i] - mid);
			} else {
				scc_mgr_set_dm_out1_delay(write_group, i, 0);
			}

			scc_mgr_load_dm(i);

			if (max_working_dm[i] < dm_margin) {
				dm_margin = max_working_dm[i];
			}
		}
	} else {
		dm_margin = 0;
	}

	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

	return (dq_margin + dqs_margin) > 0;
}

#endif

//USER calibrate the write operations

static uint32_t rw_mgr_mem_calibrate_writes(uint32_t rank_bgn, uint32_t g, uint32_t test_bgn)
{

	reg_file_set_stage(CAL_STAGE_WRITES);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WRITES_CENTER);

	//USER starting phases

	//USER update info for sims

	reg_file_set_group(g);

	if (!rw_mgr_mem_calibrate_writes_center(rank_bgn, g, test_bgn)) {
		set_failing_group_stage(g, CAL_STAGE_WRITES, CAL_SUBSTAGE_WRITES_CENTER);
		return 0;
	}

	return 1;
}

//USER precharge all banks and activate row 0 in bank "000..." and bank "111..."
static void mem_precharge_and_activate(void)
{
	uint32_t r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}
		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER precharge all banks ...
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_0, 0, 0x0F);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_ACTIVATE_0_AND_1_WAIT1);

		IOWR_32DIRECT(RW_MGR_LOAD_CNTR_1, 0, 0x0F);
		IOWR_32DIRECT(RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_ACTIVATE_0_AND_1_WAIT2);

		//USER activate rows
		IOWR_32DIRECT(RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_ACTIVATE_0_AND_1);
	}
}

//USER perform all refreshes necessary over all ranks

//USER Configure various memory related parameters.

static void mem_config(void)
{
	uint32_t rlat, wlat;
	uint32_t rw_wl_nop_cycles;
	uint32_t max_latency;

	//USER read in write and read latency

	wlat = IORD_32DIRECT(MEM_T_WL_ADD, 0);
	wlat += IORD_32DIRECT(DATA_MGR_MEM_T_ADD, 0);	/* WL for hard phy does not include additive latency */

	// YYONG: add addtional write latency to offset the address/command extra clock cycle
	// YYONG: We change the AC mux setting causing AC to be delayed by one mem clock cycle
	// YYONG: only do this for DDR3
	wlat = wlat + 1;

	rlat = IORD_32DIRECT(MEM_T_RL_ADD, 0);

	if (QUARTER_RATE_MODE) {
		//USER In Quarter-Rate the WL-to-nop-cycles works like this
		//USER 0,1     -> 0
		//USER 2,3,4,5 -> 1
		//USER 6,7,8,9 -> 2
		//USER etc...
		rw_wl_nop_cycles = (wlat + 6) / 4 - 1;
	} else if (HALF_RATE_MODE) {
		//USER In Half-Rate the WL-to-nop-cycles works like this
		//USER 0,1 -> -1
		//USER 2,3 -> 0
		//USER 4,5 -> 1
		//USER etc...
		if (wlat % 2) {
			rw_wl_nop_cycles = ((wlat - 1) / 2) - 1;
		} else {
			rw_wl_nop_cycles = (wlat / 2) - 1;
		}
	} else {
		rw_wl_nop_cycles = wlat - 2;
	}
	gbl->rw_wl_nop_cycles = rw_wl_nop_cycles;

	//USER For AV/CV, lfifo is hardened and always runs at full rate
	//USER so max latency in AFI clocks, used here, is correspondingly smaller
	if (QUARTER_RATE_MODE) {
		max_latency = (1 << MAX_LATENCY_COUNT_WIDTH) / 4 - 1;
	} else if (HALF_RATE_MODE) {
		max_latency = (1 << MAX_LATENCY_COUNT_WIDTH) / 2 - 1;
	} else {
		max_latency = (1 << MAX_LATENCY_COUNT_WIDTH) / 1 - 1;
	}
	//USER configure for a burst length of 8

	if (QUARTER_RATE_MODE) {
		//USER write latency
		wlat = (wlat + 5) / 4 + 1;

		//USER set a pretty high read latency initially
		gbl->curr_read_lat = (rlat + 1) / 4 + 8;
	} else if (HALF_RATE_MODE) {
		//USER write latency
		wlat = (wlat - 1) / 2 + 1;

		//USER set a pretty high read latency initially
		gbl->curr_read_lat = (rlat + 1) / 2 + 8;
	} else {
		//USER write latency
		// Adjust Write Latency for Hard PHY
		wlat = wlat + 1;

		//USER set a pretty high read latency initially
		gbl->curr_read_lat = rlat + 16;
	}

	if (gbl->curr_read_lat > max_latency) {
		gbl->curr_read_lat = max_latency;
	}
	IOWR_32DIRECT(PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);

	//USER advertise write latency
	gbl->curr_write_lat = wlat;
	IOWR_32DIRECT(PHY_MGR_AFI_WLAT, 0, wlat - 2);

	//USER initialize bit slips

	mem_precharge_and_activate();
}

//USER Set VFIFO and LFIFO to instant-on settings in skip calibration mode

static void mem_skip_calibrate(void)
{
	uint32_t vfifo_offset;
	uint32_t i, j, r;

	// Need to update every shadow register set used by the interface
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		// Strictly speaking this should be called once per group to make
		// sure each group's delay chains are refreshed from the SCC register file,
		// but since we're resetting all delay chains anyway, we can save some
		// runtime by calling select_shadow_regs_for_update just once to switch rank.
		select_shadow_regs_for_update(r, 0, 1);

		//USER Set output phase alignment settings appropriate for skip calibration
		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {

			scc_mgr_set_dqs_en_phase(i, 0);
			// Case:33398
			//
			// Write data arrives to the I/O two cycles before write latency is reached (720 deg).
			//   -> due to bit-slip in a/c bus
			//   -> to allow board skew where dqs is longer than ck
			//      -> how often can this happen!?
			//      -> can claim back some ptaps for high freq support if we can relax this, but i digress...
			//
			// The write_clk leads mem_ck by 90 deg
			// The minimum ptap of the OPA is 180 deg
			// Each ptap has (360 / IO_DLL_CHAIN_LENGH) deg of delay
			// The write_clk is always delayed by 2 ptaps
			//
			// Hence, to make DQS aligned to CK, we need to delay DQS by:
			//    (720 - 90 - 180 - 2 * (360 / IO_DLL_CHAIN_LENGTH))
			//
			// Dividing the above by (360 / IO_DLL_CHAIN_LENGTH) gives us the number of ptaps, which simplies to:
			//
			//    (1.25 * IO_DLL_CHAIN_LENGTH - 2)
			scc_mgr_set_dqdqs_output_phase(i, (1.25 * IO_DLL_CHAIN_LENGTH - 2));
		}

		IOWR_32DIRECT(SCC_MGR_DQS_ENA, 0, 0xff);
		IOWR_32DIRECT(SCC_MGR_DQS_IO_ENA, 0, 0xff);

		for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
			IOWR_32DIRECT(SCC_MGR_GROUP_COUNTER, 0, i);
			IOWR_32DIRECT(SCC_MGR_DQ_ENA, 0, 0xff);
			IOWR_32DIRECT(SCC_MGR_DM_ENA, 0, 0xff);
		}

		IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	}

	// Compensate for simulation model behaviour
	for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
		scc_mgr_set_dqs_bus_in_delay(i, 10);
		scc_mgr_load_dqs(i);
	}
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);

	//ArriaV has hard FIFOs that can only be initialized by incrementing in sequencer
	vfifo_offset = CALIB_VFIFO_OFFSET;
	for (j = 0; j < vfifo_offset; j++) {
		if (HARD_PHY) {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_HARD_PHY, 0, 0xff);
		} else {
			IOWR_32DIRECT(PHY_MGR_CMD_INC_VFIFO_FR, 0, 0xff);
		}
	}

	IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);

	// For ACV with hard lfifo, we get the skip-cal setting from generation-time constant
	gbl->curr_read_lat = CALIB_LFIFO_OFFSET;
	IOWR_32DIRECT(PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
}

//USER Memory calibration entry point

static uint32_t mem_calibrate(void)
{
	uint32_t i;
	uint32_t rank_bgn, sr;
	uint32_t write_group, write_test_bgn;
	uint32_t read_group, read_test_bgn;
	uint32_t run_groups, current_run;
	uint32_t failing_groups = 0;
	uint32_t group_failed = 0;
	uint32_t sr_failed = 0;

	// Initialize the data settings
	DPRINT(1, "Preparing to init data");
	DPRINT(1, "Init complete");

	gbl->error_substage = CAL_SUBSTAGE_NIL;
	gbl->error_stage = CAL_STAGE_NIL;
	gbl->error_group = 0xff;
	gbl->fom_in = 0;
	gbl->fom_out = 0;

	mem_config();

	if (ARRIAV || CYCLONEV) {
		uint32_t bypass_mode = (HARD_PHY) ? 0x1 : 0x0;
		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
			IOWR_32DIRECT(SCC_MGR_GROUP_COUNTER, 0, i);
			scc_set_bypass_mode(i, bypass_mode);
		}
	}

	if (((DYNAMIC_CALIB_STEPS) & CALIB_SKIP_ALL) == CALIB_SKIP_ALL) {
		//USER Set VFIFO and LFIFO to instant-on settings in skip calibration mode

		mem_skip_calibrate();
	} else {
		for (i = 0; i < NUM_CALIB_REPEAT; i++) {

			//USER Zero all delay chain/phase settings for all groups and all shadow register sets
			scc_mgr_zero_all();

			run_groups = ~param->skip_groups;

			for (write_group = 0, write_test_bgn = 0;
			     write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
			     write_group++, write_test_bgn += RW_MGR_MEM_DQ_PER_WRITE_DQS) {
				// Initialized the group failure
				group_failed = 0;

				// Mark the group as being attempted for calibration

				BFM_GBL_SET(vfifo_idx, 0);
				current_run =
				    run_groups & ((1 << RW_MGR_NUM_DQS_PER_WRITE_GROUP) - 1);
				run_groups = run_groups >> RW_MGR_NUM_DQS_PER_WRITE_GROUP;

				if (current_run == 0) {
					continue;
				}

				IOWR_32DIRECT(SCC_MGR_GROUP_COUNTER, 0, write_group);
				scc_mgr_zero_group(write_group, write_test_bgn, 0);

				for (read_group =
				     write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH /
				     RW_MGR_MEM_IF_WRITE_DQS_WIDTH, read_test_bgn = 0;
				     read_group <
				     (write_group +
				      1) * RW_MGR_MEM_IF_READ_DQS_WIDTH /
				     RW_MGR_MEM_IF_WRITE_DQS_WIDTH && group_failed == 0;
				     read_group++, read_test_bgn += RW_MGR_MEM_DQ_PER_READ_DQS) {

					//USER Calibrate the VFIFO
					if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_VFIFO)) {
						if (!rw_mgr_mem_calibrate_vfifo
						    (read_group, read_test_bgn)) {
							group_failed = 1;

							if (!
							    (gbl->
							     phy_debug_mode_flags &
							     PHY_DEBUG_SWEEP_ALL_GROUPS)) {
								return 0;
							}
						}
					}
				}

				//USER level writes (or align DK with CK for RLDRAMX)
				if (group_failed == 0) {
					if ((DDRX || RLDRAMII) && !(ARRIAV || CYCLONEV)) {
						if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WLEVEL)) {
							if (!rw_mgr_mem_calibrate_wlevel
							    (write_group, write_test_bgn)) {
								group_failed = 1;

								if (!
								    (gbl->
								     phy_debug_mode_flags &
								     PHY_DEBUG_SWEEP_ALL_GROUPS)) {
									return 0;
								}
							}
						}
					}
				}
				//USER Calibrate the output side
				if (group_failed == 0) {
					for (rank_bgn = 0, sr = 0;
					     rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS;
					     rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {
						sr_failed = 0;
						if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES)) {
							if ((STATIC_CALIB_STEPS) &
							    CALIB_SKIP_DELAY_SWEEPS) {
								//USER not needed in quick mode!
							} else {
								//USER Determine if this set of ranks should be skipped entirely
								if (!param->skip_shadow_regs[sr]) {

									//USER Select shadow register set
									select_shadow_regs_for_update
									    (rank_bgn, write_group,
									     1);

									if (!rw_mgr_mem_calibrate_writes(rank_bgn, write_group, write_test_bgn)) {
										sr_failed = 1;
										if (!
										    (gbl->
										     phy_debug_mode_flags
										     &
										     PHY_DEBUG_SWEEP_ALL_GROUPS))
										{
											return 0;
										}
									}
								}
							}
						}
						if (sr_failed == 0) {
						} else {
							group_failed = 1;
						}
					}
				}

				if (group_failed == 0) {
					for (read_group =
					     write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH /
					     RW_MGR_MEM_IF_WRITE_DQS_WIDTH, read_test_bgn = 0;
					     read_group <
					     (write_group +
					      1) * RW_MGR_MEM_IF_READ_DQS_WIDTH /
					     RW_MGR_MEM_IF_WRITE_DQS_WIDTH && group_failed == 0;
					     read_group++, read_test_bgn +=
					     RW_MGR_MEM_DQ_PER_READ_DQS) {

						if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES)) {
							if (!rw_mgr_mem_calibrate_vfifo_end
							    (read_group, read_test_bgn)) {
								group_failed = 1;

								if (!
								    (gbl->
								     phy_debug_mode_flags &
								     PHY_DEBUG_SWEEP_ALL_GROUPS)) {
									return 0;
								}
							}
						}
					}
				}

				if (group_failed == 0) {

#if STATIC_IN_RTL_SIM
#else
#endif
				}

				if (group_failed != 0) {
					failing_groups++;
				}

			}

			// USER If there are any failing groups then report the failure
			if (failing_groups != 0) {
				return 0;
			}
			//USER Calibrate the LFIFO
			if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_LFIFO)) {
				//USER If we're skipping groups as part of debug, don't calibrate LFIFO
				if (param->skip_groups == 0) {
					if (!rw_mgr_mem_calibrate_lfifo()) {
						return 0;
					}
				}
			}
		}
	}

	//USER Do not remove this line as it makes sure all of our decisions have been applied
	IOWR_32DIRECT(SCC_MGR_UPD, 0, 0);
	return 1;
}

static uint32_t run_mem_calibrate(void)
{

	uint32_t pass;
	uint32_t debug_info;
	uint32_t ctrlcfg = IORD_32DIRECT(CTRL_CONFIG_REG, 0);

	// Initialize the debug status to show that calibration has started.
	// This should occur before anything else
	// Reset pass/fail status shown on afi_cal_success/fail
	IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_RESET);
	//stop tracking manger

	IOWR_32DIRECT(CTRL_CONFIG_REG, 0, ctrlcfg & 0xFFBFFFFF);

	initialize();

	rw_mgr_mem_initialize();

	pass = mem_calibrate();

	mem_precharge_and_activate();

	//pe_checkout_pattern();

	IOWR_32DIRECT(PHY_MGR_CMD_FIFO_RESET, 0, 0);

	if (pass) {
#ifdef TEST_SIZE
		if (!check_test_mem(0)) {
			gbl->error_stage = 0x92;
			gbl->error_group = 0x92;
		}
#endif
	}

	//USER Handoff

	//USER Don't return control of the PHY back to AFI when in debug mode
	if ((gbl->phy_debug_mode_flags & PHY_DEBUG_IN_DEBUG_MODE) == 0) {
		rw_mgr_mem_handoff();

		// In Hard PHY this is a 2-bit control:
		// 0: AFI Mux Select
		// 1: DDIO Mux Select
		IOWR_32DIRECT(PHY_MGR_MUX_SEL, 0, 0x2);
	}
	IOWR_32DIRECT(CTRL_CONFIG_REG, 0, ctrlcfg);

	if (pass) {
		IPRINT("CALIBRATION PASSED");

		gbl->fom_in /= 2;
		gbl->fom_out /= 2;

		if (gbl->fom_in > 0xff) {
			gbl->fom_in = 0xff;
		}

		if (gbl->fom_out > 0xff) {
			gbl->fom_out = 0xff;
		}

		// Update the FOM in the register file
		debug_info = gbl->fom_in;
		debug_info |= gbl->fom_out << 8;
		IOWR_32DIRECT(REG_FILE_FOM, 0, debug_info);

		IOWR_32DIRECT(PHY_MGR_CAL_DEBUG_INFO, 0, debug_info);
		IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_SUCCESS);

	} else {

		IPRINT("CALIBRATION FAILED");

		debug_info = gbl->error_stage;
		debug_info |= gbl->error_substage << 8;
		debug_info |= gbl->error_group << 16;

		IOWR_32DIRECT(REG_FILE_FAILING_STAGE, 0, debug_info);
		IOWR_32DIRECT(PHY_MGR_CAL_DEBUG_INFO, 0, debug_info);
		IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_FAIL);

		// Update the failing group/stage in the register file
		debug_info = gbl->error_stage;
		debug_info |= gbl->error_substage << 8;
		debug_info |= gbl->error_group << 16;
		IOWR_32DIRECT(REG_FILE_FAILING_STAGE, 0, debug_info);

	}

	// Set the debug status to show that calibration has ended.
	// This should occur after everything else
	return pass;

}

static void hc_initialize_rom_data(void)
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
	// Initialize the register file with the correct data
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
	// These may need to be included also:
	// wrap_back_en (false)
	// atpg_en (false)
	// pipelineglobalenable (true)

	uint32_t reg;
	// Tracking also gets configured here because it's in the same register
	uint32_t trk_sample_count = 7500;
	uint32_t trk_long_idle_sample_count = (10 << 16) | 100;	// Format is number of outer loops in the 16 MSB, sample count in 16 LSB.

	reg = 0;
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ACDELAYEN_SET(2);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSLOGICDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_RESETDELAYEN_SET(0);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_LPDDRDIS_SET(1);
	// Fix for long latency VFIFO
	// This field selects the intrinsic latency to RDATA_EN/FULL path. 00-bypass, 01- add 5 cycles, 10- add 10 cycles, 11- add 15 cycles.
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ADDLATSEL_SET(0);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_SET(trk_sample_count);
	IOWR_32DIRECT(BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_OFFSET, reg);

	reg = 0;
	reg |=
	    SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_SAMPLECOUNT_31_20_SET(trk_sample_count >>
								SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_WIDTH);
	reg |=
	    SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_SET(trk_long_idle_sample_count);
	IOWR_32DIRECT(BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_OFFSET, reg);

	reg = 0;
	reg |=
	    SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_LONGIDLESAMPLECOUNT_31_20_SET(trk_long_idle_sample_count
									>>
									SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_WIDTH);
	IOWR_32DIRECT(BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_OFFSET, reg);
}

static void initialize_tracking(void)
{
	uint32_t concatenated_longidle = 0x0;
	uint32_t concatenated_delays = 0x0;
	uint32_t concatenated_rw_addr = 0x0;
	uint32_t concatenated_refresh = 0x0;
	uint32_t dtaps_per_ptap;
	uint32_t tmp_delay;

	// compute usable version of value in case we skip full computation later
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DCHAIN_TAP;
	}
	dtaps_per_ptap--;

	concatenated_longidle = concatenated_longidle ^ 10;	//longidle outer loop
	concatenated_longidle = concatenated_longidle << 16;
	concatenated_longidle = concatenated_longidle ^ 100;	//longidle sample count

	concatenated_delays = concatenated_delays ^ 243;	// trfc, worst case of 933Mhz 4Gb
	concatenated_delays = concatenated_delays << 8;
	concatenated_delays = concatenated_delays ^ 14;	// trcd, worst case
	concatenated_delays = concatenated_delays << 8;
	concatenated_delays = concatenated_delays ^ 10;	// vfifo wait
	concatenated_delays = concatenated_delays << 8;
	concatenated_delays = concatenated_delays ^ 4;	// mux delay

	concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_IDLE;
	concatenated_rw_addr = concatenated_rw_addr << 8;
	concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_ACTIVATE_1;
	concatenated_rw_addr = concatenated_rw_addr << 8;
	concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_SGLE_READ;
	concatenated_rw_addr = concatenated_rw_addr << 8;
	concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_PRECHARGE_ALL;

	concatenated_refresh = concatenated_refresh ^ __RW_MGR_REFRESH_ALL;
	concatenated_refresh = concatenated_refresh << 24;
	concatenated_refresh = concatenated_refresh ^ 1000;	// trefi

	// Initialize the register file with the correct data
	IOWR_32DIRECT(REG_FILE_DTAPS_PER_PTAP, 0, dtaps_per_ptap);
	IOWR_32DIRECT(REG_FILE_TRK_SAMPLE_COUNT, 0, 7500);
	IOWR_32DIRECT(REG_FILE_TRK_LONGIDLE, 0, concatenated_longidle);
	IOWR_32DIRECT(REG_FILE_DELAYS, 0, concatenated_delays);
	IOWR_32DIRECT(REG_FILE_TRK_RW_MGR_ADDR, 0, concatenated_rw_addr);
	IOWR_32DIRECT(REG_FILE_TRK_READ_DQS_WIDTH, 0, RW_MGR_MEM_IF_READ_DQS_WIDTH);
	IOWR_32DIRECT(REG_FILE_TRK_RFSH, 0, concatenated_refresh);
}

static int socfpga_mem_calibration(void)
{
	param_t my_param;
	gbl_t my_gbl;
	uint32_t pass;
	uint32_t i;

	param = &my_param;
	gbl = &my_gbl;

	// Initialize the debug mode flags
	gbl->phy_debug_mode_flags = 0;
	// Set the calibration enabled by default
	gbl->phy_debug_mode_flags |= PHY_DEBUG_ENABLE_CAL_RPT;
	// Only enable margining by default if requested
	// Only sweep all groups (regardless of fail state) by default if requested
	//Set enabled read test by default

	// Initialize the register file
	initialize_reg_file();

	// Initialize any PHY CSR
	initialize_hps_phy();

	scc_mgr_initialize();

	initialize_tracking();

	// Initialize the TCL report. This must occur before any printf
	// but after the debug mode flags and register file

	// USER Enable all ranks, groups
	for (i = 0; i < RW_MGR_MEM_NUMBER_OF_RANKS; i++) {
		param->skip_ranks[i] = 0;
	}
	for (i = 0; i < NUM_SHADOW_REGS; ++i) {
		param->skip_shadow_regs[i] = 0;
	}
	param->skip_groups = 0;

	IPRINT("Preparing to start memory calibration");

	DPRINT(1,
	       "%s%s %s ranks=%lu cs/dimm=%lu dq/dqs=%lu,%lu vg/dqs=%lu,%lu dqs=%lu,%lu dq=%lu dm=%lu "
	       "ptap_delay=%lu dtap_delay=%lu dtap_dqsen_delay=%lu, dll=%lu",
	       RDIMM ? "r" : (LRDIMM ? "l" : ""),
	       DDR2 ? "DDR2" : (DDR3 ? "DDR3"
				: (QDRII ? "QDRII"
				   : (RLDRAMII ? "RLDRAMII"
				      : (RLDRAM3 ? "RLDRAM3" : "??PROTO??")))),
	       FULL_RATE ? "FR" : (HALF_RATE ? "HR" : (QUARTER_RATE ? "QR" : "??RATE??")),
	       (long unsigned int)RW_MGR_MEM_NUMBER_OF_RANKS,
	       (long unsigned int)RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM,
	       (long unsigned int)RW_MGR_MEM_DQ_PER_READ_DQS,
	       (long unsigned int)RW_MGR_MEM_DQ_PER_WRITE_DQS,
	       (long unsigned int)RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS,
	       (long unsigned int)RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS,
	       (long unsigned int)RW_MGR_MEM_IF_READ_DQS_WIDTH,
	       (long unsigned int)RW_MGR_MEM_IF_WRITE_DQS_WIDTH,
	       (long unsigned int)RW_MGR_MEM_DATA_WIDTH,
	       (long unsigned int)RW_MGR_MEM_DATA_MASK_WIDTH,
	       (long unsigned int)IO_DELAY_PER_OPA_TAP, (long unsigned int)IO_DELAY_PER_DCHAIN_TAP,
	       (long unsigned int)IO_DELAY_PER_DQS_EN_DCHAIN_TAP,
	       (long unsigned int)IO_DLL_CHAIN_LENGTH);
	DPRINT(1,
	       "max values: en_p=%lu dqdqs_p=%lu en_d=%lu dqs_in_d=%lu io_in_d=%lu io_out1_d=%lu io_out2_d=%lu"
	       "dqs_in_reserve=%lu dqs_out_reserve=%lu", (long unsigned int)IO_DQS_EN_PHASE_MAX,
	       (long unsigned int)IO_DQDQS_OUT_PHASE_MAX, (long unsigned int)IO_DQS_EN_DELAY_MAX,
	       (long unsigned int)IO_DQS_IN_DELAY_MAX, (long unsigned int)IO_IO_IN_DELAY_MAX,
	       (long unsigned int)IO_IO_OUT1_DELAY_MAX, (long unsigned int)IO_IO_OUT2_DELAY_MAX,
	       (long unsigned int)IO_DQS_IN_RESERVE, (long unsigned int)IO_DQS_OUT_RESERVE);

	hc_initialize_rom_data();

	//USER update info for sims
	reg_file_set_stage(CAL_STAGE_NIL);
	reg_file_set_group(0);

	// Load global needed for those actions that require
	// some dynamic calibration support
	dyn_calib_steps = STATIC_CALIB_STEPS;

	// Load global to allow dynamic selection of delay loop settings
	// based on calibration mode
	if (!((DYNAMIC_CALIB_STEPS) & CALIB_SKIP_DELAY_LOOPS)) {
		skip_delay_mask = 0xff;
	} else {
		skip_delay_mask = 0x0;
	}

#ifdef TEST_SIZE
	if (!check_test_mem(1)) {
		IOWR_32DIRECT(PHY_MGR_CAL_DEBUG_INFO, 0, 0x9090);
		IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_FAIL);
	}
	write_test_mem();
	if (!check_test_mem(0)) {
		IOWR_32DIRECT(PHY_MGR_CAL_DEBUG_INFO, 0, 0x9191);
		IOWR_32DIRECT(PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_FAIL);
	}
#endif

	pass = run_mem_calibrate();

	// EMPTY

	return pass;
}
