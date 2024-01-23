// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <debug_ll.h>
#include <ddr_spd.h>
#include <image-metadata.h>
#include <platform_data/mmc-esdhc-imx.h>
#include <soc/fsl/fsl_ddr_sdram.h>
#include <soc/fsl/immap_lsch2.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/syscounter.h>
#include <asm/cache.h>
#include <mach/layerscape/lowlevel.h>
#include <mach/layerscape/xload.h>
#include <mach/layerscape/errata.h>
#include <mach/layerscape/layerscape.h>
#include <linux/bitfield.h>

static struct fsl_ddr_controller ddrc = {
	.memctl_opts.ddrtype = SDRAM_TYPE_DDR4,
	.base = IOMEM(LSCH2_DDR_ADDR),
	.ddr_freq = 1600000000,
	.erratum_A009942 = 1,
	.erratum_A009663 = 1,
	.chip_selects_per_ctrl = 4,
	.fsl_ddr_config_reg = {
		.cs[0].bnds		= 0x000000ff,
		.cs[0].config		= 0x80040422,
		.cs[0].config_2		= 0,
		.cs[1].bnds		= 0,
		.cs[1].config		= 0,
		.cs[1].config_2		= 0,

		.timing_cfg_3		= 0x01111000,
		.timing_cfg_0		= 0xd0550018,
		.timing_cfg_1		= 0xFAFC0C42,
		.timing_cfg_2		= 0x0048c114,
		.ddr_sdram_cfg		= 0xe50c000c,
		.ddr_sdram_cfg_2	= 0x00401110,
		.ddr_sdram_mode		= 0x01010230,
		.ddr_sdram_mode_2	= 0x0,

		.ddr_sdram_md_cntl	= 0x0600001f,
		.ddr_sdram_interval	= 0x18600618,
		.ddr_data_init		= 0xdeadbeef,

		.ddr_sdram_clk_cntl	= 0x02000000,
		.ddr_init_addr		= 0,
		.ddr_init_ext_addr	= 0,

		.timing_cfg_4		= 0x00000002,
		.timing_cfg_5		= 0x07401400,
		.timing_cfg_6		= 0x0,
		.timing_cfg_7		= 0x23300000,

		.ddr_zq_cntl		= 0x8A090705,
		.ddr_wrlvl_cntl		= 0x86550607,
		.ddr_sr_cntr		= 0,
		.ddr_sdram_rcw_1	= 0,
		.ddr_sdram_rcw_2	= 0,
		.ddr_wrlvl_cntl_2	= 0x0708080A,
		.ddr_wrlvl_cntl_3	= 0x0A0B0C09,

		.ddr_sdram_mode_9	= 0x00000400,
		.ddr_sdram_mode_10	= 0x04000000,

		.timing_cfg_8		= 0x06115600,

		.dq_map_0		= 0x5b65b658,
		.dq_map_1		= 0xd96d8000,
		.dq_map_2		= 0,
		.dq_map_3		= 0x01600000,

		.ddr_cdr1		= 0x80040000,
		.ddr_cdr2		= 0x000000C1
	},
};

extern char __dtb_z_fsl_ls1028a_rdb_start[];

#define MEM_PLL_RAT	GENMASK(15, 10)

static unsigned long get_ddr_freq(void)
{
	unsigned long freq = 100000000;
	u32 rcwsr1 = readl(0x1e00100);
	u32 mult;

	mult = FIELD_GET(MEM_PLL_RAT, rcwsr1);

	return freq * mult;
}

struct dram_regions_info dram_info = {
	.num_dram_regions = 2,
	.total_dram_size = SZ_4G,
	.region = {
		{
			.addr = LS1028A_DDR_SDRAM_BASE,
			.size = SZ_2G,
		}, {
			.addr = LS1028A_DDR_SDRAM_HIGHMEM_BASE,
			.size = SZ_2G,
		},
	},
};

static noinline __noreturn void ls1028ardb_r_entry(unsigned long memsize)
{
	unsigned long membase = LS1028A_DDR_SDRAM_BASE;

	if (get_pc() >= membase)
		barebox_arm_entry(membase, SZ_2G - LS1028A_TFA_RESERVED_SIZE,
				  __dtb_z_fsl_ls1028a_rdb_start);

	arm_cpu_lowlevel_init();
	ls1028a_init_lowlevel();
	ddrc.ddr_freq = get_ddr_freq();

	fsl_ddr_set_memctl_regs(&ddrc, 0, true);

	ls1028a_tzc400_init(SZ_4G);

	ls1028a_errata_post_ddr();

	ls1028a_esdhc1_start_image(&dram_info);

	hang();
}

void ls1028ardb_entry(unsigned long r0, unsigned long r1, unsigned long r2);

__noreturn void ls1028ardb_entry(unsigned long r0, unsigned long r1, unsigned long r2)
{
	ls1028a_uart_setup(IOMEM(LSCH2_NS16550_COM1));

	relocate_to_current_adr();
	setup_c();

	ls1028ardb_r_entry(r0);
}
