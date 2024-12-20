// SPDX-License-Identifier: GPL-2.0-only

#include <asm/barebox-arm.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/imx6.h>
#include <mach/imx/iomux-mx6.h>

#define UART_DT_IOMUX_PAD(_mux_ctrl_ofs, _pad_ctrl_ofs, _sel_input_ofs, _mux_mode, _sel_input) \
	IOMUX_PAD(_pad_ctrl_ofs, _mux_ctrl_ofs, _mux_mode, _sel_input_ofs, \
		  _sel_input, MX6Q_UART_PAD_CTRL)

/* We only have C defines for MX6Q_ */
#define MX6DL_PAD_CSI0_DAT11__UART1_TX_DATA        UART_DT_IOMUX_PAD(0x050, 0x364, 0x000, 0x3, 0x0)

static void setup_uart(void)
{
	void __iomem *uart1base = IOMEM(MX6_UART1_BASE_ADDR);
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	imx6_uart_setup(uart1base);
	imx_uart_set_dte(uart1base);

	imx_setup_pad(iomuxbase, MX6DL_PAD_CSI0_DAT11__UART1_TX_DATA);

	putc_ll('>');
}

extern char __dtb_z_imx6dl_colibri_iris_start[];

ENTRY_FUNCTION(start_imx6dl_colibri_iris, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_imx6dl_colibri_iris_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

void __noreturn start_imx6s_colibri_iris(ulong, ulong, ulong)
	__alias(start_imx6dl_colibri_iris);
