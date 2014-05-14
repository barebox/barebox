/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

/*
 * parse_t30.h - Definitions for the dev/sdram parameters
 */

#include "../parse.h"
#include "nvboot_bct_t30.h"

enum_item s_devtype_table_t30[] = {
	{ "NvBootDevType_Sdmmc", nvboot_dev_type_sdmmc },
	{ "NvBootDevType_Spi", nvboot_dev_type_spi },
	{ "NvBootDevType_Nand", nvboot_dev_type_nand },
	{ "Sdmmc", nvboot_dev_type_sdmmc },
	{ "Spi", nvboot_dev_type_spi },
	{ "Nand", nvboot_dev_type_nand },

	{ NULL, 0 }
};

enum_item s_sdmmc_data_width_table_t30[] = {
	{
	  "NvBootSdmmcDataWidth_4Bit",
	  nvboot_sdmmc_data_width_4bit
	},
	{
	  "NvBootSdmmcDataWidth_8Bit",
	  nvboot_sdmmc_data_width_8bit
	},
	{ "4Bit", nvboot_sdmmc_data_width_4bit },
	{ "8Bit", nvboot_sdmmc_data_width_8bit },
	{ NULL, 0 }
};

enum_item s_spi_clock_source_table_t30[] = {
	{
	    "NvBootSpiClockSource_PllPOut0",
	    nvboot_spi_clock_source_pllp_out0
	},
	{
	    "NvBootSpiClockSource_PllCOut0",
	    nvboot_spi_clock_source_pllc_out0
	},
	{
	    "NvBootSpiClockSource_PllMOut0",
	    nvboot_spi_clock_source_pllm_out0
	},
	{
	    "NvBootSpiClockSource_ClockM",
	    nvboot_spi_clock_source_clockm
	},

	{ "ClockSource_PllPOut0", nvboot_spi_clock_source_pllp_out0 },
	{ "ClockSource_PllCOut0", nvboot_spi_clock_source_pllc_out0 },
	{ "ClockSource_PllMOut0", nvboot_spi_clock_source_pllm_out0 },
	{ "ClockSource_ClockM",   nvboot_spi_clock_source_clockm },


	{ "PllPOut0", nvboot_spi_clock_source_pllp_out0 },
	{ "PllCOut0", nvboot_spi_clock_source_pllc_out0 },
	{ "PllMOut0", nvboot_spi_clock_source_pllm_out0 },
	{ "ClockM",   nvboot_spi_clock_source_clockm },

	{ NULL, 0 }
};

enum_item s_nvboot_memory_type_table_t30[] = {
	{ "NvBootMemoryType_None", nvboot_memory_type_none },
	{ "NvBootMemoryType_Ddr3", nvboot_memory_type_ddr3 },
	{ "NvBootMemoryType_Ddr2", nvboot_memory_type_ddr2 },
	{ "NvBootMemoryType_Ddr", nvboot_memory_type_ddr },
	{ "NvBootMemoryType_LpDdr2", nvboot_memory_type_lpddr2 },
	{ "NvBootMemoryType_LpDdr", nvboot_memory_type_lpddr },

	{ "None", nvboot_memory_type_none },
	{ "Ddr3", nvboot_memory_type_ddr3 },
	{ "Ddr2", nvboot_memory_type_ddr2 },
	{ "Ddr", nvboot_memory_type_ddr },
	{ "LpDdr2", nvboot_memory_type_lpddr2 },
	{ "LpDdr", nvboot_memory_type_lpddr },

	{ NULL, 0 }
};

#define TOKEN(name)						\
	token_##name, field_type_u32, NULL

field_item s_sdram_field_table_t30[] = {
	{ "MemoryType", token_memory_type,
	  field_type_enum, s_nvboot_memory_type_table_t30 },

	{ "PllMChargePumpSetupControl", TOKEN(pllm_charge_pump_setup_ctrl) },
	{ "PllMLoopFilterSetupControl", TOKEN(pllm_loop_filter_setup_ctrl) },
	{ "PllMInputDivider",           TOKEN(pllm_input_divider) },
	{ "PllMFeedbackDivider",        TOKEN(pllm_feedback_divider) },
	{ "PllMPostDivider",            TOKEN(pllm_post_divider) },
	{ "PllMStableTime",             TOKEN(pllm_stable_time) },
	{ "EmcClockDivider",            TOKEN(emc_clock_divider) },
	{ "EmcAutoCalInterval",         TOKEN(emc_auto_cal_interval) },
	{ "EmcAutoCalConfig",           TOKEN(emc_auto_cal_config) },
	{ "EmcAutoCalWait",             TOKEN(emc_auto_cal_wait) },
	{ "EmcPinProgramWait",          TOKEN(emc_pin_program_wait) },
	{ "EmcRc",                      TOKEN(emc_rc) },
	{ "EmcRfc",                     TOKEN(emc_rfc) },
	{ "EmcRas",                     TOKEN(emc_ras) },
	{ "EmcRp",                      TOKEN(emc_rp) },
	{ "EmcR2w",                     TOKEN(emc_r2w) },
	{ "EmcW2r",                     TOKEN(emc_w2r) },
	{ "EmcR2p",                     TOKEN(emc_r2p) },
	{ "EmcW2p",                     TOKEN(emc_w2p) },
	{ "EmcRrd",                     TOKEN(emc_rrd) },
	{ "EmcRdRcd",                   TOKEN(emc_rd_rcd) },
	{ "EmcWrRcd",                   TOKEN(emc_wr_rcd) },
	{ "EmcRext",                    TOKEN(emc_rext) },
	{ "EmcWdv",                     TOKEN(emc_wdv) },
	{ "EmcQUseExtra",               TOKEN(emc_quse_extra) },
	{ "EmcQUse",                    TOKEN(emc_quse) },
	{ "EmcQRst",                    TOKEN(emc_qrst) },
	{ "EmcQSafe",                   TOKEN(emc_qsafe) },
	{ "EmcRdv",                     TOKEN(emc_rdv) },
	{ "EmcRefresh",                 TOKEN(emc_refresh) },
	{ "EmcBurstRefreshNum",         TOKEN(emc_burst_refresh_num) },
	{ "EmcPdEx2Wr",                 TOKEN(emc_pdex2wr) },
	{ "EmcPdEx2Rd",                 TOKEN(emc_pdex2rd) },
	{ "EmcPChg2Pden",               TOKEN(emc_pchg2pden) },
	{ "EmcAct2Pden",                TOKEN(emc_act2pden) },
	{ "EmcAr2Pden",                 TOKEN(emc_ar2pden) },
	{ "EmcRw2Pden",                 TOKEN(emc_rw2pden) },
	{ "EmcTxsr",                    TOKEN(emc_txsr) },
	{ "EmcTcke",                    TOKEN(emc_tcke) },
	{ "EmcTfaw",                    TOKEN(emc_tfaw) },
	{ "EmcTrpab",                   TOKEN(emc_trpab) },
	{ "EmcTClkStable",              TOKEN(emc_tclkstable) },
	{ "EmcTClkStop",                TOKEN(emc_tclkstop) },
	{ "EmcTRefBw",                  TOKEN(emc_trefbw) },
	{ "EmcFbioCfg5",                TOKEN(emc_fbio_cfg5) },
	{ "EmcFbioCfg6",                TOKEN(emc_fbio_cfg6) },
	{ "EmcFbioSpare",               TOKEN(emc_fbio_spare) },
	{ "EmcMrsResetDllWait",         TOKEN(emc_mrs_reset_dll_wait) },
	{ "EmcMrsResetDll",             TOKEN(emc_mrs_reset_dll) },
	{ "EmcMrsDdr2DllReset",         TOKEN(emc_mrs_ddr2_dll_reset) },
	{ "EmcMrs",                     TOKEN(emc_mrs) },
	{ "EmcEmrsEmr2",                TOKEN(emc_emrs_emr2) },
	{ "EmcEmrsEmr3",                TOKEN(emc_emrs_emr3) },
	{ "EmcEmrsDdr2DllEnable",       TOKEN(emc_emrs_ddr2_dll_enable) },
	{ "EmcEmrsDdr2OcdCalib",        TOKEN(emc_emrs_ddr2_ocd_calib) },
	{ "EmcEmrs",                    TOKEN(emc_emrs) },
	{ "EmcMrw1",                    TOKEN(emc_mrw1) },
	{ "EmcMrw2",                    TOKEN(emc_mrw2) },
	{ "EmcMrw3",                    TOKEN(emc_mrw3) },
	{ "EmcMrwResetCommand",         TOKEN(emc_mrw_reset_command) },
	{ "EmcMrwResetNInitWait",       TOKEN(emc_mrw_reset_ninit_wait) },
	{ "EmcAdrCfg",                  TOKEN(emc_adr_cfg) },
	{ "McEmemCfg",                  TOKEN(mc_emem_cfg) },
	{ "EmcCfg2",                    TOKEN(emc_cfg2) },
	{ "EmcCfgDigDll",               TOKEN(emc_cfg_dig_dll) },
	{ "EmcCfgDigDllPeriod",         TOKEN(emc_cfg_dig_dll_period) },
	{ "EmcCfg",                     TOKEN(emc_cfg) },
	{ "EmcDbg",                     TOKEN(emc_dbg) },
	{ "WarmBootWait",               TOKEN(warm_boot_wait) },
	{ "EmcCttTermCtrl",             TOKEN(emc_ctt_term_ctrl) },
	{ "EmcOdtWrite",                TOKEN(emc_odt_write) },
	{ "EmcOdtRead",                 TOKEN(emc_odt_read) },
	{ "EmcZcalWaitCnt",             TOKEN(emc_zcal_wait_cnt) },
	{ "EmcZcalMrwCmd",              TOKEN(emc_zcal_mrw_cmd) },
	{ "EmcDdr2Wait",                TOKEN(emc_ddr2_wait) },
	{ "PmcDdrPwr",                  TOKEN(pmc_ddr_pwr) },
	{ "EmcClockSource",             TOKEN(emc_clock_source) },
	{ "EmcClockUsePllMUD",          TOKEN(emc_clock_use_pll_mud) },
	{ "EmcPinExtraWait",            TOKEN(emc_pin_extra_wait) },
	{ "EmcTimingControlWait",       TOKEN(emc_timing_control_wait) },
	{ "EmcWext",                    TOKEN(emc_wext) },
	{ "EmcCtt",                     TOKEN(emc_ctt) },
	{ "EmcCttDuration",             TOKEN(emc_ctt_duration) },
	{ "EmcPreRefreshReqCnt",        TOKEN(emc_prerefresh_req_cnt) },
	{ "EmcTxsrDll",                 TOKEN(emc_txsr_dll) },
	{ "EmcCfgRsv",                  TOKEN(emc_cfg_rsv) },
	{ "EmcMrwExtra",                TOKEN(emc_mrw_extra) },
	{ "EmcWarmBootMrw1",            TOKEN(emc_warm_boot_mrw1) },
	{ "EmcWarmBootMrw2",            TOKEN(emc_warm_boot_mrw2) },
	{ "EmcWarmBootMrw3",            TOKEN(emc_warm_boot_mrw3) },
	{ "EmcWarmBootMrwExtra",        TOKEN(emc_warm_boot_mrw_extra) },
	{ "EmcWarmBootExtraModeRegWriteEnable",
			TOKEN(emc_warm_boot_extramode_reg_write_enable) },
	{ "EmcExtraModeRegWriteEnable", TOKEN(emc_extramode_reg_write_enable) },
	{ "EmcMrsWaitCnt",              TOKEN(emc_mrs_wait_cnt) },
	{ "EmcCmdQ",                    TOKEN(emc_cmd_q) },
	{ "EmcMc2EmcQ",                 TOKEN(emc_mc2emc_q) },
	{ "EmcDynSelfRefControl",       TOKEN(emc_dyn_self_ref_control) },
	{ "AhbArbitrationXbarCtrlMemInitDone",
			TOKEN(ahb_arbitration_xbar_ctrl_meminit_done) },
	{ "EmcDevSelect",               TOKEN(emc_dev_select) },
	{ "EmcSelDpdCtrl",              TOKEN(emc_sel_dpd_ctrl) },
	{ "EmcDllXformDqs0",            TOKEN(emc_dll_xform_dqs0) },
	{ "EmcDllXformDqs1",            TOKEN(emc_dll_xform_dqs1) },
	{ "EmcDllXformDqs2",            TOKEN(emc_dll_xform_dqs2) },
	{ "EmcDllXformDqs3",            TOKEN(emc_dll_xform_dqs3) },
	{ "EmcDllXformDqs4",            TOKEN(emc_dll_xform_dqs4) },
	{ "EmcDllXformDqs5",            TOKEN(emc_dll_xform_dqs5) },
	{ "EmcDllXformDqs6",            TOKEN(emc_dll_xform_dqs6) },
	{ "EmcDllXformDqs7",            TOKEN(emc_dll_xform_dqs7) },
	{ "EmcDllXformQUse0",           TOKEN(emc_dll_xform_quse0) },
	{ "EmcDllXformQUse1",           TOKEN(emc_dll_xform_quse1) },
	{ "EmcDllXformQUse2",           TOKEN(emc_dll_xform_quse2) },
	{ "EmcDllXformQUse3",           TOKEN(emc_dll_xform_quse3) },
	{ "EmcDllXformQUse4",           TOKEN(emc_dll_xform_quse4) },
	{ "EmcDllXformQUse5",           TOKEN(emc_dll_xform_quse5) },
	{ "EmcDllXformQUse6",           TOKEN(emc_dll_xform_quse6) },
	{ "EmcDllXformQUse7",           TOKEN(emc_dll_xform_quse7) },
	{ "EmcDliTrimTxDqs0",           TOKEN(emc_dli_trim_tx_dqs0) },
	{ "EmcDliTrimTxDqs1",           TOKEN(emc_dli_trim_tx_dqs1) },
	{ "EmcDliTrimTxDqs2",           TOKEN(emc_dli_trim_tx_dqs2) },
	{ "EmcDliTrimTxDqs3",           TOKEN(emc_dli_trim_tx_dqs3) },
	{ "EmcDliTrimTxDqs4",           TOKEN(emc_dli_trim_tx_dqs4) },
	{ "EmcDliTrimTxDqs5",           TOKEN(emc_dli_trim_tx_dqs5) },
	{ "EmcDliTrimTxDqs6",           TOKEN(emc_dli_trim_tx_dqs6) },
	{ "EmcDliTrimTxDqs7",           TOKEN(emc_dli_trim_tx_dqs7) },
	{ "EmcDllXformDq0",             TOKEN(emc_dll_xform_dq0) },
	{ "EmcDllXformDq1",             TOKEN(emc_dll_xform_dq1) },
	{ "EmcDllXformDq2",             TOKEN(emc_dll_xform_dq2) },
	{ "EmcDllXformDq3",             TOKEN(emc_dll_xform_dq3) },
	{ "EmcZcalInterval",            TOKEN(emc_zcal_interval) },
	{ "EmcZcalInitDev0",            TOKEN(emc_zcal_init_dev0) },
	{ "EmcZcalInitDev1",            TOKEN(emc_zcal_init_dev1) },
	{ "EmcZcalInitWait",            TOKEN(emc_zcal_init_wait) },
	{ "EmcZcalColdBootEnable",      TOKEN(emc_zcal_cold_boot_enable) },
	{ "EmcZcalWarmBootEnable",      TOKEN(emc_zcal_warm_boot_enable) },
	{ "EmcMrwLpddr2ZcalWarmBoot",   TOKEN(emc_mrw_lpddr2zcal_warm_boot) },
	{ "EmcZqCalDdr3WarmBoot",       TOKEN(emc_zqcal_ddr3_warm_boot) },
	{ "EmcZcalWarmBootWait",        TOKEN(emc_zcal_warm_boot_wait) },
	{ "EmcMrsWarmBootEnable",       TOKEN(emc_mrs_warm_boot_enable) },
	{ "EmcMrsExtra",                TOKEN(emc_mrs_extra) },
	{ "EmcWarmBootMrs",             TOKEN(emc_warm_boot_mrs) },
	{ "EmcWarmBootEmrs",            TOKEN(emc_warm_boot_emrs) },
	{ "EmcWarmBootEmr2",            TOKEN(emc_warm_boot_emr2) },
	{ "EmcWarmBootEmr3",            TOKEN(emc_warm_boot_emr3) },
	{ "EmcWarmBootMrsExtra",        TOKEN(emc_warm_boot_mrs_extra) },
	{ "EmcClkenOverride",           TOKEN(emc_clken_override) },
	{ "EmcExtraRefreshNum",         TOKEN(emc_extra_refresh_num) },
	{ "EmcClkenOverrideAllWarmBoot",
			TOKEN(emc_clken_override_allwarm_boot) },
	{ "McClkenOverrideAllWarmBoot", TOKEN(mc_clken_override_allwarm_boot) },
	{ "EmcCfgDigDllPeriodWarmBoot",
			TOKEN(emc_cfg_dig_dll_period_warm_boot) },
	{ "PmcVddpSel",                 TOKEN(pmc_vddp_sel) },
	{ "PmcDdrCfg",                  TOKEN(pmc_ddr_cfg) },
	{ "PmcIoDpdReq",                TOKEN(pmc_io_dpd_req) },
	{ "PmcENoVttGen",               TOKEN(pmc_eno_vtt_gen) },
	{ "PmcNoIoPower",               TOKEN(pmc_no_io_power) },
	{ "EmcXm2CmdPadCtrl",           TOKEN(emc_xm2cmd_pad_ctrl) },
	{ "EmcXm2CmdPadCtrl2",          TOKEN(emc_xm2cmd_pad_ctrl2) },
	{ "EmcXm2DqsPadCtrl",           TOKEN(emc_xm2dqs_pad_ctrl) },
	{ "EmcXm2DqsPadCtrl2",          TOKEN(emc_xm2dqs_pad_ctrl2) },
	{ "EmcXm2DqsPadCtrl3",          TOKEN(emc_xm2dqs_pad_ctrl3) },
	{ "EmcXm2DqPadCtrl",            TOKEN(emc_xm2dq_pad_ctrl) },
	{ "EmcXm2DqPadCtrl2",           TOKEN(emc_xm2dq_pad_ctrl2) },
	{ "EmcXm2ClkPadCtrl",           TOKEN(emc_xm2clk_pad_ctrl) },
	{ "EmcXm2CompPadCtrl",          TOKEN(emc_xm2comp_pad_ctrl) },
	{ "EmcXm2VttGenPadCtrl",        TOKEN(emc_xm2vttgen_pad_ctrl) },
	{ "EmcXm2VttGenPadCtrl2",       TOKEN(emc_xm2vttgen_pad_ctrl2) },
	{ "EmcXm2QUsePadCtrl",          TOKEN(emc_xm2quse_pad_ctrl) },
	{ "McEmemAdrCfg",               TOKEN(mc_emem_adr_cfg) },
	{ "McEmemAdrCfgDev0",           TOKEN(mc_emem_adr_cfg_dev0) },
	{ "McEmemAdrCfgDev1",           TOKEN(mc_emem_adr_cfg_dev1) },
	{ "McEmemArbCfg",               TOKEN(mc_emem_arb_cfg) },
	{ "McEmemArbOutstandingReq",    TOKEN(mc_emem_arb_outstanding_req) },
	{ "McEmemArbTimingRcd",         TOKEN(mc_emem_arb_timing_rcd) },
	{ "McEmemArbTimingRp",          TOKEN(mc_emem_arb_timing_rp) },
	{ "McEmemArbTimingRc",          TOKEN(mc_emem_arb_timing_rc) },
	{ "McEmemArbTimingRas",         TOKEN(mc_emem_arb_timing_ras) },
	{ "McEmemArbTimingFaw",         TOKEN(mc_emem_arb_timing_faw) },
	{ "McEmemArbTimingRrd",         TOKEN(mc_emem_arb_timing_rrd) },
	{ "McEmemArbTimingRap2Pre",     TOKEN(mc_emem_arb_timing_rap2pre) },
	{ "McEmemArbTimingWap2Pre",     TOKEN(mc_emem_arb_timing_wap2pre) },
	{ "McEmemArbTimingR2R",         TOKEN(mc_emem_arb_timing_r2r) },
	{ "McEmemArbTimingW2W",         TOKEN(mc_emem_arb_timing_w2w) },
	{ "McEmemArbTimingR2W",         TOKEN(mc_emem_arb_timing_r2w) },
	{ "McEmemArbTimingW2R",         TOKEN(mc_emem_arb_timing_w2r) },
	{ "McEmemArbDaTurns",           TOKEN(mc_emem_arb_da_turns) },
	{ "McEmemArbDaCovers",          TOKEN(mc_emem_arb_da_covers) },
	{ "McEmemArbMisc0",             TOKEN(mc_emem_arb_misc0) },
	{ "McEmemArbMisc1",             TOKEN(mc_emem_arb_misc1) },
	{ "McEmemArbRing1Throttle",     TOKEN(mc_emem_arb_ring1_throttle) },
	{ "McEmemArbOverride",          TOKEN(mc_emem_arb_override) },
	{ "McEmemArbRsv",               TOKEN(mc_emem_arb_rsv) },
	{ "McClkenOverride",            TOKEN(mc_clken_override) },
	{ NULL, 0, 0, NULL }
};

field_item s_nand_table_t30[] = {

	/* Note: NandTiming2 must appear before NandTiming, because NandTiming
	 *       is a prefix of NandTiming2 and would otherwise match first.
	 */
	{ "ClockDivider",          TOKEN(nand_clock_divider) },
	{ "BlockSizeLog2",          TOKEN(nand_block_size_log2) },
	{ "PageSizeLog2",          TOKEN(nand_page_size_log2) },
	{ "NandAsyncTiming3",          TOKEN(nand_async_timing3) },
	{ "NandAsyncTiming2",          TOKEN(nand_async_timing2) },
	{ "NandAsyncTiming1",          TOKEN(nand_async_timing1) },
	{ "NandAsyncTiming0",          TOKEN(nand_async_timing0) },
	{ "DisableSyncDDR",          TOKEN(nand_disable_sync_ddr) },
	{ "NandSDDRTiming1",          TOKEN(nand_sddr_timing1) },
	{ "NandSDDRTiming0",          TOKEN(nand_sddr_timing0) },
	{ "NandTDDRTiming1",          TOKEN(nand_tddr_timing1) },
	{ "NandTDDRTiming0",          TOKEN(nand_tddr_timing0) },
	{ "NandFbioDqsibDlyByte",          TOKEN(nand_fbio_dqsib_dly_byte) },
	{ "NandFbioQuseDlyByte",          TOKEN(nand_fbio_quse_dly_byte) },
	{ "NandFbioCfgQuseLate",          TOKEN(nand_fbio_cfg_quse_late) },
	{ NULL, 0, 0, NULL }
};

field_item s_sdmmc_table_t30[] = {
	{ "ClockDivider",           TOKEN(sdmmc_clock_divider) },
	{ "DataWidth",
	  token_sdmmc_data_width,
	  field_type_enum,
	  s_sdmmc_data_width_table_t30 },
	{ "MaxPowerClassSupported", TOKEN(sdmmc_max_power_class_supported) },
	{ "SdController", TOKEN(sdmmc_sd_controller) },
	{ NULL, 0, 0, NULL }
};

field_item s_spiflash_table_t30[] = {
	{ "ReadCommandTypeFast", TOKEN(spiflash_read_command_type_fast) },
	{ "ClockDivider",        TOKEN(spiflash_clock_divider) },
	{ "ClockSource",
	  token_spiflash_clock_source,
	  field_type_enum,
	  s_spi_clock_source_table_t30 },
	{ NULL, 0, 0, NULL }
};

parse_subfield_item s_device_type_table_t30[] = {
	{ "NandParams.", token_nand_params,
		s_nand_table_t30, t30_set_dev_param },
	{ "SdmmcParams.", token_sdmmc_params,
		s_sdmmc_table_t30, t30_set_dev_param },
	{ "SpiFlashParams.", token_spiflash_params,
		s_spiflash_table_t30, t30_set_dev_param },

	{ NULL, 0, NULL }
};
