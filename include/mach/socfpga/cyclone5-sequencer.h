#ifndef _SEQUENCER_H_
#define _SEQUENCER_H_

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

#define ALTERA_ASSERT(condition)
#define ALTERA_INFO_ASSERT(condition,text)

#define RW_MGR_NUM_DM_PER_WRITE_GROUP (RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH)
#define RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP (RW_MGR_TRUE_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH)

#define RW_MGR_NUM_DQS_PER_WRITE_GROUP (RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH)
#define NUM_RANKS_PER_SHADOW_REG (RW_MGR_MEM_NUMBER_OF_RANKS / NUM_SHADOW_REGS)

#define RW_MGR_RUN_SINGLE_GROUP BASE_RW_MGR
#define RW_MGR_RUN_ALL_GROUPS BASE_RW_MGR + 0x0400

#define RW_MGR_DI_BASE (BASE_RW_MGR + 0x0020)

#define DDR3_MR1_ODT_MASK  0xFFFFFD99
#define DDR3_MR2_ODT_MASK  0xFFFFF9FF
#define DDR3_AC_MIRR_MASK  0x020A8

#define RW_MGR_LOAD_CNTR_0 BASE_RW_MGR + 0x0800
#define RW_MGR_LOAD_CNTR_1 BASE_RW_MGR + 0x0804
#define RW_MGR_LOAD_CNTR_2 BASE_RW_MGR + 0x0808
#define RW_MGR_LOAD_CNTR_3 BASE_RW_MGR + 0x080C

#define RW_MGR_LOAD_JUMP_ADD_0 BASE_RW_MGR + 0x0C00
#define RW_MGR_LOAD_JUMP_ADD_1 BASE_RW_MGR + 0x0C04
#define RW_MGR_LOAD_JUMP_ADD_2 BASE_RW_MGR + 0x0C08
#define RW_MGR_LOAD_JUMP_ADD_3 BASE_RW_MGR + 0x0C0C

#define RW_MGR_RESET_READ_DATAPATH BASE_RW_MGR + 0x1000
#define RW_MGR_SOFT_RESET BASE_RW_MGR + 0x2000

#define RW_MGR_SET_CS_AND_ODT_MASK BASE_RW_MGR + 0x1400
#define RW_MGR_SET_ACTIVE_RANK BASE_RW_MGR + 0x2400

#define RW_MGR_LOOPBACK_MODE BASE_RW_MGR + 0x0200

#define RW_MGR_ENABLE_REFRESH BASE_RW_MGR + 0x3000

#define RW_MGR_RANK_NONE 0xFF
#define RW_MGR_RANK_ALL 0x00

#define RW_MGR_ODT_MODE_OFF 0
#define RW_MGR_ODT_MODE_READ_WRITE 1

#define NUM_CALIB_REPEAT	1

#define NUM_READ_TESTS			7
#define NUM_READ_PB_TESTS		7
#define NUM_WRITE_TESTS			15
#define NUM_WRITE_PB_TESTS		31

#define PASS_ALL_BITS			1
#define PASS_ONE_BIT			0

/* calibration stages */

#define CAL_STAGE_NIL			0
#define CAL_STAGE_VFIFO			1
#define CAL_STAGE_WLEVEL		2
#define CAL_STAGE_LFIFO			3
#define CAL_STAGE_WRITES		4
#define CAL_STAGE_FULLTEST		5
#define CAL_STAGE_REFRESH		6
#define CAL_STAGE_CAL_SKIPPED		7
#define CAL_STAGE_CAL_ABORTED		8
#define CAL_STAGE_VFIFO_AFTER_WRITES	9

/* calibration substages */

#define CAL_SUBSTAGE_NIL		0
#define CAL_SUBSTAGE_GUARANTEED_READ	1
#define CAL_SUBSTAGE_DQS_EN_PHASE	2
#define CAL_SUBSTAGE_VFIFO_CENTER	3
#define CAL_SUBSTAGE_WORKING_DELAY	1
#define CAL_SUBSTAGE_LAST_WORKING_DELAY	2
#define CAL_SUBSTAGE_WLEVEL_COPY	3
#define CAL_SUBSTAGE_WRITES_CENTER	1
#define CAL_SUBSTAGE_READ_LATENCY	1
#define CAL_SUBSTAGE_REFRESH		1

#define MAX_RANKS			(RW_MGR_MEM_NUMBER_OF_RANKS)
#define MAX_DQS				(RW_MGR_MEM_IF_WRITE_DQS_WIDTH > RW_MGR_MEM_IF_READ_DQS_WIDTH ? RW_MGR_MEM_IF_WRITE_DQS_WIDTH : RW_MGR_MEM_IF_READ_DQS_WIDTH)
#define MAX_DQ				(RW_MGR_MEM_DATA_WIDTH)
#define MAX_DM				(RW_MGR_MEM_DATA_MASK_WIDTH)

/* length of VFIFO, from SW_MACROS */
#define VFIFO_SIZE			(READ_VALID_FIFO_SIZE)

/* Memory for data transfer between TCL scripts and NIOS.
 *
 * - First word is a command request.
 * - The remaining words are part of the transfer.
 */

/* Define the base address of each manager. */

/* MarkW: how should these base addresses be done for A-V? */
#define BASE_PTR_MGR 					SEQUENCER_PTR_MGR_INST_BASE
#define BASE_PHY_MGR 					(0x00088000)
#define BASE_RW_MGR 					(0x00090000)
#define BASE_DATA_MGR 					(0x00098000)
#define BASE_SCC_MGR					SEQUENCER_SCC_MGR_INST_BASE
#define BASE_REG_FILE					SEQUENCER_REG_FILE_INST_BASE
#define BASE_TIMER					SEQUENCER_TIMER_INST_BASE
#define BASE_MMR                                        (0x000C0000)
#define BASE_TRK_MGR					(0x000D0000)

/* Register file addresses. */
#define REG_FILE_SIGNATURE				(BASE_REG_FILE + 0x0000)
#define REG_FILE_DEBUG_DATA_ADDR		(BASE_REG_FILE + 0x0004)
#define REG_FILE_CUR_STAGE              (BASE_REG_FILE + 0x0008)
#define REG_FILE_FOM                    (BASE_REG_FILE + 0x000C)
#define REG_FILE_FAILING_STAGE          (BASE_REG_FILE + 0x0010)
#define REG_FILE_DEBUG1                 (BASE_REG_FILE + 0x0014)
#define REG_FILE_DEBUG2                 (BASE_REG_FILE + 0x0018)

#define REG_FILE_DTAPS_PER_PTAP         (BASE_REG_FILE + 0x001C)
#define REG_FILE_TRK_SAMPLE_COUNT       (BASE_REG_FILE + 0x0020)
#define REG_FILE_TRK_LONGIDLE           (BASE_REG_FILE + 0x0024)
#define REG_FILE_DELAYS                 (BASE_REG_FILE + 0x0028)
#define REG_FILE_TRK_RW_MGR_ADDR        (BASE_REG_FILE + 0x002C)
#define REG_FILE_TRK_READ_DQS_WIDTH     (BASE_REG_FILE + 0x0030)
#define REG_FILE_TRK_RFSH               (BASE_REG_FILE + 0x0034)
#define CTRL_CONFIG_REG			(BASE_MMR      + 0x0000)

/* PHY manager configuration registers. */

#define PHY_MGR_PHY_RLAT				(BASE_PHY_MGR + 0x4000)
#define PHY_MGR_RESET_MEM_STBL			(BASE_PHY_MGR + 0x4004)
#define PHY_MGR_MUX_SEL					(BASE_PHY_MGR + 0x4008)
#define PHY_MGR_CAL_STATUS				(BASE_PHY_MGR + 0x400c)
#define PHY_MGR_CAL_DEBUG_INFO			(BASE_PHY_MGR + 0x4010)
#define PHY_MGR_VFIFO_RD_EN_OVRD		(BASE_PHY_MGR + 0x4014)
#define PHY_MGR_AFI_WLAT				(BASE_PHY_MGR + 0x4018)
#define PHY_MGR_AFI_RLAT				(BASE_PHY_MGR + 0x401c)

#define PHY_MGR_CAL_RESET				(0)
#define PHY_MGR_CAL_SUCCESS			(1)
#define PHY_MGR_CAL_FAIL				(2)

/* PHY manager command addresses. */

#define PHY_MGR_CMD_INC_VFIFO_FR		(BASE_PHY_MGR + 0x0000)
#define PHY_MGR_CMD_INC_VFIFO_HR		(BASE_PHY_MGR + 0x0004)
#define PHY_MGR_CMD_INC_VFIFO_HARD_PHY	(BASE_PHY_MGR + 0x0004)
#define PHY_MGR_CMD_FIFO_RESET			(BASE_PHY_MGR + 0x0008)
#define PHY_MGR_CMD_INC_VFIFO_FR_HR		(BASE_PHY_MGR + 0x000C)
#define PHY_MGR_CMD_INC_VFIFO_QR		(BASE_PHY_MGR + 0x0010)

/* PHY manager parameters. */

#define PHY_MGR_MAX_RLAT_WIDTH			(BASE_PHY_MGR + 0x0000)
#define PHY_MGR_MAX_AFI_WLAT_WIDTH 		(BASE_PHY_MGR + 0x0004)
#define PHY_MGR_MAX_AFI_RLAT_WIDTH 		(BASE_PHY_MGR + 0x0008)
#define PHY_MGR_CALIB_SKIP_STEPS		(BASE_PHY_MGR + 0x000c)
#define PHY_MGR_CALIB_VFIFO_OFFSET		(BASE_PHY_MGR + 0x0010)
#define PHY_MGR_CALIB_LFIFO_OFFSET		(BASE_PHY_MGR + 0x0014)
#define PHY_MGR_RDIMM					(BASE_PHY_MGR + 0x0018)
#define PHY_MGR_MEM_T_WL				(BASE_PHY_MGR + 0x001c)
#define PHY_MGR_MEM_T_RL				(BASE_PHY_MGR + 0x0020)

/* Data Manager */
#define DATA_MGR_DRAM_CFG				(BASE_DATA_MGR + 0x0000)
#define DATA_MGR_MEM_T_WL				(BASE_DATA_MGR + 0x0004)
#define DATA_MGR_MEM_T_ADD				(BASE_DATA_MGR + 0x0008)
#define DATA_MGR_MEM_T_RL				(BASE_DATA_MGR + 0x000C)
#define DATA_MGR_MEM_T_RFC				(BASE_DATA_MGR + 0x0010)
#define DATA_MGR_MEM_T_REFI				(BASE_DATA_MGR + 0x0014)
#define DATA_MGR_MEM_T_WR				(BASE_DATA_MGR + 0x0018)
#define DATA_MGR_MEM_T_MRD				(BASE_DATA_MGR + 0x001C)
#define DATA_MGR_COL_WIDTH				(BASE_DATA_MGR + 0x0020)
#define DATA_MGR_ROW_WIDTH				(BASE_DATA_MGR + 0x0024)
#define DATA_MGR_BANK_WIDTH				(BASE_DATA_MGR + 0x0028)
#define DATA_MGR_CS_WIDTH				(BASE_DATA_MGR + 0x002C)
#define DATA_MGR_ITF_WIDTH				(BASE_DATA_MGR + 0x0030)
#define DATA_MGR_DVC_WIDTH				(BASE_DATA_MGR + 0x0034)

#define MEM_T_WL_ADD DATA_MGR_MEM_T_WL
#define MEM_T_RL_ADD DATA_MGR_MEM_T_RL

#define CALIB_SKIP_DELAY_LOOPS			(1 << 0)
#define CALIB_SKIP_ALL_BITS_CHK			(1 << 1)
#define CALIB_SKIP_DELAY_SWEEPS			(1 << 2)
#define CALIB_SKIP_VFIFO				(1 << 3)
#define CALIB_SKIP_LFIFO				(1 << 4)
#define CALIB_SKIP_WLEVEL				(1 << 5)
#define CALIB_SKIP_WRITES				(1 << 6)
#define CALIB_SKIP_FULL_TEST			(1 << 7)
#define CALIB_SKIP_ALL					(CALIB_SKIP_VFIFO | CALIB_SKIP_LFIFO | CALIB_SKIP_WLEVEL | CALIB_SKIP_WRITES | CALIB_SKIP_FULL_TEST)
#define CALIB_IN_RTL_SIM				(1 << 8)

/* Scan chain manager command addresses */

#define WRITE_SCC_DQS_IN_DELAY(group, delay)	IOWR_32DIRECT(SCC_MGR_DQS_IN_DELAY, (group) << 2, delay)
#define WRITE_SCC_DQS_EN_DELAY(group, delay)	IOWR_32DIRECT(SCC_MGR_DQS_EN_DELAY, (group) << 2, (delay) + IO_DQS_EN_DELAY_OFFSET)
#define WRITE_SCC_DQS_EN_PHASE(group, phase)	IOWR_32DIRECT(SCC_MGR_DQS_EN_PHASE, (group) << 2, phase)
#define WRITE_SCC_DQDQS_OUT_PHASE(group, phase)	IOWR_32DIRECT(SCC_MGR_DQDQS_OUT_PHASE, (group) << 2, phase)
#define WRITE_SCC_OCT_OUT1_DELAY(group, delay)	IOWR_32DIRECT(SCC_MGR_OCT_OUT1_DELAY, (group) << 2, delay)
#define WRITE_SCC_OCT_OUT2_DELAY(group, delay)
#define WRITE_SCC_DQS_BYPASS(group, bypass)

#define WRITE_SCC_DQ_OUT1_DELAY(pin, delay)		IOWR_32DIRECT(SCC_MGR_IO_OUT1_DELAY, (pin) << 2, delay)

#define WRITE_SCC_DQ_OUT2_DELAY(pin, delay)

#define WRITE_SCC_DQ_IN_DELAY(pin, delay)		IOWR_32DIRECT(SCC_MGR_IO_IN_DELAY, (pin) << 2, delay)

#define WRITE_SCC_DQ_BYPASS(pin, bypass)

#define WRITE_SCC_RFIFO_MODE(pin, mode)

#define WRITE_SCC_HHP_EXTRAS(value) 	    IOWR_32DIRECT(SCC_MGR_HHP_GLOBALS, SCC_MGR_HHP_EXTRAS_OFFSET, value)
#define WRITE_SCC_HHP_DQSE_MAP(value) 	    IOWR_32DIRECT(SCC_MGR_HHP_GLOBALS, SCC_MGR_HHP_DQSE_MAP_OFFSET, value)

#define WRITE_SCC_DQS_IO_OUT1_DELAY(delay)	IOWR_32DIRECT(SCC_MGR_IO_OUT1_DELAY, (RW_MGR_MEM_DQ_PER_WRITE_DQS) << 2, delay)

#define WRITE_SCC_DQS_IO_OUT2_DELAY(delay)

#define WRITE_SCC_DQS_IO_IN_DELAY(delay)	IOWR_32DIRECT(SCC_MGR_IO_IN_DELAY, (RW_MGR_MEM_DQ_PER_WRITE_DQS) << 2, delay)

#define WRITE_SCC_DM_IO_OUT1_DELAY(pin, delay)	IOWR_32DIRECT(SCC_MGR_IO_OUT1_DELAY, (RW_MGR_MEM_DQ_PER_WRITE_DQS + 1 + pin) << 2, delay)

#define WRITE_SCC_DM_IO_OUT2_DELAY(pin, delay)

#define WRITE_SCC_DM_IO_IN_DELAY(pin, delay)	IOWR_32DIRECT(SCC_MGR_IO_IN_DELAY, (RW_MGR_MEM_DQ_PER_WRITE_DQS + 1 + pin) << 2, delay)

#define WRITE_SCC_DM_BYPASS(pin, bypass)

#define READ_SCC_DQS_IN_DELAY(group)	IORD_32DIRECT(SCC_MGR_DQS_IN_DELAY, (group) << 2)
#define READ_SCC_DQS_EN_DELAY(group)	(IORD_32DIRECT(SCC_MGR_DQS_EN_DELAY, (group) << 2) - IO_DQS_EN_DELAY_OFFSET)
#define READ_SCC_DQS_EN_PHASE(group)	IORD_32DIRECT(SCC_MGR_DQS_EN_PHASE, (group) << 2)
#define READ_SCC_DQDQS_OUT_PHASE(group)	IORD_32DIRECT(SCC_MGR_DQDQS_OUT_PHASE, (group) << 2)
#define READ_SCC_OCT_OUT1_DELAY(group)	IORD_32DIRECT(SCC_MGR_OCT_OUT1_DELAY, (group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH) << 2)
#define READ_SCC_OCT_OUT2_DELAY(group)	0
#define READ_SCC_DQS_BYPASS(group) 		0
#define READ_SCC_DQS_BYPASS(group) 		0

#define READ_SCC_DQ_OUT1_DELAY(pin)		IORD_32DIRECT(SCC_MGR_IO_OUT1_DELAY, (pin) << 2)
#define READ_SCC_DQ_OUT2_DELAY(pin)		0
#define READ_SCC_DQ_IN_DELAY(pin)		IORD_32DIRECT(SCC_MGR_IO_IN_DELAY, (pin) << 2)
#define READ_SCC_DQ_BYPASS(pin) 	    0
#define READ_SCC_RFIFO_MODE(pin) 	    0

#define READ_SCC_DQS_IO_OUT1_DELAY()	IORD_32DIRECT(SCC_MGR_IO_OUT1_DELAY, (RW_MGR_MEM_DQ_PER_WRITE_DQS) << 2)
#define READ_SCC_DQS_IO_OUT2_DELAY()	0
#define READ_SCC_DQS_IO_IN_DELAY()	IORD_32DIRECT(SCC_MGR_IO_IN_DELAY, (RW_MGR_MEM_DQ_PER_WRITE_DQS) << 2)

#define READ_SCC_DM_IO_OUT1_DELAY(pin)	IORD_32DIRECT(SCC_MGR_IO_OUT1_DELAY, (RW_MGR_MEM_DQ_PER_WRITE_DQS + 1 + pin) << 2)
#define READ_SCC_DM_IO_OUT2_DELAY(pin)	0
#define READ_SCC_DM_IO_IN_DELAY(pin)	IORD_32DIRECT(SCC_MGR_IO_IN_DELAY, (RW_MGR_MEM_DQ_PER_WRITE_DQS + 1 + pin) << 2)
#define READ_SCC_DM_BYPASS(pin) 	    0

#define SCC_MGR_GROUP_COUNTER			(BASE_SCC_MGR + 0x0000)
#define SCC_MGR_DQS_IN_DELAY			(BASE_SCC_MGR + 0x0100)
#define SCC_MGR_DQS_EN_PHASE			(BASE_SCC_MGR + 0x0200)
#define SCC_MGR_DQS_EN_DELAY			(BASE_SCC_MGR + 0x0300)
#define SCC_MGR_DQDQS_OUT_PHASE			(BASE_SCC_MGR + 0x0400)
#define SCC_MGR_OCT_OUT1_DELAY			(BASE_SCC_MGR + 0x0500)
#define SCC_MGR_IO_OUT1_DELAY			(BASE_SCC_MGR + 0x0700)
#define SCC_MGR_IO_IN_DELAY				(BASE_SCC_MGR + 0x0900)

/* HHP-HPS-specific versions of some commands */
#define SCC_MGR_DQS_EN_DELAY_GATE		(BASE_SCC_MGR + 0x0600)
#define SCC_MGR_IO_OE_DELAY			(BASE_SCC_MGR + 0x0800)
#define SCC_MGR_HHP_GLOBALS				(BASE_SCC_MGR + 0x0A00)
#define SCC_MGR_HHP_RFILE				(BASE_SCC_MGR + 0x0B00)

/* HHP-HPS-specific values */
#define SCC_MGR_HHP_EXTRAS_OFFSET			0
#define SCC_MGR_HHP_DQSE_MAP_OFFSET			1

#define SCC_MGR_DQS_ENA					(BASE_SCC_MGR + 0x0E00)
#define SCC_MGR_DQS_IO_ENA				(BASE_SCC_MGR + 0x0E04)
#define SCC_MGR_DQ_ENA					(BASE_SCC_MGR + 0x0E08)
#define SCC_MGR_DM_ENA					(BASE_SCC_MGR + 0x0E0C)
#define SCC_MGR_UPD						(BASE_SCC_MGR + 0x0E20)
#define SCC_MGR_ACTIVE_RANK				(BASE_SCC_MGR + 0x0E40)
#define SCC_MGR_AFI_CAL_INIT			(BASE_SCC_MGR + 0x0D00)

// PHY Debug mode flag constants
#define PHY_DEBUG_IN_DEBUG_MODE 0x00000001
#define PHY_DEBUG_ENABLE_CAL_RPT 0x00000002
#define PHY_DEBUG_ENABLE_MARGIN_RPT 0x00000004
#define PHY_DEBUG_SWEEP_ALL_GROUPS 0x00000008
#define PHY_DEBUG_DISABLE_GUARANTEED_READ 0x00000010
#define PHY_DEBUG_ENABLE_NON_DESTRUCTIVE_CALIBRATION 0x00000020

// Init and Reset delay constants - Only use if defined by sequencer_defines.h,
// otherwise, revert to defaults
// Default for Tinit = (0+1) * ((202+1) * (2 * 131 + 1) + 1) = 53532 = 200.75us @ 266MHz
#ifdef TINIT_CNTR0_VAL
#define SEQ_TINIT_CNTR0_VAL TINIT_CNTR0_VAL
#else
#define SEQ_TINIT_CNTR0_VAL 0
#endif

#ifdef TINIT_CNTR1_VAL
#define SEQ_TINIT_CNTR1_VAL TINIT_CNTR1_VAL
#else
#define SEQ_TINIT_CNTR1_VAL 202
#endif

#ifdef TINIT_CNTR2_VAL
#define SEQ_TINIT_CNTR2_VAL TINIT_CNTR2_VAL
#else
#define SEQ_TINIT_CNTR2_VAL 131
#endif

// Default for Treset = (2+1) * ((252+1) * (2 * 131 + 1) + 1) = 133563 = 500.86us @ 266MHz
#ifdef TRESET_CNTR0_VAL
#define SEQ_TRESET_CNTR0_VAL TRESET_CNTR0_VAL
#else
#define SEQ_TRESET_CNTR0_VAL 2
#endif

#ifdef TRESET_CNTR1_VAL
#define SEQ_TRESET_CNTR1_VAL TRESET_CNTR1_VAL
#else
#define SEQ_TRESET_CNTR1_VAL 252
#endif

#ifdef TRESET_CNTR2_VAL
#define SEQ_TRESET_CNTR2_VAL TRESET_CNTR2_VAL
#else
#define SEQ_TRESET_CNTR2_VAL 131
#endif

/* Bitfield type changes depending on protocol */
typedef uint32_t t_btfld;

#define RW_MGR_INST_ROM_WRITE BASE_RW_MGR + 0x1800
#define RW_MGR_AC_ROM_WRITE BASE_RW_MGR + 0x1C00

static const uint32_t inst_rom_init_size;
static const uint32_t inst_rom_init[];
static const uint32_t ac_rom_init_size;
static const uint32_t ac_rom_init[];

/* parameter variable holder */

typedef struct sequencer_param_type {
	t_btfld dm_correct_mask;
	t_btfld read_correct_mask;
	t_btfld read_correct_mask_vg;
	t_btfld write_correct_mask;
	t_btfld write_correct_mask_vg;

	/* set a particular entry to 1 if we need to skip a particular rank */

	uint32_t skip_ranks[MAX_RANKS];

	/* set a particular entry to 1 if we need to skip a particular group */

	uint32_t skip_groups;

	/* set a particular entry to 1 if the shadow register (which represents a set of ranks) needs to be skipped */

	uint32_t skip_shadow_regs[NUM_SHADOW_REGS];

} param_t;

/* global variable holder */

typedef struct gbl_type {

	uint32_t phy_debug_mode_flags;

	/* current read latency */

	uint32_t curr_read_lat;

	/* current write latency */

	uint32_t curr_write_lat;

	/* error code */

	uint32_t error_substage;
	uint32_t error_stage;
	uint32_t error_group;

	/* figure-of-merit in, figure-of-merit out */

	uint32_t fom_in;
	uint32_t fom_out;

	//USER Number of RW Mgr NOP cycles between write command and write data
	uint32_t rw_wl_nop_cycles;
} gbl_t;

// External global variables
static gbl_t *gbl;
static param_t *param;

// External functions
static uint32_t run_mem_calibrate(void);
static void rw_mgr_mem_initialize(void);
static void rw_mgr_mem_dll_lock_wait(void);
static inline void scc_mgr_set_dq_in_delay(uint32_t write_group, uint32_t dq_in_group,
					   uint32_t delay);
static inline void scc_mgr_set_dq_out1_delay(uint32_t write_group, uint32_t dq_in_group,
					     uint32_t delay);
static inline void scc_mgr_set_dq_out2_delay(uint32_t write_group, uint32_t dq_in_group,
					     uint32_t delay);
static inline void scc_mgr_load_dq(uint32_t dq_in_group);
static inline void scc_mgr_set_dqs_bus_in_delay(uint32_t read_group, uint32_t delay);
static inline void scc_mgr_load_dqs(uint32_t dqs);
static void scc_mgr_set_group_dqs_io_and_oct_out1_gradual(uint32_t write_group, uint32_t delay);
static void scc_mgr_set_dqs_en_delay_all_ranks(uint32_t read_group, uint32_t delay);
static void scc_mgr_set_dqs_en_phase_all_ranks(uint32_t read_group, uint32_t phase);
static void scc_mgr_set_dqdqs_output_phase_all_ranks(uint32_t write_group, uint32_t phase);
static inline void scc_mgr_set_dm_out1_delay(uint32_t write_group, uint32_t dm, uint32_t delay);
static inline void scc_mgr_set_dm_out2_delay(uint32_t write_group, uint32_t dm, uint32_t delay);
static inline void scc_mgr_load_dm(uint32_t dm);
int sdram_calibration(void);
#endif
