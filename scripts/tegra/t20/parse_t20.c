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
 * parse_t20.c - Parsing code for t20
 */

#include "../parse.h"
#include "nvboot_bct_t20.h"

enum_item s_devtype_table_t20[] = {
	{ "NvBootDevType_Sdmmc", nvboot_dev_type_sdmmc },
	{ "NvBootDevType_Spi", nvboot_dev_type_spi },
	{ "NvBootDevType_Nand", nvboot_dev_type_nand },
	{ "Sdmmc", nvboot_dev_type_sdmmc },
	{ "Spi", nvboot_dev_type_spi },
	{ "Nand", nvboot_dev_type_nand },

	{ NULL, 0 }
};

enum_item s_sdmmc_data_width_table_t20[] = {
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

enum_item s_spi_clock_source_table_t20[] = {
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

enum_item s_nvboot_memory_type_table_t20[] = {
	{ "NvBootMemoryType_None", nvboot_memory_type_none },
	{ "NvBootMemoryType_Ddr2", nvboot_memory_type_ddr2 },
	{ "NvBootMemoryType_Ddr", nvboot_memory_type_ddr },
	{ "NvBootMemoryType_LpDdr2", nvboot_memory_type_lpddr2 },
	{ "NvBootMemoryType_LpDdr", nvboot_memory_type_lpddr },

	{ "None", nvboot_memory_type_none },
	{ "Ddr2", nvboot_memory_type_ddr2 },
	{ "Ddr", nvboot_memory_type_ddr },
	{ "LpDdr2", nvboot_memory_type_lpddr2 },
	{ "LpDdr", nvboot_memory_type_lpddr },

	{ NULL, 0 }
};

#define TOKEN(name)						\
	token_##name, field_type_u32, NULL

field_item s_sdram_field_table_t20[] = {
	{ "MemoryType", token_memory_type,
	  field_type_enum, s_nvboot_memory_type_table_t20 },

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
	{ "EmcFbioCfg1",                TOKEN(emc_fbio_cfg1) },
	{ "EmcFbioDqsibDlyMsb",         TOKEN(emc_fbio_dqsib_dly_msb) },
	{ "EmcFbioDqsibDly",            TOKEN(emc_fbio_dqsib_dly) },
	{ "EmcFbioQuseDlyMsb",          TOKEN(emc_fbio_quse_dly_msb) },
	{ "EmcFbioQuseDly",             TOKEN(emc_fbio_quse_dly) },
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
	{ "EmcAdrCfg1",                 TOKEN(emc_adr_cfg1) },
	{ "EmcAdrCfg",                  TOKEN(emc_adr_cfg) },
	{ "McEmemCfg",                  TOKEN(mc_emem_cfg) },
	{ "McLowLatencyConfig",         TOKEN(mc_lowlatency_config) },
	{ "EmcCfg2",                    TOKEN(emc_cfg2) },
	{ "EmcCfgDigDll",               TOKEN(emc_cfg_dig_dll) },
	{ "EmcCfgClktrim0",             TOKEN(emc_cfg_clktrim0) },
	{ "EmcCfgClktrim1",             TOKEN(emc_cfg_clktrim1) },
	{ "EmcCfgClktrim2",             TOKEN(emc_cfg_clktrim2) },
	{ "EmcCfg",                     TOKEN(emc_cfg) },
	{ "EmcDbg",                     TOKEN(emc_dbg) },
	{ "AhbArbitrationXbarCtrl",     TOKEN(ahb_arbitration_xbar_ctrl) },
	{ "EmcDllXformDqs",             TOKEN(emc_dll_xform_dqs) },
	{ "EmcDllXformQUse",            TOKEN(emc_dll_xform_quse) },
	{ "WarmBootWait",               TOKEN(warm_boot_wait) },
	{ "EmcCttTermCtrl",             TOKEN(emc_ctt_term_ctrl) },
	{ "EmcOdtWrite",                TOKEN(emc_odt_write) },
	{ "EmcOdtRead",                 TOKEN(emc_odt_read) },
	{ "EmcZcalRefCnt",              TOKEN(emc_zcal_ref_cnt) },
	{ "EmcZcalWaitCnt",             TOKEN(emc_zcal_wait_cnt) },
	{ "EmcZcalMrwCmd",              TOKEN(emc_zcal_mrw_cmd) },
	{ "EmcMrwZqInitDev0",           TOKEN(emc_mrw_zq_init_dev0) },
	{ "EmcMrwZqInitDev1",           TOKEN(emc_mrw_zq_init_dev1) },
	{ "EmcMrwZqInitWait",           TOKEN(emc_mrw_zq_init_wait) },
	{ "EmcDdr2Wait",                TOKEN(emc_ddr2_wait) },
	{ "PmcDdrPwr",                  TOKEN(pmc_ddr_pwr) },
	{ "ApbMiscGpXm2CfgAPadCtrl",    TOKEN(apb_misc_gp_xm2cfga_pad_ctrl) },
	{ "ApbMiscGpXm2CfgCPadCtrl2",   TOKEN(apb_misc_gp_xm2cfgc_pad_ctrl2) },
	{ "ApbMiscGpXm2CfgCPadCtrl",    TOKEN(apb_misc_gp_xm2cfgc_pad_ctrl) },
	{ "ApbMiscGpXm2CfgDPadCtrl2",   TOKEN(apb_misc_gp_xm2cfgd_pad_ctrl2) },
	{ "ApbMiscGpXm2CfgDPadCtrl",    TOKEN(apb_misc_gp_xm2cfgd_pad_ctrl) },
	{ "ApbMiscGpXm2ClkCfgPadCtrl",  TOKEN(apb_misc_gp_xm2clkcfg_Pad_ctrl)},
	{ "ApbMiscGpXm2CompPadCtrl",    TOKEN(apb_misc_gp_xm2comp_pad_ctrl) },
	{ "ApbMiscGpXm2VttGenPadCtrl",  TOKEN(apb_misc_gp_xm2vttgen_pad_ctrl)},
	{ NULL, 0, 0, NULL }
};

field_item s_nand_table_t20[] = {
	{ "ClockDivider",  TOKEN(nand_clock_divider) },
	/* Note: NandTiming2 must appear before NandTiming, because NandTiming
	 *       is a prefix of NandTiming2 and would otherwise match first.
	 */
	{ "NandTiming2",   TOKEN(nand_nand_timing2) },
	{ "NandTiming",    TOKEN(nand_nand_timing) },
	{ "BlockSizeLog2", TOKEN(nand_block_size_log2) },
	{ "PageSizeLog2",  TOKEN(nand_page_size_log2) },
	{ NULL, 0, 0, NULL }
};

field_item s_sdmmc_table_t20[] = {
	{ "ClockDivider",           TOKEN(sdmmc_clock_divider) },
	{ "DataWidth",
	  token_sdmmc_data_width,
	  field_type_enum,
	  s_sdmmc_data_width_table_t20 },
	{ "MaxPowerClassSupported", TOKEN(sdmmc_max_power_class_supported) },
	{ NULL, 0, 0, NULL }
};

field_item s_spiflash_table_t20[] = {
	{ "ReadCommandTypeFast", TOKEN(spiflash_read_command_type_fast) },
	{ "ClockDivider",        TOKEN(spiflash_clock_divider) },
	{ "ClockSource",
	  token_spiflash_clock_source,
	  field_type_enum,
	  s_spi_clock_source_table_t20 },
	{ NULL, 0, 0, NULL }
};

parse_subfield_item s_device_type_table_t20[] = {
	{ "NandParams.", token_nand_params,
		s_nand_table_t20, t20_set_dev_param },
	{ "SdmmcParams.", token_sdmmc_params,
		s_sdmmc_table_t20, t20_set_dev_param },
	{ "SpiFlashParams.", token_spiflash_params,
		s_spiflash_table_t20, t20_set_dev_param },

	{ NULL, 0, NULL }
};
