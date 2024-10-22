// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <mach/layerscape/debug_ll.h>
#include <debug_ll.h>
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

static struct fsl_ddr_controller tqmls1046a_ddrc[] = {
	{
		.memctl_opts.ddrtype = SDRAM_TYPE_DDR4,
		.base = IOMEM(LSCH2_DDR_ADDR),
		.ddr_freq = LS1046A_DDR_FREQ,
		.erratum_A008511 = 1,
		.erratum_A009803 = 1,
		.erratum_A010165 = 1,
		.erratum_A009801 = 1,
		.erratum_A009942 = 1,
		.chip_selects_per_ctrl = 4,
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
	.timing_cfg_0       = 0xF7660008,
	.timing_cfg_1       = 0xF1FC4178,
	.timing_cfg_2       = 0x00590160,
	.ddr_sdram_cfg      = 0x65000008,
	.ddr_sdram_cfg_2    = 0x00401150,
	.ddr_sdram_cfg_3    = 0x40000000,
	.ddr_sdram_mode     = 0x01030631,
	.ddr_sdram_mode_2   = 0x00100200,
	.ddr_sdram_mode_3   = 0x00000000,
	.ddr_sdram_mode_4   = 0x00000000,
	.ddr_sdram_mode_5   = 0x00000000,
	.ddr_sdram_mode_6   = 0x00000000,
	.ddr_sdram_mode_7   = 0x00000000,
	.ddr_sdram_mode_8   = 0x00000000,
	.ddr_sdram_mode_9   = 0x00000500,
	.ddr_sdram_mode_10  = 0x08800000,
	.ddr_sdram_mode_11  = 0x00000000,
	.ddr_sdram_mode_12  = 0x00000000,
	.ddr_sdram_mode_13  = 0x00000000,
	.ddr_sdram_mode_14  = 0x00000000,
	.ddr_sdram_mode_15  = 0x00000000,
	.ddr_sdram_mode_16  = 0x00000000,
	.ddr_sdram_interval = 0x0F3C079E,
	.ddr_data_init      = 0xDEADBEEF,
	.ddr_sdram_clk_cntl = 0x03000000,
	.ddr_init_addr      = 0x00000000,
	.ddr_init_ext_addr  = 0x00000000,
	.timing_cfg_4       = 0x00220002,
	.timing_cfg_5       = 0x00000000,
	.timing_cfg_6       = 0x00000000,
	.timing_cfg_7       = 0x25500000,
	.timing_cfg_8       = 0x05447A00,
	.timing_cfg_9       = 0x00000000,
	.ddr_zq_cntl        = 0x8A090705,
	.ddr_wrlvl_cntl     = 0x8605070A,
	.ddr_wrlvl_cntl_2   = 0x0A080807,
	.ddr_wrlvl_cntl_3   = 0x0706060A,
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

static struct fsl_ddr_controller tqmls1046a_ddrc_8g[] = {
	{
		.memctl_opts.ddrtype = SDRAM_TYPE_DDR4,
		.base = IOMEM(LSCH2_DDR_ADDR),
		.ddr_freq = LS1046A_DDR_FREQ,
		.erratum_A008511 = 1,
		.erratum_A009803 = 1,
		.erratum_A010165 = 1,
		.erratum_A009801 = 1,
		.erratum_A009942 = 1,
		.chip_selects_per_ctrl = 4,
		.fsl_ddr_config_reg = {
	.cs[0].bnds         = 0x000001FF,
	.cs[0].config       = 0x80010422,
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
	.timing_cfg_0       = 0xF7660008,
	.timing_cfg_1       = 0xF1FCC178,
	.timing_cfg_2       = 0x00590160,
	.ddr_sdram_cfg      = 0x65000008,
	.ddr_sdram_cfg_2    = 0x00401150,
	.ddr_sdram_cfg_3    = 0x40000000,
	.ddr_sdram_mode     = 0x01030631,
	.ddr_sdram_mode_2   = 0x00100200,
	.ddr_sdram_mode_3   = 0x00000000,
	.ddr_sdram_mode_4   = 0x00000000,
	.ddr_sdram_mode_5   = 0x00000000,
	.ddr_sdram_mode_6   = 0x00000000,
	.ddr_sdram_mode_7   = 0x00000000,
	.ddr_sdram_mode_8   = 0x00000000,
	.ddr_sdram_mode_9   = 0x00000500,
	.ddr_sdram_mode_10  = 0x08800000,
	.ddr_sdram_mode_11  = 0x00000000,
	.ddr_sdram_mode_12  = 0x00000000,
	.ddr_sdram_mode_13  = 0x00000000,
	.ddr_sdram_mode_14  = 0x00000000,
	.ddr_sdram_mode_15  = 0x00000000,
	.ddr_sdram_mode_16  = 0x00000000,
	.ddr_sdram_interval = 0x0F3C079E,
	.ddr_data_init      = 0xDEADBEEF,
	.ddr_sdram_clk_cntl = 0x03000000,
	.ddr_init_addr      = 0x00000000,
	.ddr_init_ext_addr  = 0x00000000,
	.timing_cfg_4       = 0x00220002,
	.timing_cfg_5       = 0x00000000,
	.timing_cfg_6       = 0x00000000,
	.timing_cfg_7       = 0x25500000,
	.timing_cfg_8       = 0x05447A00,
	.timing_cfg_9       = 0x00000000,
	.ddr_zq_cntl        = 0x8A090705,
	.ddr_wrlvl_cntl     = 0x8605070A,
	.ddr_wrlvl_cntl_2   = 0x0A080807,
	.ddr_wrlvl_cntl_3   = 0x0706060A,
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

static struct fsl_ddr_controller tqmls1046a_ddrc_8g_ca[] = {
	{
		.memctl_opts.ddrtype = SDRAM_TYPE_DDR4,
		.base = IOMEM(LSCH2_DDR_ADDR),
		.ddr_freq = LS1046A_DDR_FREQ,
		.erratum_A008511 = 1,
		.erratum_A009803 = 1,
		.erratum_A010165 = 1,
		.erratum_A009801 = 1,
		.erratum_A009942 = 1,
		.chip_selects_per_ctrl = 4,
		.fsl_ddr_config_reg = {
	.cs[0].bnds         = 0x000001FF,
	.cs[0].config       = 0x80010422,
	.cs[0].config       = 0x80010512,
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
	.timing_cfg_3       = 0x12551100,
	.timing_cfg_0       = 0xA0660008,
	.timing_cfg_1       = 0x060E6278,
	.timing_cfg_2       = 0x0058625E,
	.ddr_sdram_cfg      = 0x65200008,
	.ddr_sdram_cfg_2    = 0x00401070,
	.ddr_sdram_cfg_3    = 0x40000000,
	.ddr_sdram_mode     = 0x05030635,
	.ddr_sdram_mode_2   = 0x00100000,
	.ddr_sdram_mode_3   = 0x00000000,
	.ddr_sdram_mode_4   = 0x00000000,
	.ddr_sdram_mode_5   = 0x00000000,
	.ddr_sdram_mode_6   = 0x00000000,
	.ddr_sdram_mode_7   = 0x00000000,
	.ddr_sdram_mode_8   = 0x00000000,
	.ddr_sdram_mode_9   = 0x00000701,
	.ddr_sdram_mode_10  = 0x08800000,
	.ddr_sdram_mode_11  = 0x00000000,
	.ddr_sdram_mode_12  = 0x00000000,
	.ddr_sdram_mode_13  = 0x00000000,
	.ddr_sdram_mode_14  = 0x00000000,
	.ddr_sdram_mode_15  = 0x00000000,
	.ddr_sdram_mode_16  = 0x00000000,
	.ddr_sdram_interval = 0x0F3C079E,
	.ddr_data_init      = 0xDEADBEEF,
	.ddr_sdram_clk_cntl = 0x02400000,
	.ddr_init_addr      = 0x00000000,
	.ddr_init_ext_addr  = 0x00000000,
	.timing_cfg_4       = 0x00224502,
	.timing_cfg_5       = 0x06401400,
	.timing_cfg_6       = 0x00000000,
	.timing_cfg_7       = 0x25540000,
	.timing_cfg_8       = 0x06445A00,
	.timing_cfg_9       = 0x00000000,
	.ddr_zq_cntl        = 0x8A090705,
	.ddr_wrlvl_cntl     = 0x86750608,
	.ddr_wrlvl_cntl_2   = 0x07070605,
	.ddr_wrlvl_cntl_3   = 0x05040407,
	.ddr_sr_cntr        = 0x00000000,
	.ddr_sdram_rcw_1    = 0x00000000,
	.ddr_sdram_rcw_2    = 0x00000000,
	.ddr_sdram_rcw_3    = 0x00000000,
	.ddr_cdr1           = 0x80080000,
	.ddr_cdr2           = 0x00000080,
	.dq_map_0           = 0x06106104,
	.dq_map_1           = 0x84184184,
	.dq_map_2           = 0x06106104,
	.dq_map_3           = 0x84184000,
	.debug[28]          = 0x00700046,
		},
	},
};

#define TQ_VARIANT_LEN	18
#define TQ_VARIANT_TQMLS1046A_P2	0
#define TQ_VARIANT_TQMLS1046A_CA	1

static int tqmls1046a_get_variant(void)
{
	struct pbl_i2c *i2c;
	unsigned char buf[TQ_VARIANT_LEN + 1] = { 0 };
	uint8_t pos = 0x40;
	int ret;
	int variant = TQ_VARIANT_TQMLS1046A_P2;
	struct i2c_msg msg[] = {
		{
			.addr = 0x50,
			.len = 1,
			.buf = &pos,
		}, {
			.addr = 0x50,
			.len = TQ_VARIANT_LEN,
			.flags = I2C_M_RD,
			.buf = (void *)&buf,
		}
	};

	i2c = ls1046_i2c_init(IOMEM(LSCH2_I2C1_BASE_ADDR));

	ret = pbl_i2c_xfer(i2c, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		pr_err("I2C EEPROM read failed: %d\n", ret);
		goto out;
	}

	if (buf[0] == 0xff) {
		pr_err("Erased EEPROM detected\n");
		goto out;
	}

	pr_debug("Board Variant: %s\n", buf);

	if (!strcmp(buf, "TQMLS1046A-CA.0202")) {
		variant = TQ_VARIANT_TQMLS1046A_CA;
		goto out;
	}

	if (!strcmp(buf, "TQMLS1046A-P2.0201")) {
		variant = TQ_VARIANT_TQMLS1046A_P2;
		goto out;
	}

	pr_err("Unknown board variant, using default\n");

out:
	return variant;
}

extern char __dtb_z_fsl_ls1046a_tqmls1046a_mbls10xxa_start[];

static noinline __noreturn void tqmls1046a_r_entry(bool is_8g)
{
	unsigned long membase = LS1046A_DDR_SDRAM_BASE;
	int board_variant = 0;

	if (get_pc() >= membase)
		barebox_arm_entry(membase, 0x80000000 - SZ_64M,
				  __dtb_z_fsl_ls1046a_tqmls1046a_mbls10xxa_start);

	arm_cpu_lowlevel_init();
	ls1046a_init_lowlevel();

	ls1046a_debug_ll_init();

	udelay(500);
	putc_ll('>');

	if (is_8g) {
		board_variant = tqmls1046a_get_variant();
		if (board_variant == TQ_VARIANT_TQMLS1046A_CA)
			fsl_ddr_set_memctl_regs(&tqmls1046a_ddrc_8g_ca[0], 0, false);
		else
			fsl_ddr_set_memctl_regs(&tqmls1046a_ddrc_8g[0], 0, false);
	} else {
		fsl_ddr_set_memctl_regs(&tqmls1046a_ddrc[0], 0, false);
	}

	ls1046a_errata_post_ddr();

	ls1046a_xload_start_image(NULL);

	pr_err("Booting failed\n");

	while (1);
}

void tqmls1046a_entry(void);

__noreturn void tqmls1046a_entry(void)
{
	relocate_to_current_adr();
	setup_c();

	tqmls1046a_r_entry(false);
}

void tqmls1046a_8g_entry(void);

__noreturn void tqmls1046a_8g_entry(void)
{
	relocate_to_current_adr();
	setup_c();

	tqmls1046a_r_entry(true);
}
