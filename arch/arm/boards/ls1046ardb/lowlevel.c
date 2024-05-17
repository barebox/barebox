// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <mach/layerscape/debug_ll.h>
#include <debug_ll.h>
#include <ddr_spd.h>
#include <image-metadata.h>
#include <pbl/i2c.h>
#include <soc/fsl/fsl_ddr_sdram.h>
#include <soc/fsl/immap_lsch2.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/syscounter.h>
#include <asm/cache.h>
#include <mach/layerscape/errata.h>
#include <mach/layerscape/lowlevel.h>
#include <mach/layerscape/xload.h>
#include <mach/layerscape/layerscape.h>

struct board_specific_parameters {
	u32 n_ranks;
	u32 datarate_mhz_high;
	u32 rank_gb;
	u32 clk_adjust;
	u32 wrlvl_start;
	u32 wrlvl_ctl_2;
	u32 wrlvl_ctl_3;
};

/*
 * These tables contain all valid speeds we want to override with board
 * specific parameters. datarate_mhz_high values need to be in ascending order
 * for each n_ranks group.
 */
static const struct board_specific_parameters udimm0[] = {
	/*
	 * memory controller 0
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
	{2,  1350, 0, 8,     6, 0x0708090B, 0x0C0D0E09,},
	{2,  1666, 0, 8,     7, 0x08090A0C, 0x0D0F100B,},
	{2,  1900, 0, 8,     7, 0x09090B0D, 0x0E10120B,},
	{2,  2300, 0, 8,     9, 0x0A0B0C10, 0x1213140E,},
	{}
};

static const struct board_specific_parameters *udimms[] = {
	udimm0,
};

static const struct board_specific_parameters rdimm0[] = {
	/*
	 * memory controller 0
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
	{2,  1666, 0, 0x8,     0x0D, 0x0C0B0A08, 0x0A0B0C08,},
	{2,  1900, 0, 0x8,     0x0E, 0x0D0C0B09, 0x0B0C0D09,},
	{2,  2300, 0, 0xa,     0x12, 0x100F0D0C, 0x0E0F100C,},
	{1,  1666, 0, 0x8,     0x0D, 0x0C0B0A08, 0x0A0B0C08,},
	{1,  1900, 0, 0x8,     0x0E, 0x0D0C0B09, 0x0B0C0D09,},
	{1,  2300, 0, 0xa,     0x12, 0x100F0D0C, 0x0E0F100C,},
	{}
};

static const struct board_specific_parameters *rdimms[] = {
	rdimm0,
};

static void ddr_board_options(memctl_options_t *popts,
			      struct dimm_params *pdimm,
			      struct fsl_ddr_controller *c)
{
	const struct board_specific_parameters *pbsp, *pbsp_highest = NULL;
	unsigned long ddr_freq;

	if (!pdimm->n_ranks)
		return;

	if (popts->registered_dimm_en)
		pbsp = rdimms[0];
	else
		pbsp = udimms[0];

	/* Get clk_adjust, wrlvl_start, wrlvl_ctl, according to the board ddr
	 * freqency and n_banks specified in board_specific_parameters table.
	 */
	ddr_freq = c->ddr_freq / 1000000;
	while (pbsp->datarate_mhz_high) {
		if (pbsp->n_ranks == pdimm->n_ranks) {
			if (ddr_freq <= pbsp->datarate_mhz_high) {
				popts->clk_adjust = pbsp->clk_adjust;
				popts->wrlvl_start = pbsp->wrlvl_start;
				popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
				popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
				goto found;
			}
			pbsp_highest = pbsp;
		}
		pbsp++;
	}

	if (pbsp_highest) {
		printf("Error: board specific timing not found for %lu MT/s\n",
		       ddr_freq);
		printf("Trying to use the highest speed (%u) parameters\n",
		       pbsp_highest->datarate_mhz_high);
		popts->clk_adjust = pbsp_highest->clk_adjust;
		popts->wrlvl_start = pbsp_highest->wrlvl_start;
		popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
		popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
	} else {
		panic("DIMM is not supported by this board");
	}
found:
	debug("Found timing match: n_ranks %d, data rate %d, rank_gb %d\n",
	      pbsp->n_ranks, pbsp->datarate_mhz_high, pbsp->rank_gb);

	popts->data_bus_width = 0;	/* 64-bit data bus */
	popts->bstopre = 0;		/* enable auto precharge */

	/*
	 * Factors to consider for half-strength driver enable:
	 *	- number of DIMMs installed
	 */
	popts->half_strength_driver_enable = 0;
	/*
	 * Write leveling override
	 */
	popts->wrlvl_override = 1;
	popts->wrlvl_sample = 0xf;

	/*
	 * Rtt and Rtt_WR override
	 */
	popts->rtt_override = 0;

	/* Enable ZQ calibration */
	popts->zq_en = 1;

	popts->ddr_cdr1 = DDR_CDR1_DHC_EN | DDR_CDR1_ODT(DDR4_CDR_ODT_80ohm);
	popts->ddr_cdr2 = DDR_CDR2_ODT(DDR4_CDR_ODT_80ohm) |
			  DDR_CDR2_VREF_TRAIN_EN | DDR_CDR2_VREF_RANGE_2;

	/* optimize cpo for erratum A-009942 */
	popts->cpo_sample = 0x61;
}

extern char __dtb_z_fsl_ls1046a_rdb_start[];

static struct spd_eeprom spd_eeprom[] = {
	{
		/* filled during runtime */
	},
};

static struct dimm_params dimm_params[] = {
	{
		/* filled during runtime */
	},
};

static struct fsl_ddr_controller ddrc[] = {
	{
		.dimm_slots_per_ctrl = ARRAY_SIZE(dimm_params),
		.spd_installed_dimms = spd_eeprom,
		.dimm_params = dimm_params,
		.memctl_opts.ddrtype = SDRAM_TYPE_DDR4,
		.base = IOMEM(LSCH2_DDR_ADDR),
		.ddr_freq = LS1046A_DDR_FREQ,
		.erratum_A008511 = 1,
		.erratum_A009803 = 1,
		.erratum_A010165 = 1,
		.erratum_A009801 = 1,
		.erratum_A009942 = 1,
		.chip_selects_per_ctrl = 4,
		.board_options = ddr_board_options,
	},
};

static struct fsl_ddr_info ls1046a_info = {
	.num_ctrls = ARRAY_SIZE(ddrc),
	.c = ddrc,
};

static noinline __noreturn void ls1046ardb_r_entry(unsigned long memsize)
{
	unsigned long membase = LS1046A_DDR_SDRAM_BASE;
	struct pbl_i2c *i2c;
	int ret;

	if (get_pc() >= membase) {
		if (memsize + membase >= 0x100000000)
			memsize = 0x100000000 - membase;

		barebox_arm_entry(membase, 0x80000000 - SZ_64M,
				  __dtb_z_fsl_ls1046a_rdb_start);
	}

	arm_cpu_lowlevel_init();
	ls1046a_debug_ll_init();
	ls1046a_init_lowlevel();

	i2c = ls1046_i2c_init(IOMEM(LSCH2_I2C1_BASE_ADDR));
	ret = spd_read_eeprom(i2c, 0x51, &spd_eeprom, SPD_MEMTYPE_DDR4);
	if (ret) {
		pr_err("Cannot read SPD EEPROM: %d\n", ret);
		goto err;
	}

	memsize = fsl_ddr_sdram(&ls1046a_info, false);

	ls1046a_errata_post_ddr();

	ls1046a_esdhc_start_image(memsize, 0, 0);

err:
	pr_err("Booting failed\n");

	while (1);
}

void ls1046ardb_entry(unsigned long r0, unsigned long r1, unsigned long r2);

__noreturn void ls1046ardb_entry(unsigned long r0, unsigned long r1, unsigned long r2)
{
	relocate_to_current_adr();
	setup_c();

	ls1046ardb_r_entry(r0);
}
