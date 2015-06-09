#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/generic.h>
#include <mach/imx6-regs.h>
#include <debug_ll.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	writel(0x1, iomuxbase + 0x2b0);

	imx6_ungate_all_peripherals();
	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_imx6q_guf_santaro_start[];

ENTRY_FUNCTION(start_imx6q_guf_santaro, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_imx6q_guf_santaro_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
