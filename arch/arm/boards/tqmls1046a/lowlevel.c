// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <debug_ll.h>
#include <image-metadata.h>
#include <platform_data/mmc-esdhc-imx.h>
#include <soc/fsl/fsl_ddr_sdram.h>
#include <soc/fsl/immap_lsch2.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/syscounter.h>
#include <asm/cache.h>
#include <mach/errata.h>
#include <mach/lowlevel.h>
#include <mach/xload.h>
#include <mach/layerscape.h>

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
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl | cpo  |wrdata|2T
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3  |      |delay |
	 */
	{1,  2100, 0, 8,     9, 0x09080806, 0x07060606,},
	{}
};

static const struct board_specific_parameters *udimms[] = {
	udimm0,
};

static void ddr_board_options(memctl_options_t *popts,
			      struct dimm_params *pdimm,
			      struct fsl_ddr_controller *c)
{
	const struct board_specific_parameters *pbsp, *pbsp_highest = NULL;
	unsigned long ddr_freq;

	if (!pdimm->n_ranks)
		return;

	pbsp = udimms[0];

	/*
	 * Get clk_adjust, wrlvl_start, wrlvl_ctl, according to the board ddr
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

	popts->ddr_cdr1 = DDR_CDR1_DHC_EN | DDR_CDR1_ODT(DDR4_CDR_ODT_60ohm);
	popts->ddr_cdr2 = DDR_CDR2_ODT(DDR4_CDR_ODT_60ohm) |
			  DDR_CDR2_VREF_TRAIN_EN | DDR_CDR2_VREF_RANGE_2;

	/* optimize cpo for erratum A-009942 */
	popts->cpo_sample = 0x48;
}

static struct dimm_params dimm_params[] = {
	{
		.n_ranks = 1,
		.rank_density = 2147483648u,
		.capacity = 2147483648u,
		.primary_sdram_width = 64,
		.ec_sdram_width = 8,
		.registered_dimm = 0,
		.mirrored_dimm = 0,
		.n_row_addr = 15,
		.n_col_addr = 10,
		.bank_addr_bits = 2,
		.bank_group_bits = 0,
		.edc_config = 2,
		.burst_lengths_bitmask = 0x0c,

		.tckmin_x_ps = 833,
		.tckmax_ps = 1900,
		.caslat_x = 0x000DFA00, //
		.taa_ps = 13320,
		.trcd_ps = 13320,
		.trp_ps = 13320,
		.tras_ps = 32000,
		.trc_ps = 45320,
		.trfc1_ps = 260000,
		.trfc2_ps = 160000,
		.trfc4_ps = 110000,
		.tfaw_ps = 21000,
		.trrds_ps = 3300,
		.trrdl_ps = 4900,
		.tccdl_ps = 5000,
		.trfc_slr_ps = 3500000,
		.refresh_rate_ps = 7800000,
	},
};

static struct fsl_ddr_controller ddrc[] = {
	{
		.dimm_slots_per_ctrl = ARRAY_SIZE(dimm_params),
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
		.fsl_ddr_config_reg = {
	.cs[0].bnds         = 0x0000007F,
	.cs[0].config       = 0x80010312,
	.cs[0].config_2     = 0x00000000,
	.cs[1].bnds         = 0x00000000,
	.cs[1].config       = 0x00000000,
	.cs[1].config_2     = 0x00000000,
	.cs[2].bnds         = 0x00000000,
	.cs[2].config       = 0x00000000,
	.cs[2].config_2     = 0x00000000,
	.cs[3].bnds         = 0x00000000,
	.cs[3].config       = 0x00000000,
	.cs[3].config_2     = 0x00000000,
	.timing_cfg_3       = 0x020F1100,
	.timing_cfg_0       = 0x77660008,
	.timing_cfg_1       = 0xF1FCC265,
	.timing_cfg_2       = 0x0059415E,
	.ddr_sdram_cfg      = 0x65000000,
	.ddr_sdram_cfg_2    = 0x00401150,
	.ddr_sdram_cfg_3    = 0x00000000,
	.ddr_sdram_mode     = 0x03010625,
	.ddr_sdram_mode_2   = 0x00100200,
	.ddr_sdram_mode_3   = 0x00010625,
	.ddr_sdram_mode_4   = 0x00100200,
	.ddr_sdram_mode_5   = 0x00010625,
	.ddr_sdram_mode_6   = 0x00100200,
	.ddr_sdram_mode_7   = 0x00010625,
	.ddr_sdram_mode_8   = 0x00100200,
	.ddr_sdram_mode_9   = 0x00000500,
	.ddr_sdram_mode_10  = 0x04400000,
	.ddr_sdram_mode_11  = 0x00000400,
	.ddr_sdram_mode_12  = 0x04400000,
	.ddr_sdram_mode_13  = 0x00000400,
	.ddr_sdram_mode_14  = 0x04400000,
	.ddr_sdram_mode_15  = 0x00000400,
	.ddr_sdram_mode_16  = 0x04400000,
	.ddr_sdram_interval = 0x0F3C0000,
	.ddr_data_init      = 0xDEADBEEF,
	.ddr_sdram_clk_cntl = 0x02000000,
	.ddr_init_addr      = 0x00000000,
	.ddr_init_ext_addr  = 0x00000000,
	.timing_cfg_4       = 0x00224002,
	.timing_cfg_5       = 0x04401400,
	.timing_cfg_6       = 0x00000000,
	.timing_cfg_7       = 0x25500000,
	.timing_cfg_8       = 0x03335A00,
	.timing_cfg_9       = 0x00000000,
	.ddr_zq_cntl        = 0x8A090705,
	.ddr_wrlvl_cntl     = 0x86550609,
	.ddr_wrlvl_cntl_2   = 0x09080806,
	.ddr_wrlvl_cntl_3   = 0x06040409,
	.ddr_sr_cntr        = 0x00000000,
	.ddr_sdram_rcw_1    = 0x00000000,
	.ddr_sdram_rcw_2    = 0x00000000,
	.ddr_sdram_rcw_3    = 0x00000000,
	.ddr_cdr1           = 0x80080000,
	.ddr_cdr2           = 0x000000C0,
	.dq_map_0           = 0x00000000,
	.dq_map_1           = 0x00000000,
	.dq_map_2           = 0x00000000,
	.dq_map_3           = 0x00000000,
	.debug[28]          = 0x00700046,
		},
	},
};

extern char __dtb_fsl_tqmls1046a_mbls10xxa_start[];

static noinline __noreturn void tqmls1046a_r_entry(void)
{
	unsigned long membase = LS1046A_DDR_SDRAM_BASE;

	if (get_pc() >= membase)
		barebox_arm_entry(membase, 0x80000000,
				  __dtb_fsl_tqmls1046a_mbls10xxa_start);

	arm_cpu_lowlevel_init();
	ls1046a_init_lowlevel();

	debug_ll_init();

	udelay(500);
	putc_ll('>');

	IMD_USED_OF(fsl_tqmls1046a_mbls10xxa);

	fsl_ddr_set_memctl_regs(&ddrc[0], 0);

	ls1046a_errata_post_ddr();

	ls1046a_xload_start_image(0, 0, 0);

	pr_err("Booting failed\n");

	while (1);
}

void tqmls1046a_entry(unsigned long r0, unsigned long r1, unsigned long r2);

__noreturn void tqmls1046a_entry(unsigned long r0, unsigned long r1, unsigned long r2)
{
	relocate_to_current_adr();
	setup_c();

	tqmls1046a_r_entry();
}
