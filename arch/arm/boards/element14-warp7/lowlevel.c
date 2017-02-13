#define DEBUG
#include <io.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/debug_ll.h>
#include <asm/cache.h>

extern char __dtb_imx7s_warp_start[];

static noinline void warp7_start(void)
{
	void __iomem *iomuxbase = IOMEM(MX7_IOMUXC_BASE_ADDR);
	void __iomem *uart = IOMEM(MX7_UART1_BASE_ADDR);
	void __iomem *ccmbase = IOMEM(MX7_CCM_BASE_ADDR);
	void *fdt;

	writel(0x3, ccmbase + 0x4000 + 16 * 148 + 0x8);
	writel(0x10000000, ccmbase + 0x8000 + 128 * 95);
	writel(0x3, ccmbase + 0x4000 + 16 * 148 + 0x4);
	writel(0x0, iomuxbase + 0x128);
	writel(0x0, iomuxbase + 0x12c);

	imx7_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	pr_debug("Element14 i.MX7 Warp\n");

	fdt = __dtb_imx7s_warp_start - get_runtime_offset();

	barebox_arm_entry(0x80000000, SZ_512M, fdt);
}

ENTRY_FUNCTION(start_imx7s_element14_warp7, r0, r1, r2)
{
	imx7_cpu_lowlevel_init();

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();
	barrier();

	warp7_start();
}