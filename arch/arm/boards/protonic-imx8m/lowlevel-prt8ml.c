// SPDX-License-Identifier: GPL-2.0-only

#include <asm/barebox-arm.h>
#include <common.h>
#include <debug_ll.h>
#include <firmware.h>
#include <image-metadata.h>
#include <mach/imx/atf.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/imx8mp-regs.h>
#include <mach/imx/iomux-mx8mp.h>
#include <mach/imx/xload.h>
#include <serial/imx-uart.h>
#include <soc/fsl/fsl_udc.h>
#include <soc/imx8m/ddr.h>

extern char __dtb_imx8mp_prt8ml_start[];

#define UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART4_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mp_setup_pad(MX8MP_PAD_UART4_TXD__UART4_DTE_RX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

/* read piggydata via a bootrom callback and place it behind our copy in SDRAM */
extern struct dram_timing_info prt8ml_dram_timing;

static void start_atf(void)
{
	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() != 3)
		return;

	printf("Starting PBL...\r\n");

	imx8mp_early_clock_init();

	imx8mp_ddr_init(&prt8ml_dram_timing, DRAM_TYPE_LPDDR4);

	imx8mp_load_and_start_image_via_tfa();
}

/*
 * Power-on execution flow of start_prt_prt8ml() might not be
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
static __noreturn noinline void prt_prt8ml_start(void)
{
	setup_uart();

	start_atf();

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mp_barebox_entry(__dtb_imx8mp_prt8ml_start);
}

ENTRY_FUNCTION(start_prt_prt8ml, r0, r1, r2)
{
	imx8mp_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	prt_prt8ml_start();
}
