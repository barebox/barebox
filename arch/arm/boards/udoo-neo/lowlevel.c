#include <debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/esdctl.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	imx6_ungate_all_peripherals();

	writel(0x0, iomuxbase + 0x24);
	writel(0x1b0b1, iomuxbase + 0x036C);
	writel(0x0, iomuxbase + 0x28);
	writel(0x1b0b1, iomuxbase + 0x0370);

	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_imx6sx_udoo_neo_full_start[];

ENTRY_FUNCTION(start_imx6sx_udoo_neo, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_imx6sx_udoo_neo_full_start + get_runtime_offset();

	imx6sx_barebox_entry(fdt);
}
