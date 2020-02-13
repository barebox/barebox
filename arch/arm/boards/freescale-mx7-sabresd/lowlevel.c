#include <debug_ll.h>
#include <io.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx7-ccm-regs.h>
#include <mach/iomux-mx7.h>
#include <mach/debug_ll.h>
#include <asm/cache.h>
#include <mach/esdctl.h>

extern char __dtb_imx7d_sdb_start[];

static inline void setup_uart(void)
{
	imx7_early_setup_uart_clock();

	imx7_setup_pad(MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX);

	imx7_uart_setup_ll();

	putc_ll('>');
}

ENTRY_FUNCTION(start_imx7d_sabresd, r0, r1, r2)
{
	imx7_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	imx7d_barebox_entry(__dtb_imx7d_sdb_start + get_runtime_offset());
}
