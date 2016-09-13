#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/cache.h>
#include <mach/generic.h>
#include <mach/imx6-regs.h>
#include <debug_ll.h>
#include <console.h>
#include <mach/esdctl.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	writel(0x1, iomuxbase + 0x2b0);

	imx6_ungate_all_peripherals();
	imx6_uart_setup(IOMEM(MX6_UART2_BASE_ADDR));
}

extern char __dtb_imx6q_guf_santaro_start[];

static noinline void santaro_start(void)
{
	pbl_set_putc(imx_uart_putc, IOMEM(MX6_UART2_BASE_ADDR));

	pr_debug("Garz+Fricke Santaro\n");

	imx6q_barebox_entry(__dtb_imx6q_guf_santaro_start);
}

ENTRY_FUNCTION(start_imx6q_guf_santaro, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	arm_early_mmu_cache_invalidate();

	setup_uart();

	relocate_to_current_adr();
	setup_c();
	barrier();

	santaro_start();
}
