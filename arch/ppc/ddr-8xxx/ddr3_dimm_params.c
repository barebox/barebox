/*
 * Copyright 2008-2012 Freescale Semiconductor, Inc.
 *	Dave Liu <daveliu@freescale.com>
 *
 * calculate the organization and timing parameter
 * from ddr3 spd, please refer to the spec
 * JEDEC standard No.21-C 4_01_02_11R18.pdf
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <asm/fsl_ddr_sdram.h>
#include "ddr.h"

/*
 * Calculate the Density of each Physical Rank.
 * Returned size is in bytes.
 *
 * each rank size =
 * sdram capacity(bit) / 8 * primary bus width / sdram width
 *
 * where: sdram capacity  = spd byte4[3:0]
 *        primary bus width = spd byte8[2:0]
 *        sdram width = spd byte7[2:0]
 *
 * SPD byte4 - sdram density and banks
 *	bit[3:0]	size(bit)	size(byte)
 *	0000		256Mb		32MB
 *	0001		512Mb		64MB
 *	0010		1Gb		128MB
 *	0011		2Gb		256MB
 *	0100		4Gb		512MB
 *	0101		8Gb		1GB
 *	0110		16Gb		2GB
 *
 * SPD byte8 - module memory bus width
 *	bit[2:0]	primary bus width
 *	000		8bits
 *	001		16bits
 *	010		32bits
 *	011		64bits
 *
 * SPD byte7 - module organiztion
 *	bit[2:0]	sdram device width
 *	000		4bits
 *	001		8bits
 *	010		16bits
 *	011		32bits
 */
static uint64_t compute_ranksize(const struct ddr3_spd_eeprom_s *spd)
{
	uint64_t bsize;
	int sdram_cap_bsize = 0, prim_bus_width = 0, sdram_width = 0;

	if ((spd->density_banks & 0xf) < 7)
		sdram_cap_bsize = (spd->density_banks & 0xf) + 28;
	if ((spd->bus_width & 0x7) < 4)
		prim_bus_width = (spd->bus_width & 0x7) + 3;
	if ((spd->organization & 0x7) < 4)
		sdram_width = (spd->organization & 0x7) + 2;

	bsize = 1ULL << (sdram_cap_bsize - 3 + prim_bus_width - sdram_width);

	return bsize;
}

/*
 * compute_dimm_parameters for DDR3 SPD
 *
 * Compute DIMM parameters based upon the SPD information in spd.
 * Writes the results to the dimm_params_s structure pointed by pdimm.
 *
 */
uint32_t
compute_dimm_parameters(const generic_spd_eeprom_t *spdin,
		struct dimm_params_s *pdimm)
{
	const struct ddr3_spd_eeprom_s *spd = spdin;
	uint32_t retval, mtb_ps;
	int ftb_tmp;

	if (spd->mem_type != SPD_MEMTYPE_DDR3)
		goto error;

	retval = ddr3_spd_checksum_pass(spd);
	if (retval)
		goto error;

	memset(pdimm->mpart, 0, sizeof(pdimm->mpart));
	if ((spd->info_size_crc & 0xf) > 1)
		memcpy(pdimm->mpart, spd->mpart, sizeof(pdimm->mpart) - 1);

	/* DIMM organization parameters */
	pdimm->n_ranks = ((spd->organization >> 3) & 0x7) + 1;
	pdimm->rank_density = compute_ranksize(spd);
	pdimm->capacity = pdimm->n_ranks * pdimm->rank_density;
	pdimm->data_width = pdimm->primary_sdram_width + pdimm->ec_sdram_width;
	pdimm->primary_sdram_width = 1 << (3 + (spd->bus_width & 0x7));
	if ((spd->bus_width >> 3) & 0x3)
		pdimm->ec_sdram_width = 8;
	else
		pdimm->ec_sdram_width = 0;
	pdimm->device_width = 1 << ((spd->organization & 0x7) + 2);

	/* These are the types defined by the JEDEC DDR3 SPD spec */
	pdimm->mirrored_dimm = 0;
	pdimm->registered_dimm = 0;
	switch (spd->module_type & DDR3_SPD_MODULETYPE_MASK) {
	case DDR3_SPD_MODULETYPE_UDIMM:
		/* Unbuffered DIMMs */
		if (spd->mod_section.unbuffered.addr_mapping & 0x1)
			pdimm->mirrored_dimm = 1;
		break;

	default:
		goto error;
	}

	/* SDRAM device parameters */
	pdimm->n_row_addr = ((spd->addressing >> 3) & 0x7) + 12;
	pdimm->n_col_addr = (spd->addressing & 0x7) + 9;
	pdimm->n_banks_per_sdram_device =
			8 << ((spd->density_banks >> 4) & 0x7);

	/*
	 * The SPD spec does not define an ECC bit. The DIMM is considered
	 * to have ECC capability if the extension bus exists.
	 */
	if (pdimm->ec_sdram_width)
		pdimm->edc_config = 0x02;
	else
		pdimm->edc_config = 0x00;

	/*
	 * The SPD spec does not define the burst length byte.
	 * but the DDR3 spec defines BL8 and BC4, on bit 3 and
	 * bit 2.
	 */
	pdimm->burst_lengths_bitmask = 0x0c;
	pdimm->row_density = __ilog2(pdimm->rank_density);

	mtb_ps = (spd->mtb_dividend * 1000) / spd->mtb_divisor;
	pdimm->mtb_ps = mtb_ps;

	ftb_tmp = spd->ftb_div & 0xf0;
	pdimm->ftb_10th_ps = ((ftb_tmp >> 4) * 10) / ftb_tmp;

	pdimm->tCKmin_X_ps = spd->tck_min * mtb_ps +
		(spd->fine_tck_min * ftb_tmp) / 10;
	pdimm->caslat_X  = ((spd->caslat_msb << 8) | spd->caslat_lsb) << 4;

	pdimm->taa_ps = spd->taa_min * mtb_ps +
		(spd->fine_taa_min * ftb_tmp) / 10;

	pdimm->tRCD_ps = spd->trcd_min * mtb_ps +
		(spd->fine_trcd_min * ftb_tmp) / 10;

	pdimm->tRP_ps = spd->trp_min * mtb_ps +
		(spd->fine_trp_min * ftb_tmp) / 10;
	pdimm->tRAS_ps = (((spd->tras_trc_ext & 0xf) << 8) | spd->tras_min_lsb)
			* mtb_ps;
	pdimm->tWR_ps = spd->twr_min * mtb_ps;
	pdimm->tWTR_ps = spd->twtr_min * mtb_ps;
	pdimm->tRFC_ps = ((spd->trfc_min_msb << 8) | spd->trfc_min_lsb)
			* mtb_ps;
	pdimm->tRRD_ps = spd->trrd_min * mtb_ps;
	pdimm->tRC_ps = (((spd->tras_trc_ext & 0xf0) << 4) | spd->trc_min_lsb);
	pdimm->tRC_ps *= mtb_ps;
	pdimm->tRC_ps += (spd->fine_trc_min * ftb_tmp) / 10;

	pdimm->tRTP_ps = spd->trtp_min * mtb_ps;

	/*
	 * Average periodic refresh interval
	 * tREFI = 7.8 us at normal temperature range
	 *       = 3.9 us at ext temperature range
	 */
	pdimm->refresh_rate_ps = 7800000;
	if ((spd->therm_ref_opt & 0x1) && !(spd->therm_ref_opt & 0x2)) {
		pdimm->refresh_rate_ps = 3900000;
		pdimm->extended_op_srt = 1;
	}

	pdimm->tfaw_ps = (((spd->tfaw_msb & 0xf) << 8) | spd->tfaw_min)
			* mtb_ps;

	return 0;
error:
	return 1;
}
