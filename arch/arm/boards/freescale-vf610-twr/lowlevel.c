#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/vf610-regs.h>
#include <mach/clock-vf610.h>
#include <mach/iomux-vf610.h>
#include <debug_ll.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(VF610_IOMUXC_BASE_ADDR);

	vf610_ungate_all_peripherals();

	/*
	 * VF610_PAD_PTB4__UART1_TX
	 */
	writel(VF610_UART_PAD_CTRL | (2 << 20), iomuxbase + 0x0068);
	writel(0, iomuxbase + 0x0380);

	vf610_uart_setup_ll();
}

extern char __dtb_vf610_twr_start[];

ENTRY_FUNCTION(start_vf610_twr, r0, r1, r2)
{
	void *fdt;

	vf610_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_vf610_twr_start - get_runtime_offset();
	barebox_arm_entry(0x80000000, SZ_128M, fdt);
}
