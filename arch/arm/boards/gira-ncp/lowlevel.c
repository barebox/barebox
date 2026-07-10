// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8mm-regs.h>
#include <mach/imx/iomux-mx8mm.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/xload.h>
#include <soc/imx8m/ddr.h>

#define UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART1_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mm_setup_pad(IMX8MM_PAD_UART1_TXD_UART1_TX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

/*
 * Power-on execution flow of start_gira_ncp() might not be
 * obvious for a very first read, so here's, hopefully helpful,
 * summary:
 *
 * 1. MaskROM uploads PBL into OCRAM and that's where this function is
 *    executed for the first time. At entry the exception level is EL3.
 *
 * 2. DDR is initialized and the image is loaded from storage into DRAM. The PBL
 *    part is copied from OCRAM to the TF-A return address in DRAM.
 *
 * 3. TF-A is executed and exits into the PBL code in DRAM. TF-A has taken us
 *    from EL3 to EL2.
 *
 * 4. Standard barebox boot flow continues
 */
extern struct dram_timing_info nt5ad512m16c4_2gb_timing;
extern char __dtb_z_imx8mm_gira_ncp_start[];

static __noreturn noinline void gira_ncp_start(void)
{
	setup_uart();

	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() == 3) {
		imx8mm_early_clock_init();
		imx8mm_ddr_init(&nt5ad512m16c4_2gb_timing, DRAM_TYPE_DDR4);
		imx8mm_load_and_start_image_via_tfa();
	}

	/*
	 * Standard entry we hit once we initialized both DDR and ATF. I2C pad
	 * and clock setup already done during power_init_board().
	 */
	imx8mm_barebox_entry(__dtb_z_imx8mm_gira_ncp_start);
}

ENTRY_FUNCTION(start_gira_ncp, r0, r1, r2)
{
	imx8mm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	gira_ncp_start();
}
