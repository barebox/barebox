// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <mach/imx/generic.h>
#include <mach/imx/xload.h>
#include <asm/barebox-arm.h>
#include <soc/imx9/ddr.h>
#include <mach/imx/atf.h>
#include <mach/imx/xload.h>
#include <mach/imx/esdctl.h>
#include <pbl/i2c.h>
#include <boards/tq/tq_eeprom.h>

extern char __dtb_z_imx93_tqma9352_mba93xxca_start[];
extern char __dtb_z_imx93_tqma9352_mba93xxla_start[];
extern struct dram_timing_info tqma93xxca_dram_timing;
extern struct dram_timing_info tqma93xxla_dram_timing;

static int tqma93xx_get_formfactor(void)
{
	struct pbl_i2c *i2c;
	struct tq_eeprom *eeprom;
	phys_size_t ramsize;
	int formfactor;

	i2c = imx93_i2c_early_init(IOMEM(MX9_I2C1_BASE_ADDR));

	eeprom = pbl_tq_read_eeprom(i2c, 0x53, 0);
	if (!eeprom)
		return VARD_FORMFACTOR_TYPE_CONNECTOR;

	ramsize = tq_vard_ramsize(&eeprom->vard);
	if (ramsize != SZ_1G)
		pr_err("unsupported ram size 0x%08llx\n", ramsize);

	formfactor = tq_vard_get_formfactor(&eeprom->vard);

	switch (formfactor) {
	case VARD_FORMFACTOR_TYPE_LGA:
		pr_debug("LGA board type\n");
		break;
	case VARD_FORMFACTOR_TYPE_CONNECTOR:
		pr_debug("CONNECTOR board type\n");
		break;
	default:
		pr_err("Unknown formfactor\n");
		formfactor = VARD_FORMFACTOR_TYPE_CONNECTOR;
	}

	return formfactor;
}

static noinline void tqma93xx_continue(void)
{
	void *base = IOMEM(MX9_UART1_BASE_ADDR);
	void *muxbase = IOMEM(MX9_IOMUXC_BASE_ADDR);
	int formfactor;
	void *fdt;

	writel(0x10, muxbase + 0x170);
	writel(0x10, muxbase + 0x174);
	writel(0x0, muxbase + 0x184);
	writel(0xb9e, muxbase + 0x320);
	writel(0xb9e, muxbase + 0x324);

	imx9_uart_setup(IOMEM(base));
	pbl_set_putc(lpuart32_putc, base + 0x10);

	formfactor = tqma93xx_get_formfactor();

	if (current_el() == 3) {
		switch (formfactor) {
		case VARD_FORMFACTOR_TYPE_LGA:
			imx93_ddr_init(&tqma93xxla_dram_timing, DRAM_TYPE_LPDDR4);
			break;
		case VARD_FORMFACTOR_TYPE_CONNECTOR:
			imx93_ddr_init(&tqma93xxca_dram_timing, DRAM_TYPE_LPDDR4);
			break;
		}

		imx93_load_and_start_image_via_tfa();
	}

	switch (formfactor) {
	case VARD_FORMFACTOR_TYPE_LGA:
		fdt = __dtb_z_imx93_tqma9352_mba93xxla_start;
		break;
	case VARD_FORMFACTOR_TYPE_CONNECTOR:
		fdt = __dtb_z_imx93_tqma9352_mba93xxca_start;
		break;
	default:
		__builtin_unreachable();
	}

	imx93_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx93_tqma93xx, r0, r1, r2)
{
	if (current_el() == 3)
		imx93_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	tqma93xx_continue();
}
