// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <asm/barebox-arm.h>
#include <pbl/i2c.h>
#include <pbl/pmic.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/atf.h>
#include <mach/imx/generic.h>
#include <mach/imx/iomux-mx8mm.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mfd/bd71837.h>
#include <mach/imx/xload.h>
#include <soc/imx8m/ddr.h>

#include "lowlevel.h"

extern char __dtb_z_imx8mm_innocomm_wb15_evk_start[];

#define UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART2_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mm_setup_pad(IMX8MM_PAD_UART2_TXD_UART2_TX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);
	putc_ll('>');
}

static struct pmic_config bd71837_cfg[] = {
	/* unlock the PMIC regs */
	{ BD718XX_REGLOCK, 0x0 },
	/* retry powering up indefinitely every 250ms after VR fault */
	{ BD718XX_RCVCFG, 0xfc },
	/* decrease RESET key long push time from the default 10s to 10ms */
	{ BD718XX_PWRONCONFIG1, 0x0 },
	/* WDOG_B: Warm Reset */
	{ BD718XX_PWRCTRL0, 0xa3 },
	/* READY=>SNVS on PMIC_ON_REQ, SNVS=>RUN on VSYS_UVLO */
	{ BD718XX_TRANS_COND0, 0x48 },
	/* WDOG_B: Go to SNVS power state after deassert */
	{ BD718XX_TRANS_COND1, 0xc0 },
	/* Set VDD_SOC/VDD_DRAM to typical value 0.85v for nominal mode */
	{ BD718XX_BUCK1_VOLT_RUN, 0xf },
	/* increase VDD_DRAM to 0.900v for 2400MT/s DDR */
	{ BD718XX_1ST_NODVS_BUCK_VOLT, 0x02 },
	/* set BUCK8 to 1.10v */
	{ BD718XX_4TH_NODVS_BUCK_VOLT, 0x1e },
	/* lock the PMIC regs */
	{ BD718XX_REGLOCK, 0x11 },
};

void innocomm_wb15_power_init_board(void)
{
	struct pbl_i2c *i2c;

	imx8mm_setup_pad(IMX8MM_PAD_I2C1_SCL_I2C1_SCL);
	imx8mm_setup_pad(IMX8MM_PAD_I2C1_SDA_I2C1_SDA);

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MM_I2C1_BASE_ADDR));

	pmic_configure(i2c, 0x4b, bd71837_cfg, ARRAY_SIZE(bd71837_cfg));
}

ENTRY_FUNCTION(start_innocomm_wb15_evk, r0, r1, r2)
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

		innocomm_wb15_power_init_board();

		imx8mm_ddr_init(&innocomm_wb15_dram_timing, DRAM_TYPE_LPDDR4);

		imx8mm_load_and_start_image_via_tfa();
	}

	/* Standard entry we hit once we initialized both DDR and ATF */
	imx8mm_barebox_entry(__dtb_z_imx8mm_innocomm_wb15_evk_start);
}
