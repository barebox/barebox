// SPDX-License-Identifier: GPL-2.0

#include <asm/barebox-arm.h>
#include <boards/phytec/phytec-som-imx8m-detection.h>
#include <common.h>
#include <debug_ll.h>
#include <mach/imx/atf.h>
#include <mach/imx/debug_ll.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/iomux-mx8mm.h>
#include <mach/imx/xload.h>
#include <mfd/bd71837.h>
#include <pbl/i2c.h>
#include <pbl/pmic.h>
#include <soc/imx8m/ddr.h>

#include "lowlevel.h"

extern char __dtb_z_imx8mm_phyboard_polis_rdk_start[];

#define UART_PAD_CTRL MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART3_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mm_setup_pad(IMX8MM_PAD_UART3_TXD_UART3_TX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);
	putc_ll('>');
}

#define EEPROM_ADDR 0x51
#define EEPROM_ADDR_FALLBACK 0x59

static void phyboard_polis_rdk_ddr_init(enum phytec_imx8m_ddr_size size)
{
	int ret;

	if (size == PHYTEC_IMX8M_DDR_AUTODETECT) {
		struct pbl_i2c *i2c;

		imx8mm_setup_pad(IMX8MM_PAD_I2C1_SCL_I2C1_SCL);
		imx8mm_setup_pad(IMX8MM_PAD_I2C1_SDA_I2C1_SDA);
		imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);
		i2c = imx8m_i2c_early_init(IOMEM(MX8MM_I2C1_BASE_ADDR));

		ret = phytec_eeprom_data_setup(i2c, NULL, EEPROM_ADDR,
					       EEPROM_ADDR_FALLBACK, IMX_CPU_IMX8MM);
		if (ret) {
			pr_err("Could not detect correct RAM size. Fallback to default.\n");
		} else {
			phytec_print_som_info(NULL);
			size = phytec_get_imx8m_ddr_size(NULL);
		}
	}

	switch (size) {
	case PHYTEC_IMX8M_DDR_1G:
		phyboard_polis_rdk_dram_timing.ddrc_cfg[5].val = 0x2d0087;
		phyboard_polis_rdk_dram_timing.ddrc_cfg[21].val = 0x8d;
		phyboard_polis_rdk_dram_timing.ddrc_cfg[42].val = 0xf070707;
		phyboard_polis_rdk_dram_timing.ddrc_cfg[58].val = 0x60012;
		phyboard_polis_rdk_dram_timing.ddrc_cfg[73].val = 0x13;
		phyboard_polis_rdk_dram_timing.ddrc_cfg[83].val = 0x30005;
		phyboard_polis_rdk_dram_timing.ddrc_cfg[98].val = 0x5;
		break;
	default:
	case PHYTEC_IMX8M_DDR_AUTODETECT:
	case PHYTEC_IMX8M_DDR_2G:
		break;
	case PHYTEC_IMX8M_DDR_4G:
		phyboard_polis_rdk_dram_timing.ddrc_cfg[2].val = 0xa3080020;
		phyboard_polis_rdk_dram_timing.ddrc_cfg[37].val = 0x17;
		phyboard_polis_rdk_dram_timing.fsp_msg[0].fsp_cfg[8].val = 0x310;
		phyboard_polis_rdk_dram_timing.fsp_msg[0].fsp_cfg[20].val = 0x3;
		phyboard_polis_rdk_dram_timing.fsp_msg[1].fsp_cfg[9].val = 0x310;
		phyboard_polis_rdk_dram_timing.fsp_msg[1].fsp_cfg[21].val = 0x3;
		phyboard_polis_rdk_dram_timing.fsp_msg[2].fsp_cfg[9].val = 0x310;
		phyboard_polis_rdk_dram_timing.fsp_msg[2].fsp_cfg[21].val = 0x3;
		phyboard_polis_rdk_dram_timing.fsp_msg[3].fsp_cfg[10].val = 0x310;
		phyboard_polis_rdk_dram_timing.fsp_msg[3].fsp_cfg[22].val = 0x3;
		break;
	}

	imx8mm_ddr_init(&phyboard_polis_rdk_dram_timing, DRAM_TYPE_LPDDR4);
}

static void start_phyboard_polis_rdk_common(enum phytec_imx8m_ddr_size size)
{
	imx8mm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	setup_uart();

	/*
	 * If we are in EL3 we are running for the first time out of OCRAM,
	 * we'll need to initialize the DRAM and run TF-A (BL31). The TF-A
	 * will then jump to DRAM in EL2
	 */
	if (current_el() == 3) {
		imx8mm_early_clock_init();

		phyboard_polis_rdk_ddr_init(size);

		imx8mm_load_and_start_image_via_tfa();
	}

	/* Standard entry we hit once we initialized both DDR and ATF */
	imx8mm_barebox_entry(__dtb_z_imx8mm_phyboard_polis_rdk_start);
}

ENTRY_FUNCTION(start_phyboard_polis_rdk_ddr_autodetect, r0, r1, r2)
{
	start_phyboard_polis_rdk_common(PHYTEC_IMX8M_DDR_AUTODETECT);
}

ENTRY_FUNCTION(start_phyboard_polis_rdk_ddr_1g, r0, r1, r2)
{
	start_phyboard_polis_rdk_common(PHYTEC_IMX8M_DDR_1G);
}

ENTRY_FUNCTION(start_phyboard_polis_rdk_ddr_2g, r0, r1, r2)
{
	start_phyboard_polis_rdk_common(PHYTEC_IMX8M_DDR_1G);
}

ENTRY_FUNCTION(start_phyboard_polis_rdk_ddr_4g, r0, r1, r2)
{
	start_phyboard_polis_rdk_common(PHYTEC_IMX8M_DDR_4G);
}
