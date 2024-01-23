// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: (C) Copyright 2023 Ametek Inc.
// SPDX-FileCopyrightText: 2023 Renaud Barbier <renaud.barbier@ametek.com>

/*
 * Derived from Freescale LSDK-19.09-update-311219
 */
#include <common.h>
#include <clock.h>
#include <debug_ll.h>
#include <soc/fsl/fsl_ddr_sdram.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/syscounter.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <linux/sizes.h>
#include <mach/layerscape/errata.h>
#include <mach/layerscape/lowlevel.h>
#include <mach/layerscape/xload.h>
#include <mach/layerscape/layerscape.h>

static struct fsl_ddr_controller ddrc[] = {
	{
		.memctl_opts.ddrtype = SDRAM_TYPE_DDR3,
		.base = IOMEM(LSCH2_DDR_ADDR),
		.ddr_freq = LS1021A_DDR_FREQ,
		.erratum_A009942 = 1,
		.chip_selects_per_ctrl = 4,
		.fsl_ddr_config_reg = {
			.cs[0].bnds         = 0x008000bf,
			.cs[0].config       = 0x80014302,
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
			.timing_cfg_3       = 0x010e1000,
			.timing_cfg_0       = 0x50550004,
			.timing_cfg_1       = 0xbcb38c56,
			.timing_cfg_2       = 0x0040d120,
			.ddr_sdram_cfg      = 0x470c0008,
			.ddr_sdram_cfg_2    = 0x00401010,
			.ddr_sdram_mode     = 0x00061c60,
			.ddr_sdram_mode_2   = 0x00180000,
			.ddr_sdram_interval = 0x18600618,
			.ddr_data_init      = 0xDEADBEEF,
			.ddr_sdram_clk_cntl = 0x02000000,
			.ddr_init_addr      = 0x00000000,
			.ddr_init_ext_addr  = 0x00000000,
			.timing_cfg_4       = 0x00000001,
			.timing_cfg_5       = 0x03401400,
			.ddr_zq_cntl        = 0x89080600,
			.ddr_wrlvl_cntl     = 0x8655f605,
			.ddr_wrlvl_cntl_2   = 0x05060607,
			.ddr_wrlvl_cntl_3   = 0x05050505,
			.ddr_sr_cntr        = 0x00000000,
			.ddr_sdram_rcw_1    = 0x00000000,
			.ddr_sdram_rcw_2    = 0x00000000,
			.ddr_sdram_rcw_3    = 0x00000000,
			.ddr_cdr1           = 0x80040000,
			.ddr_cdr2           = 0x000000C0,
			.dq_map_0           = 0x00000000,
			.dq_map_1           = 0x00000000,
			.dq_map_2           = 0x00000000,
			.dq_map_3           = 0x00000000,
			.debug[28]          = 0x00700046,
		},
	},
};

extern char __dtb_fsl_ls1021a_iot_start[];

static noinline __noreturn void ls1021aiot_r_entry(void)
{
	unsigned long membase = LS1021A_DDR_SDRAM_BASE;

	if (get_pc() >= membase) {
		barebox_arm_entry(membase, SZ_1G - SZ_64M,
				  __dtb_fsl_ls1021a_iot_start);
	}

	ls102xa_init_lowlevel();
	ls102xa_debug_ll_init();

	udelay(500);
	putc_ll('>');

	fsl_ddr_set_memctl_regs(&ddrc[0], 0, false);

	ls1021a_errata_post_ddr();

	ls1021a_xload_start_image(SZ_1G, 0, 0);

	pr_err("Booting failed\n");

	while (1)
		;
}

void ls1021aiot_entry(unsigned long r0, unsigned long r1, unsigned long r2);

__noreturn void
ls1021aiot_entry(unsigned long r0, unsigned long r1, unsigned long r2)
{
	relocate_to_current_adr();
	setup_c();

	ls1021aiot_r_entry();
}
