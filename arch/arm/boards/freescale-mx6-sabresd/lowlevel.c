#include <debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	imx6_ungate_all_peripherals();

	writel(0x1b0b1, iomuxbase + 0x0650);
	writel(3, iomuxbase + 0x0280);

	writel(0x1b0b1, iomuxbase + 0x0654);
	writel(3, iomuxbase + 0x0284);
	writel(1, iomuxbase + 0x0920);

	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_imx6q_sabresd_start[];

ENTRY_FUNCTION(start_imx6q_sabresd, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_imx6q_sabresd_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
