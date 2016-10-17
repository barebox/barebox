#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx6-regs.h>
#include <io.h>
#include <mach/debug_ll.h>
#include <mach/esdctl.h>
#include <asm/cache.h>

extern char __dtb_imx6q_sabrelite_start[];

static noinline void imx6q_sabrelite_start(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *uart = IOMEM(MX6_UART2_BASE_ADDR);

	writel(0x4, iomuxbase + 0x0bc);

	imx6_ungate_all_peripherals();
	imx6_uart_setup(uart);
	pbl_set_putc(imx_uart_putc, uart);

	pr_debug("Freescale i.MX6q SabreLite\n");

	imx6q_barebox_entry(__dtb_imx6q_sabrelite_start);
}

ENTRY_FUNCTION(start_imx6q_sabrelite, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();
	barrier();

	imx6q_sabrelite_start();
}

extern char __dtb_imx6dl_sabrelite_start[];

static noinline void imx6dl_sabrelite_start(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *uart = IOMEM(MX6_UART2_BASE_ADDR);

	writel(0x4, iomuxbase + 0x16c);

	imx6_ungate_all_peripherals();
	imx6_uart_setup(uart);
	pbl_set_putc(imx_uart_putc, uart);

	pr_debug("Freescale i.MX6dl SabreLite\n");

	imx6q_barebox_entry(__dtb_imx6dl_sabrelite_start);
}

ENTRY_FUNCTION(start_imx6dl_sabrelite, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();
	barrier();

	imx6dl_sabrelite_start();
}
