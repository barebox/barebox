// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 John Watts <contact@jookia.org>

#include <asm/barebox-arm.h>
#include <common.h>
#include <ddr_dimms.h>
#include <ddr_spd.h>
#include <debug_ll.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <mach/imx6.h>
#include <mach/imx6-mmdc.h>
#include <mach/iomux-mx6.h>
#include <mach/xload.h>
#include <pbl/i2c.h>
#include <soc/fsl/fsl_udc.h>
#include "ddr_regs.h"

#define STACK_TOP (MX6_OCRAM_BASE_ADDR + MX6_OCRAM_MAX_SIZE)

extern char __dtb_z_imx6q_novena_start[];

static struct spd_eeprom spd_eeprom;
static struct dimm_params dimm_params;

static struct pbl_i2c *setup_spd_i2c(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *i2c1base = IOMEM(MX6_I2C1_BASE_ADDR);

	imx_setup_pad(iomuxbase, MX6Q_PAD_CSI0_DAT8__I2C1_SDA);
	imx_setup_pad(iomuxbase, MX6Q_PAD_CSI0_DAT9__I2C1_SCL);

	return imx6_i2c_early_init(i2c1base);
}

static struct spd_eeprom *read_spd(void)
{
	struct spd_eeprom *eeprom = &spd_eeprom;
	struct pbl_i2c *i2c = setup_spd_i2c();
	int rc;

	rc = spd_read_eeprom(i2c, 0x50, eeprom, SPD_MEMTYPE_DDR3);
	if (rc < 0) {
		pr_err("Couldn't read SPD EEPROM: %i\n", rc);
		return NULL;
	}

	rc = ddr3_spd_check(&eeprom->ddr3);
	if (rc < 0) {
		pr_err("Couldn't verify SPD data: %i\n", rc);
		return NULL;
	}

	return eeprom;
}

static void setup_dimm_settings(struct dimm_params *params,
				struct mx6_ddr_sysinfo *info,
				struct mx6_ddr3_cfg *cfg)
{
	int capacity_gbit = params->capacity / 0x8000000;
	int density_rank = capacity_gbit / params->n_ranks;

	info->ncs = params->n_ranks;
	info->cs_density = density_rank;
	cfg->mem_speed = params->tckmin_x_ps;
	cfg->density = density_rank / params->n_banks_per_sdram_device;
	cfg->width = params->data_width;
	cfg->banks = params->n_banks_per_sdram_device;
	cfg->rowaddr = params->n_row_addr;
	cfg->coladdr = params->n_col_addr;
	cfg->trcd = params->trcd_ps / 10;
	cfg->trcmin = params->trc_ps / 10;
	cfg->trasmin = params->tras_ps / 10;
	cfg->SRT = params->extended_op_srt;

	if (params->device_width >= 16)
		cfg->pagesz = 2;
}

static void read_dimm_settings(void)
{
	struct spd_eeprom *eeprom = read_spd();
	struct dimm_params *params = &dimm_params;
	int rc;

	if (!eeprom) {
		pr_err("Couldn't read SPD EEPROM, using default settings\n");
		return;
	}

	rc = ddr3_compute_dimm_parameters(&eeprom->ddr3, params);
	if (rc < 0) {
		pr_err("Couldn't compute DIMM params: %i\n", rc);
		return;
	}

	pr_info("Found DIMM: %s\n", params->mpart);

	if (params->primary_sdram_width != 64) {
		pr_err("ERROR: DIMM stick memory width is not 64 bits\n");
		hang();
	}

	setup_dimm_settings(params, &novena_ddr_info, &novena_ddr_cfg);
}

static bool running_from_ram(void)
{
	return (get_pc() >= MX6_MMDC_PORT01_BASE_ADDR);
}

static void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *uart2base = IOMEM(MX6_UART2_BASE_ADDR);

	/* NOTE: RX is needed for TX to work on this board */
	imx_setup_pad(iomuxbase, MX6Q_PAD_EIM_D26__UART2_RXD);
	imx_setup_pad(iomuxbase, MX6Q_PAD_EIM_D27__UART2_TXD);

	imx6_uart_setup(uart2base);
	pbl_set_putc(imx_uart_putc, uart2base);

	pr_debug(">");
}

static void setup_ram(void)
{
	read_dimm_settings();

	mx6dq_dram_iocfg(64, &novena_ddr_regs, &novena_grp_regs);
	mx6_dram_cfg(&novena_ddr_info, &novena_mmdc_calib, &novena_ddr_cfg);

	mmdc_do_write_level_calibration();
	mmdc_do_dqs_calibration();
}

static void load_barebox(void)
{
	enum bootsource bootsrc;
	int bootinstance;

	imx6_get_boot_source(&bootsrc, &bootinstance);

	if (bootsrc == BOOTSOURCE_SERIAL)
		imx6_barebox_start_usb(IOMEM(MX6_MMDC_PORT01_BASE_ADDR));
	else if (bootsrc == BOOTSOURCE_MMC)
		imx6_esdhc_start_image(bootinstance);

	pr_err("Unsupported boot source %i instance %i\n",
	        bootsrc, bootinstance);
	hang();
}

ENTRY_FUNCTION_WITHSTACK(start_imx6q_novena, STACK_TOP, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();
	relocate_to_current_adr();
	setup_c();

	imx6_ungate_all_peripherals();
	setup_uart();

	if (!running_from_ram()) {
		setup_ram();
		load_barebox();
	} else {
		imx6q_barebox_entry(__dtb_z_imx6q_novena_start);
	}
}
