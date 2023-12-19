// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2008-2014 Freescale Semiconductor, Inc.
 */

/*
 * Generic driver for Freescale DDR/DDR2/DDR3 memory controller.
 * Based on code from spd_sdram.c
 * Author: James Yang [at freescale.com]
 */
#include <common.h>
#include <soc/fsl/fsl_ddr_sdram.h>
#include <linux/log2.h>
#include "fsl_ddr.h"

enum ddr_endianess ddr_endianess;

/*
 * ASSUMPTIONS:
 *    - Same number of CONFIG_DIMM_SLOTS_PER_CTLR on each controller
 *    - Same memory data bus width on all controllers
 *
 * NOTES:
 *
 * The memory controller and associated documentation use confusing
 * terminology when referring to the orgranization of DRAM.
 *
 * Here is a terminology translation table:
 *
 * memory controller/documention  |industry   |this code  |signals
 * -------------------------------|-----------|-----------|-----------------
 * physical bank/bank		  |rank       |rank	  |chip select (CS)
 * logical bank/sub-bank	  |bank       |bank	  |bank address (BA)
 * page/row			  |row	      |page	  |row address
 * ???				  |column     |column	  |column address
 *
 * The naming confusion is further exacerbated by the descriptions of the
 * memory controller interleaving feature, where accesses are interleaved
 * _BETWEEN_ two seperate memory controllers.  This is configured only in
 * CS0_CONFIG[INTLV_CTL] of each memory controller.
 *
 * memory controller documentation | number of chip selects
 *				   | per memory controller supported
 * --------------------------------|-----------------------------------------
 * cache line interleaving	   | 1 (CS0 only)
 * page interleaving		   | 1 (CS0 only)
 * bank interleaving		   | 1 (CS0 only)
 * superbank interleraving	   | depends on bank (chip select)
 *				   |   interleraving [rank interleaving]
 *				   |   mode used on every memory controller
 *
 * Even further confusing is the existence of the interleaving feature
 * _WITHIN_ each memory controller.  The feature is referred to in
 * documentation as chip select interleaving or bank interleaving,
 * although it is configured in the DDR_SDRAM_CFG field.
 *
 * Name of field		| documentation name	| this code
 * -----------------------------|-----------------------|------------------
 * DDR_SDRAM_CFG[BA_INTLV_CTL]	| Bank (chip select)	| rank interleaving
 *				|  interleaving
 */

static unsigned long long step_assign_addresses_linear(struct fsl_ddr_info *pinfo,
						       unsigned long long current_mem_base)
{
	int i, j;
	unsigned long long total_mem = 0;

	/*
	 * Simple linear assignment if memory
	 * controllers are not interleaved.
	 */
	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];
		unsigned long long total_ctlr_mem = 0;

		c->common_timing_params.base_address = current_mem_base;

		for (j = 0; j < c->dimm_slots_per_ctrl; j++) {
			/* Compute DIMM base addresses. */
			unsigned long long cap = c->dimm_params[j].capacity >>
					pinfo->c[i].dbw_capacity_adjust;

			c->dimm_params[j].base_address = current_mem_base;
			debug("ctrl %d dimm %d base 0x%llx\n", i, j, current_mem_base);
			current_mem_base += cap;
			total_ctlr_mem += cap;
		}
		debug("ctrl %d total 0x%llx\n", i, total_ctlr_mem);
		c->common_timing_params.total_mem = total_ctlr_mem;
		total_mem += total_ctlr_mem;
	}

	return total_mem;
}

static unsigned long long step_assign_addresses_interleaved(struct fsl_ddr_info *pinfo,
							    unsigned long long current_mem_base)
{
	unsigned long long total_mem, total_ctlr_mem;
	unsigned long long rank_density, ctlr_density = 0;
	int i;

	rank_density = pinfo->c[0].dimm_params[0].rank_density >>
				pinfo->c[0].dbw_capacity_adjust;

	switch (pinfo->c[0].memctl_opts.ba_intlv_ctl &
				FSL_DDR_CS0_CS1_CS2_CS3) {
	case FSL_DDR_CS0_CS1_CS2_CS3:
		ctlr_density = 4 * rank_density;
		break;
	case FSL_DDR_CS0_CS1:
	case FSL_DDR_CS0_CS1_AND_CS2_CS3:
		ctlr_density = 2 * rank_density;
		break;
	case FSL_DDR_CS2_CS3:
	default:
		ctlr_density = rank_density;
		break;
	}

	debug("rank density is 0x%llx, ctlr density is 0x%llx\n",
		rank_density, ctlr_density);

	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];

		if (c->memctl_opts.memctl_interleaving) {
			switch (c->memctl_opts.memctl_interleaving_mode) {
			case FSL_DDR_256B_INTERLEAVING:
			case FSL_DDR_CACHE_LINE_INTERLEAVING:
			case FSL_DDR_PAGE_INTERLEAVING:
			case FSL_DDR_BANK_INTERLEAVING:
			case FSL_DDR_SUPERBANK_INTERLEAVING:
				total_ctlr_mem = 2 * ctlr_density;
				break;
			case FSL_DDR_3WAY_1KB_INTERLEAVING:
			case FSL_DDR_3WAY_4KB_INTERLEAVING:
			case FSL_DDR_3WAY_8KB_INTERLEAVING:
				total_ctlr_mem = 3 * ctlr_density;
				break;
			case FSL_DDR_4WAY_1KB_INTERLEAVING:
			case FSL_DDR_4WAY_4KB_INTERLEAVING:
			case FSL_DDR_4WAY_8KB_INTERLEAVING:
				total_ctlr_mem = 4 * ctlr_density;
				break;
			default:
				panic("Unknown interleaving mode");
			}
			c->common_timing_params.base_address = current_mem_base;
			c->common_timing_params.total_mem = total_ctlr_mem;
			total_mem = current_mem_base + total_ctlr_mem;
			debug("ctrl %d base 0x%llx\n", i, current_mem_base);
			debug("ctrl %d total 0x%llx\n", i, total_ctlr_mem);
		} else {
			total_mem += step_assign_addresses_linear(pinfo, current_mem_base);
		}
	}

	return total_mem;
}

static unsigned long long step_assign_addresses(struct fsl_ddr_info *pinfo)
{
	unsigned int i, j;
	unsigned long long total_mem;

	/*
	 * If a reduced data width is requested, but the SPD
	 * specifies a physically wider device, adjust the
	 * computed dimm capacities accordingly before
	 * assigning addresses.
	 */
	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];
		unsigned int found = 0;

		switch (c->memctl_opts.data_bus_width) {
		case 2:
			/* 16-bit */
			for (j = 0; j < c->dimm_slots_per_ctrl; j++) {
				unsigned int dw;
				if (!c->dimm_params[j].n_ranks)
					continue;
				dw = c->dimm_params[j].primary_sdram_width;
				if ((dw == 72 || dw == 64)) {
					pinfo->c[i].dbw_capacity_adjust = 2;
					break;
				} else if ((dw == 40 || dw == 32)) {
					pinfo->c[i].dbw_capacity_adjust = 1;
					break;
				}
			}
			break;

		case 1:
			/* 32-bit */
			for (j = 0; j < c->dimm_slots_per_ctrl; j++) {
				unsigned int dw;
				dw = c->dimm_params[j].data_width;
				if (c->dimm_params[j].n_ranks
				    && (dw == 72 || dw == 64)) {
					/*
					 * FIXME: can't really do it
					 * like this because this just
					 * further reduces the memory
					 */
					found = 1;
					break;
				}
			}
			if (found)
				pinfo->c[i].dbw_capacity_adjust = 1;

			break;

		case 0:
			/* 64-bit */
			break;

		default:
			printf("unexpected data bus width "
				"specified controller %u\n", i);
			return 1;
		}
		debug("dbw_cap_adj[%d]=%d\n", i, pinfo->c[i].dbw_capacity_adjust);
	}

	if (pinfo->c[0].memctl_opts.memctl_interleaving)
		total_mem = step_assign_addresses_interleaved(pinfo, pinfo->mem_base);
	else
		total_mem = step_assign_addresses_linear(pinfo, pinfo->mem_base);

	debug("Total mem by %s is 0x%llx\n", __func__, total_mem);

	return total_mem;
}

static int compute_dimm_parameters(struct fsl_ddr_controller *c,
				   struct spd_eeprom *spd,
				   struct dimm_params *pdimm)
{
	unsigned int mclk_ps = get_memory_clk_period_ps(c);
	const memctl_options_t *popts = &c->memctl_opts;
	int ret = -EINVAL;

	memset(pdimm, 0, sizeof(*pdimm));

	if (is_ddr1(popts))
		ret = ddr1_compute_dimm_parameters(mclk_ps, (void *)spd, pdimm);
	else if (is_ddr2(popts))
		ret = ddr2_compute_dimm_parameters(mclk_ps, (void *)spd, pdimm);
	else if (is_ddr3(popts))
		ret = ddr3_compute_dimm_parameters((void *)spd, pdimm);
	else if (is_ddr4(popts))
		ret = ddr4_compute_dimm_parameters((void *)spd, pdimm);

	return ret;
}

static unsigned long long fsl_ddr_compute(struct fsl_ddr_info *pinfo)
{
	unsigned int i, j;
	unsigned long long total_mem = 0;
	int assert_reset = 0;
	int retval;
	unsigned int max_end = 0;

	/* STEP 2:  Compute DIMM parameters from SPD data */
	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];

		if (!c->spd_installed_dimms)
			continue;

		for (j = 0; j < c->dimm_slots_per_ctrl; j++) {
			struct spd_eeprom *spd = &c->spd_installed_dimms[j];
			struct dimm_params *pdimm = &c->dimm_params[j];

			retval = compute_dimm_parameters(c, spd, pdimm);
			if (retval == 2) {
				printf("Error: compute_dimm_parameters"
				" non-zero returned FATAL value "
				"for memctl=%u dimm=%u\n", i, j);
				return 0;
			}
			if (retval) {
				debug("Warning: compute_dimm_parameters"
				" non-zero return value for memctl=%u "
				"dimm=%u\n", i, j);
			}
		}
	}

	/*
	 * STEP 3: Compute a common set of timing parameters
	 * suitable for all of the DIMMs on each memory controller
	 */
	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];

		debug("Computing lowest common DIMM parameters for memctl=%u\n",
		      i);
		compute_lowest_common_dimm_parameters(c);
	}

	/* STEP 4:  Gather configuration requirements from user */
	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];

		debug("Reloading memory controller "
			"configuration options for memctl=%u\n", i);
		/*
		 * This "reloads" the memory controller options
		 * to defaults.  If the user "edits" an option,
		 * next_step points to the step after this,
		 * which is currently STEP_ASSIGN_ADDRESSES.
		 */
		populate_memctl_options(c);
		/*
		 * For RDIMMs, JEDEC spec requires clocks to be stable
		 * before reset signal is deasserted. For the boards
		 * using fixed parameters, this function should be
		 * be called from board init file.
		 */
		if (c->common_timing_params.all_dimms_registered)
			assert_reset = 1;
	}

	/* STEP 5:  Assign addresses to chip selects */
	check_interleaving_options(pinfo);
	total_mem = step_assign_addresses(pinfo);
	debug("Total mem %llu assigned\n", total_mem);

	/* STEP 6:  compute controller register values */
	debug("FSL Memory ctrl register computation\n");
	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];

		if (c->common_timing_params.ndimms_present == 0) {
			memset(&c->fsl_ddr_config_reg, 0,
				sizeof(fsl_ddr_cfg_regs_t));
			continue;
		}

		compute_fsl_memctl_config_regs(c);
	}

	/*
	 * Compute the amount of memory available just by
	 * looking for the highest valid CSn_BNDS value.
	 * This allows us to also experiment with using
	 * only CS0 when using dual-rank DIMMs.
	 */

	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];

		for (j = 0; j < c->chip_selects_per_ctrl; j++) {
			fsl_ddr_cfg_regs_t *reg = &c->fsl_ddr_config_reg;
			if (reg->cs[j].config & 0x80000000) {
				unsigned int end;
				/*
				 * 0xfffffff is a special value we put
				 * for unused bnds
				 */
				if (reg->cs[j].bnds == 0xffffffff)
					continue;
				end = reg->cs[j].bnds & 0xffff;
				if (end > max_end) {
					max_end = end;
				}
			}
		}
	}

	total_mem = 1 + (((unsigned long long)max_end << 24ULL) |
		    0xFFFFFFULL) - pinfo->mem_base;

	return total_mem;
}

phys_size_t fsl_ddr_sdram(struct fsl_ddr_info *pinfo, bool little_endian)
{
	unsigned int i;
	unsigned long long total_memory;
	int deassert_reset = 0;

	if (little_endian)
		ddr_endianess = DDR_ENDIANESS_LE;
	else
		ddr_endianess = DDR_ENDIANESS_BE;

	total_memory = fsl_ddr_compute(pinfo);

	/* setup 3-way interleaving before enabling DDRC */
	switch (pinfo->c[0].memctl_opts.memctl_interleaving_mode) {
	case FSL_DDR_3WAY_1KB_INTERLEAVING:
	case FSL_DDR_3WAY_4KB_INTERLEAVING:
	case FSL_DDR_3WAY_8KB_INTERLEAVING:
		fsl_ddr_set_intl3r(
			pinfo->c[0].memctl_opts.
			memctl_interleaving_mode);
		break;
	default:
		break;
	}

	/*
	 * Program configuration registers.
	 * JEDEC specs requires clocks to be stable before deasserting reset
	 * for RDIMMs. Clocks start after chip select is enabled and clock
	 * control register is set. During step 1, all controllers have their
	 * registers set but not enabled. Step 2 proceeds after deasserting
	 * reset through board FPGA or GPIO.
	 * For non-registered DIMMs, initialization can go through but it is
	 * also OK to follow the same flow.
	 */
	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];

		if (c->common_timing_params.all_dimms_registered)
			deassert_reset = 1;
	}
	for (i = 0; i < pinfo->num_ctrls; i++) {
		struct fsl_ddr_controller *c = &pinfo->c[i];

		debug("Programming controller %u\n", i);
		if (c->common_timing_params.ndimms_present == 0) {
			debug("No dimms present on controller %u; "
					"skipping programming\n", i);
			continue;
		}
		/*
		 * The following call with step = 1 returns before enabling
		 * the controller. It has to finish with step = 2 later.
		 */
		fsl_ddr_set_memctl_regs(c, deassert_reset ? 1 : 0, little_endian);
	}
	if (deassert_reset) {
		for (i = 0; i < pinfo->num_ctrls; i++) {
			struct fsl_ddr_controller *c = &pinfo->c[i];

			/* Call with step = 2 to continue initialization */
			fsl_ddr_set_memctl_regs(c, 2, little_endian);
		}
	}

	debug("total_memory by %s = %llu\n", __func__, total_memory);

	return total_memory;
}
