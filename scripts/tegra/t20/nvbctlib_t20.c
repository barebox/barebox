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

#include "../cbootimage.h"
#include "../parse.h"
#include "../crypto.h"
#include "nvboot_bct_t20.h"
#include "string.h"

/* nvbctlib_t20.c: The implementation of the nvbctlib API for t20. */

/* Definitions that simplify the code which follows. */
#define CASE_GET_SDRAM_PARAM(x) \
case token_##x:\
	*value = params->x; \
	break

#define CASE_SET_SDRAM_PARAM(x) \
case token_##x:\
	params->x = value; \
	break

#define CASE_GET_DEV_PARAM(dev, x) \
case token_##dev##_##x:\
	*value = bct->dev_params[index].dev##_params.x; \
	break

#define CASE_SET_DEV_PARAM(dev, x) \
case token_##dev##_##x:\
	bct->dev_params[index].dev##_params.x = value; \
	break

#define CASE_GET_BL_PARAM(x) \
case token_bl_##x:\
	*data = bct_ptr->bootloader[set].x; \
	break

#define CASE_SET_BL_PARAM(x) \
case token_bl_##x:\
	bct_ptr->bootloader[set].x = *data; \
	break

#define CASE_GET_NVU32(id) \
case token_##id:\
	if (bct == NULL) return -ENODATA; \
	*data = bct_ptr->id; \
	break

#define CASE_GET_CONST(id, val) \
case token_##id:\
	*data = val; \
	break

#define CASE_GET_CONST_PREFIX(id, val_prefix) \
case token_##id:\
	*data = val_prefix##_##id; \
	break

#define CASE_SET_NVU32(id) \
case token_##id:\
	bct_ptr->id = data; \
	break

#define CASE_GET_DATA(id, size) \
case token_##id:\
	if (*length < size) return -ENODATA;\
	memcpy(data, &(bct_ptr->id), size);   \
	*length = size;\
	break

#define CASE_SET_DATA(id, size) \
case token_##id:\
	if (length < size) return -ENODATA;\
	memcpy(&(bct_ptr->id), data, size);   \
	break

#define DEFAULT()                                         \
default :                                                 \
	printf("Unexpected token %d at line %d\n",        \
		token, __LINE__);                         \
	return 1

int
t20_set_dev_param(build_image_context *context,
	u_int32_t index,
	parse_token token,
	u_int32_t value)
{
	nvboot_config_table *bct = NULL;

	bct = (nvboot_config_table *)(context->bct);
	assert(context != NULL);
	assert(bct != NULL);

	bct->num_param_sets = NV_MAX(bct->num_param_sets, index + 1);

	switch (token) {
	CASE_SET_DEV_PARAM(nand, clock_divider);
	CASE_SET_DEV_PARAM(nand, nand_timing);
	CASE_SET_DEV_PARAM(nand, nand_timing2);
	CASE_SET_DEV_PARAM(nand, block_size_log2);
	CASE_SET_DEV_PARAM(nand, page_size_log2);

	CASE_SET_DEV_PARAM(sdmmc, clock_divider);
	CASE_SET_DEV_PARAM(sdmmc, data_width);
	CASE_SET_DEV_PARAM(sdmmc, max_power_class_supported);

	CASE_SET_DEV_PARAM(spiflash, clock_source);
	CASE_SET_DEV_PARAM(spiflash, clock_divider);
	CASE_SET_DEV_PARAM(spiflash, read_command_type_fast);

	case token_dev_type:
		bct->dev_type[index] = value;
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t20_get_dev_param(build_image_context *context,
	u_int32_t index,
	parse_token token,
	u_int32_t *value)
{
	nvboot_config_table *bct = NULL;

	bct = (nvboot_config_table *)(context->bct);
	assert(context != NULL);
	assert(bct != NULL);

	switch (token) {
	CASE_GET_DEV_PARAM(nand, clock_divider);
	CASE_GET_DEV_PARAM(nand, nand_timing);
	CASE_GET_DEV_PARAM(nand, nand_timing2);
	CASE_GET_DEV_PARAM(nand, block_size_log2);
	CASE_GET_DEV_PARAM(nand, page_size_log2);

	CASE_GET_DEV_PARAM(sdmmc, clock_divider);
	CASE_GET_DEV_PARAM(sdmmc, data_width);
	CASE_GET_DEV_PARAM(sdmmc, max_power_class_supported);

	CASE_GET_DEV_PARAM(spiflash, clock_source);
	CASE_GET_DEV_PARAM(spiflash, clock_divider);
	CASE_GET_DEV_PARAM(spiflash, read_command_type_fast);

	case token_dev_type:
		*value = bct->dev_type[index];
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t20_set_sdram_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t value)
{
	nvboot_sdram_params *params;
	nvboot_config_table *bct = NULL;

	bct = (nvboot_config_table *)(context->bct);
	assert(context != NULL);
	assert(bct != NULL);
	params = &(bct->sdram_params[index]);
	/* Update the number of SDRAM parameter sets. */
	bct->num_sdram_sets = NV_MAX(bct->num_sdram_sets, index + 1);

	switch (token) {
	CASE_SET_SDRAM_PARAM(memory_type);
	CASE_SET_SDRAM_PARAM(pllm_charge_pump_setup_ctrl);
	CASE_SET_SDRAM_PARAM(pllm_loop_filter_setup_ctrl);
	CASE_SET_SDRAM_PARAM(pllm_input_divider);
	CASE_SET_SDRAM_PARAM(pllm_feedback_divider);
	CASE_SET_SDRAM_PARAM(pllm_post_divider);
	CASE_SET_SDRAM_PARAM(pllm_stable_time);
	CASE_SET_SDRAM_PARAM(emc_clock_divider);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_interval);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_wait);
	CASE_SET_SDRAM_PARAM(emc_pin_program_wait);
	CASE_SET_SDRAM_PARAM(emc_rc);
	CASE_SET_SDRAM_PARAM(emc_rfc);
	CASE_SET_SDRAM_PARAM(emc_ras);
	CASE_SET_SDRAM_PARAM(emc_rp);
	CASE_SET_SDRAM_PARAM(emc_r2w);
	CASE_SET_SDRAM_PARAM(emc_w2r);
	CASE_SET_SDRAM_PARAM(emc_r2p);
	CASE_SET_SDRAM_PARAM(emc_w2p);
	CASE_SET_SDRAM_PARAM(emc_rd_rcd);
	CASE_SET_SDRAM_PARAM(emc_wr_rcd);
	CASE_SET_SDRAM_PARAM(emc_rrd);
	CASE_SET_SDRAM_PARAM(emc_rext);
	CASE_SET_SDRAM_PARAM(emc_wdv);
	CASE_SET_SDRAM_PARAM(emc_quse);
	CASE_SET_SDRAM_PARAM(emc_qrst);
	CASE_SET_SDRAM_PARAM(emc_qsafe);
	CASE_SET_SDRAM_PARAM(emc_rdv);
	CASE_SET_SDRAM_PARAM(emc_refresh);
	CASE_SET_SDRAM_PARAM(emc_burst_refresh_num);
	CASE_SET_SDRAM_PARAM(emc_pdex2wr);
	CASE_SET_SDRAM_PARAM(emc_pdex2rd);
	CASE_SET_SDRAM_PARAM(emc_pchg2pden);
	CASE_SET_SDRAM_PARAM(emc_act2pden);
	CASE_SET_SDRAM_PARAM(emc_ar2pden);
	CASE_SET_SDRAM_PARAM(emc_rw2pden);
	CASE_SET_SDRAM_PARAM(emc_txsr);
	CASE_SET_SDRAM_PARAM(emc_tcke);
	CASE_SET_SDRAM_PARAM(emc_tfaw);
	CASE_SET_SDRAM_PARAM(emc_trpab);
	CASE_SET_SDRAM_PARAM(emc_tclkstable);
	CASE_SET_SDRAM_PARAM(emc_tclkstop);
	CASE_SET_SDRAM_PARAM(emc_trefbw);
	CASE_SET_SDRAM_PARAM(emc_quse_extra);
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg1);
	CASE_SET_SDRAM_PARAM(emc_fbio_dqsib_dly);
	CASE_SET_SDRAM_PARAM(emc_fbio_dqsib_dly_msb);
	CASE_SET_SDRAM_PARAM(emc_fbio_quse_dly);
	CASE_SET_SDRAM_PARAM(emc_fbio_quse_dly_msb);
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg5);
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg6);
	CASE_SET_SDRAM_PARAM(emc_fbio_spare);
	CASE_SET_SDRAM_PARAM(emc_mrs);
	CASE_SET_SDRAM_PARAM(emc_emrs);
	CASE_SET_SDRAM_PARAM(emc_mrw1);
	CASE_SET_SDRAM_PARAM(emc_mrw2);
	CASE_SET_SDRAM_PARAM(emc_mrw3);
	CASE_SET_SDRAM_PARAM(emc_mrw_reset_command);
	CASE_SET_SDRAM_PARAM(emc_mrw_reset_ninit_wait);
	CASE_SET_SDRAM_PARAM(emc_adr_cfg);
	CASE_SET_SDRAM_PARAM(emc_adr_cfg1);
	CASE_SET_SDRAM_PARAM(mc_emem_cfg);
	CASE_SET_SDRAM_PARAM(mc_lowlatency_config);
	CASE_SET_SDRAM_PARAM(emc_cfg);
	CASE_SET_SDRAM_PARAM(emc_cfg2);
	CASE_SET_SDRAM_PARAM(emc_dbg);
	CASE_SET_SDRAM_PARAM(ahb_arbitration_xbar_ctrl);
	CASE_SET_SDRAM_PARAM(emc_cfg_dig_dll);
	CASE_SET_SDRAM_PARAM(emc_dll_xform_dqs);
	CASE_SET_SDRAM_PARAM(emc_dll_xform_quse);
	CASE_SET_SDRAM_PARAM(warm_boot_wait);
	CASE_SET_SDRAM_PARAM(emc_ctt_term_ctrl);
	CASE_SET_SDRAM_PARAM(emc_odt_write);
	CASE_SET_SDRAM_PARAM(emc_odt_read);
	CASE_SET_SDRAM_PARAM(emc_zcal_ref_cnt);
	CASE_SET_SDRAM_PARAM(emc_zcal_wait_cnt);
	CASE_SET_SDRAM_PARAM(emc_zcal_mrw_cmd);
	CASE_SET_SDRAM_PARAM(emc_mrs_reset_dll);
	CASE_SET_SDRAM_PARAM(emc_mrw_zq_init_dev0);
	CASE_SET_SDRAM_PARAM(emc_mrw_zq_init_dev1);
	CASE_SET_SDRAM_PARAM(emc_mrw_zq_init_wait);
	CASE_SET_SDRAM_PARAM(emc_mrs_reset_dll_wait);
	CASE_SET_SDRAM_PARAM(emc_emrs_emr2);
	CASE_SET_SDRAM_PARAM(emc_emrs_emr3);
	CASE_SET_SDRAM_PARAM(emc_emrs_ddr2_dll_enable);
	CASE_SET_SDRAM_PARAM(emc_mrs_ddr2_dll_reset);
	CASE_SET_SDRAM_PARAM(emc_emrs_ddr2_ocd_calib);
	CASE_SET_SDRAM_PARAM(emc_ddr2_wait);
	CASE_SET_SDRAM_PARAM(emc_cfg_clktrim0);
	CASE_SET_SDRAM_PARAM(emc_cfg_clktrim1);
	CASE_SET_SDRAM_PARAM(emc_cfg_clktrim2);
	CASE_SET_SDRAM_PARAM(pmc_ddr_pwr);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfga_pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfgc_pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfgc_pad_ctrl2);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfgd_pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfgd_pad_ctrl2);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2clkcfg_Pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2comp_pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2vttgen_pad_ctrl);

	DEFAULT();
	}
	return 0;
}

int
t20_get_sdram_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t *value)
{
	nvboot_sdram_params *params;
	nvboot_config_table *bct = NULL;

	bct = (nvboot_config_table *)(context->bct);
	assert(context != NULL);
	assert(bct != NULL);
	params = &(bct->sdram_params[index]);

	switch (token) {
	CASE_GET_SDRAM_PARAM(memory_type);
	CASE_GET_SDRAM_PARAM(pllm_charge_pump_setup_ctrl);
	CASE_GET_SDRAM_PARAM(pllm_loop_filter_setup_ctrl);
	CASE_GET_SDRAM_PARAM(pllm_input_divider);
	CASE_GET_SDRAM_PARAM(pllm_feedback_divider);
	CASE_GET_SDRAM_PARAM(pllm_post_divider);
	CASE_GET_SDRAM_PARAM(pllm_stable_time);
	CASE_GET_SDRAM_PARAM(emc_clock_divider);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_interval);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_wait);
	CASE_GET_SDRAM_PARAM(emc_pin_program_wait);
	CASE_GET_SDRAM_PARAM(emc_rc);
	CASE_GET_SDRAM_PARAM(emc_rfc);
	CASE_GET_SDRAM_PARAM(emc_ras);
	CASE_GET_SDRAM_PARAM(emc_rp);
	CASE_GET_SDRAM_PARAM(emc_r2w);
	CASE_GET_SDRAM_PARAM(emc_w2r);
	CASE_GET_SDRAM_PARAM(emc_r2p);
	CASE_GET_SDRAM_PARAM(emc_w2p);
	CASE_GET_SDRAM_PARAM(emc_rd_rcd);
	CASE_GET_SDRAM_PARAM(emc_wr_rcd);
	CASE_GET_SDRAM_PARAM(emc_rrd);
	CASE_GET_SDRAM_PARAM(emc_rext);
	CASE_GET_SDRAM_PARAM(emc_wdv);
	CASE_GET_SDRAM_PARAM(emc_quse);
	CASE_GET_SDRAM_PARAM(emc_qrst);
	CASE_GET_SDRAM_PARAM(emc_qsafe);
	CASE_GET_SDRAM_PARAM(emc_rdv);
	CASE_GET_SDRAM_PARAM(emc_refresh);
	CASE_GET_SDRAM_PARAM(emc_burst_refresh_num);
	CASE_GET_SDRAM_PARAM(emc_pdex2wr);
	CASE_GET_SDRAM_PARAM(emc_pdex2rd);
	CASE_GET_SDRAM_PARAM(emc_pchg2pden);
	CASE_GET_SDRAM_PARAM(emc_act2pden);
	CASE_GET_SDRAM_PARAM(emc_ar2pden);
	CASE_GET_SDRAM_PARAM(emc_rw2pden);
	CASE_GET_SDRAM_PARAM(emc_txsr);
	CASE_GET_SDRAM_PARAM(emc_tcke);
	CASE_GET_SDRAM_PARAM(emc_tfaw);
	CASE_GET_SDRAM_PARAM(emc_trpab);
	CASE_GET_SDRAM_PARAM(emc_tclkstable);
	CASE_GET_SDRAM_PARAM(emc_tclkstop);
	CASE_GET_SDRAM_PARAM(emc_trefbw);
	CASE_GET_SDRAM_PARAM(emc_quse_extra);
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg1);
	CASE_GET_SDRAM_PARAM(emc_fbio_dqsib_dly);
	CASE_GET_SDRAM_PARAM(emc_fbio_dqsib_dly_msb);
	CASE_GET_SDRAM_PARAM(emc_fbio_quse_dly);
	CASE_GET_SDRAM_PARAM(emc_fbio_quse_dly_msb);
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg5);
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg6);
	CASE_GET_SDRAM_PARAM(emc_fbio_spare);
	CASE_GET_SDRAM_PARAM(emc_mrs);
	CASE_GET_SDRAM_PARAM(emc_emrs);
	CASE_GET_SDRAM_PARAM(emc_mrw1);
	CASE_GET_SDRAM_PARAM(emc_mrw2);
	CASE_GET_SDRAM_PARAM(emc_mrw3);
	CASE_GET_SDRAM_PARAM(emc_mrw_reset_command);
	CASE_GET_SDRAM_PARAM(emc_mrw_reset_ninit_wait);
	CASE_GET_SDRAM_PARAM(emc_adr_cfg);
	CASE_GET_SDRAM_PARAM(emc_adr_cfg1);
	CASE_GET_SDRAM_PARAM(mc_emem_cfg);
	CASE_GET_SDRAM_PARAM(mc_lowlatency_config);
	CASE_GET_SDRAM_PARAM(emc_cfg);
	CASE_GET_SDRAM_PARAM(emc_cfg2);
	CASE_GET_SDRAM_PARAM(emc_dbg);
	CASE_GET_SDRAM_PARAM(ahb_arbitration_xbar_ctrl);
	CASE_GET_SDRAM_PARAM(emc_cfg_dig_dll);
	CASE_GET_SDRAM_PARAM(emc_dll_xform_dqs);
	CASE_GET_SDRAM_PARAM(emc_dll_xform_quse);
	CASE_GET_SDRAM_PARAM(warm_boot_wait);
	CASE_GET_SDRAM_PARAM(emc_ctt_term_ctrl);
	CASE_GET_SDRAM_PARAM(emc_odt_write);
	CASE_GET_SDRAM_PARAM(emc_odt_read);
	CASE_GET_SDRAM_PARAM(emc_zcal_ref_cnt);
	CASE_GET_SDRAM_PARAM(emc_zcal_wait_cnt);
	CASE_GET_SDRAM_PARAM(emc_zcal_mrw_cmd);
	CASE_GET_SDRAM_PARAM(emc_mrs_reset_dll);
	CASE_GET_SDRAM_PARAM(emc_mrw_zq_init_dev0);
	CASE_GET_SDRAM_PARAM(emc_mrw_zq_init_dev1);
	CASE_GET_SDRAM_PARAM(emc_mrw_zq_init_wait);
	CASE_GET_SDRAM_PARAM(emc_mrs_reset_dll_wait);
	CASE_GET_SDRAM_PARAM(emc_emrs_emr2);
	CASE_GET_SDRAM_PARAM(emc_emrs_emr3);
	CASE_GET_SDRAM_PARAM(emc_emrs_ddr2_dll_enable);
	CASE_GET_SDRAM_PARAM(emc_mrs_ddr2_dll_reset);
	CASE_GET_SDRAM_PARAM(emc_emrs_ddr2_ocd_calib);
	CASE_GET_SDRAM_PARAM(emc_ddr2_wait);
	CASE_GET_SDRAM_PARAM(emc_cfg_clktrim0);
	CASE_GET_SDRAM_PARAM(emc_cfg_clktrim1);
	CASE_GET_SDRAM_PARAM(emc_cfg_clktrim2);
	CASE_GET_SDRAM_PARAM(pmc_ddr_pwr);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfga_pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfgc_pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfgc_pad_ctrl2);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfgd_pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfgd_pad_ctrl2);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2clkcfg_Pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2comp_pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2vttgen_pad_ctrl);
	DEFAULT();
	}
	return 0;
}

int
t20_getbl_param(u_int32_t set,
		parse_token id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;

	if (set >= NVBOOT_MAX_BOOTLOADERS)
		return -ENODATA;
	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {
	CASE_GET_BL_PARAM(version);
	CASE_GET_BL_PARAM(start_blk);
	CASE_GET_BL_PARAM(start_page);
	CASE_GET_BL_PARAM(length);
	CASE_GET_BL_PARAM(load_addr);
	CASE_GET_BL_PARAM(entry_point);
	CASE_GET_BL_PARAM(attribute);

	case token_bl_crypto_hash:
		memcpy(data,
		&(bct_ptr->bootloader[set].crypto_hash),
		sizeof(nvboot_hash));
	break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t20_setbl_param(u_int32_t set,
		parse_token id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;

	if (set >= NVBOOT_MAX_BOOTLOADERS)
		return -ENODATA;
	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {
	CASE_SET_BL_PARAM(version);
	CASE_SET_BL_PARAM(start_blk);
	CASE_SET_BL_PARAM(start_page);
	CASE_SET_BL_PARAM(length);
	CASE_SET_BL_PARAM(load_addr);
	CASE_SET_BL_PARAM(entry_point);
	CASE_SET_BL_PARAM(attribute);

	case token_bl_crypto_hash:
		memcpy(&(bct_ptr->bootloader[set].crypto_hash),
		data,
		sizeof(nvboot_hash));
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t20_bct_get_value(parse_token id, u_int32_t *data, u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;
	nvboot_config_table  samplebct; /* Used for computing offsets. */

	/*
	 * Note: Not all queries require use of the BCT, so testing for a
	 *       valid BCT is distributed within the code.
	 */
	if (data == NULL)
		return -ENODATA;

	switch (id) {
	/*
	 * Simple BCT fields
	 */
	CASE_GET_NVU32(boot_data_version);
	CASE_GET_NVU32(block_size_log2);
	CASE_GET_NVU32(page_size_log2);
	CASE_GET_NVU32(partition_size);
	CASE_GET_NVU32(num_param_sets);
	CASE_GET_NVU32(num_sdram_sets);
	CASE_GET_NVU32(bootloader_used);
	CASE_GET_NVU32(odm_data);

	/*
	 * Constants.
	 */

	CASE_GET_CONST(bootloaders_max,   NVBOOT_MAX_BOOTLOADERS);
	CASE_GET_CONST(reserved_size,     NVBOOT_BCT_RESERVED_SIZE);

	case token_reserved_offset:
		*data = (u_int8_t *)&(samplebct.reserved)
				- (u_int8_t *)&samplebct;
		break;

	case token_bct_size:
		*data = sizeof(nvboot_config_table);
		break;

	CASE_GET_CONST(hash_size, sizeof(nvboot_hash));

	case token_crypto_offset:
		/* Offset to region in BCT to encrypt & sign */
		*data = (u_int8_t *)&(samplebct.random_aes_blk)
				- (u_int8_t *)&samplebct;
		break;

	case token_crypto_length:
		/* size   of region in BCT to encrypt & sign */
		*data = sizeof(nvboot_config_table) - sizeof(nvboot_hash);
	break;

	CASE_GET_CONST(max_bct_search_blks, NVBOOT_MAX_BCT_SEARCH_BLOCKS);

	CASE_GET_CONST_PREFIX(dev_type_nand, nvboot);
	CASE_GET_CONST_PREFIX(dev_type_sdmmc, nvboot);
	CASE_GET_CONST_PREFIX(dev_type_spi, nvboot);
	CASE_GET_CONST_PREFIX(sdmmc_data_width_4bit, nvboot);
	CASE_GET_CONST_PREFIX(sdmmc_data_width_8bit, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllp_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllc_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllm_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_clockm, nvboot);

	CASE_GET_CONST_PREFIX(memory_type_none, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_ddr, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_lpddr, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_ddr2, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_lpddr2, nvboot);

	default:
		return -ENODATA;
	}
	return 0;
}

int
t20_bct_set_value(parse_token id, u_int32_t  data, u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;

	if (bct == NULL)
		return -ENODATA;

	switch (id) {
	/*
	 * Simple BCT fields
	 */
	CASE_SET_NVU32(boot_data_version);
	CASE_SET_NVU32(block_size_log2);
	CASE_SET_NVU32(page_size_log2);
	CASE_SET_NVU32(partition_size);
	CASE_SET_NVU32(num_param_sets);
	CASE_SET_NVU32(num_sdram_sets);
	CASE_SET_NVU32(bootloader_used);
	CASE_SET_NVU32(odm_data);

	default:
		return -ENODATA;
	}

	return 0;
}

int
t20_bct_set_data(parse_token id,
	u_int8_t *data,
	u_int32_t  length,
	u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;

	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {

	CASE_SET_DATA(crypto_hash, sizeof(nvboot_hash));

	default:
		return -ENODATA;
	}

	return 0;
}

void t20_init_bad_block_table(build_image_context *context)
{
	u_int32_t bytes_per_entry;
	nvboot_badblock_table *table;
	nvboot_config_table *bct;

	bct = (nvboot_config_table *)(context->bct);

	assert(context != NULL);
	assert(bct != NULL);

	table = &bct->badblock_table;

	bytes_per_entry = ICEIL(context->partition_size,
				NVBOOT_BAD_BLOCK_TABLE_SIZE);
	table->block_size_log2 = context->block_size_log2;
	table->virtual_blk_size_log2 = NV_MAX(ceil_log2(bytes_per_entry),
					table->block_size_log2);
	table->entries_used = iceil_log2(context->partition_size,
					table->virtual_blk_size_log2);
}

cbootimage_soc_config tegra20_config = {
	.init_bad_block_table		= t20_init_bad_block_table,
	.set_dev_param				= t20_set_dev_param,
	.get_dev_param				= t20_get_dev_param,
	.set_sdram_param			= t20_set_sdram_param,
	.get_sdram_param			= t20_get_sdram_param,
	.setbl_param				= t20_setbl_param,
	.getbl_param				= t20_getbl_param,
	.set_value					= t20_bct_set_value,
	.get_value					= t20_bct_get_value,
	.set_data					= t20_bct_set_data,

	.devtype_table				= s_devtype_table_t20,
	.sdmmc_data_width_table		= s_sdmmc_data_width_table_t20,
	.spi_clock_source_table		= s_spi_clock_source_table_t20,
	.nvboot_memory_type_table	= s_nvboot_memory_type_table_t20,
	.sdram_field_table			= s_sdram_field_table_t20,
	.nand_table					= s_nand_table_t20,
	.sdmmc_table				= s_sdmmc_table_t20,
	.spiflash_table				= s_spiflash_table_t20,
	.device_type_table			= s_device_type_table_t20,
};

void t20_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config)
{
	context->boot_data_version = BOOTDATA_VERSION_T20;
	*soc_config = &tegra20_config;
}

int if_bct_is_t20_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config)
{
	nvboot_config_table * bct = (nvboot_config_table *) context->bct;

	if (bct->boot_data_version == BOOTDATA_VERSION_T20)
	{
		t20_get_soc_config(context, soc_config);
		return 1;
	}
	return 0;
}
