#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/esdctl.h>
#include <mach/vf610-regs.h>
#include <mach/clock-vf610.h>
#include <mach/iomux-vf610.h>
#include <debug_ll.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(VF610_IOMUXC_BASE_ADDR);

	vf610_ungate_all_peripherals();
	vf610_setup_pad(iomuxbase, VF610_PAD_PTB4__UART1_TX);
	vf610_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_vf610_twr_start[];

ENTRY_FUNCTION(start_vf610_twr, r0, r1, r2)
{

	vf610_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	vf610_barebox_entry(__dtb_vf610_twr_start + get_runtime_offset());
}
