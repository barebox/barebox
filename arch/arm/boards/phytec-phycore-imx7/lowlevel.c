#define DEBUG
#include <io.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/debug_ll.h>
#include <asm/cache.h>

extern char __dtb_imx7d_phyboard_zeta_start[];

static noinline void phytec_phycore_imx7_start(void)
{
	void __iomem *iomuxbase = IOMEM(0x302c0000);
	void __iomem *uart = IOMEM(MX7_UART5_BASE_ADDR);
	void __iomem *ccmbase = IOMEM(MX7_CCM_BASE_ADDR);
	void *fdt;

	writel(0x3, ccmbase + 0x4000 + 16 * 152 + 0x8);
	writel(0x10000000, ccmbase + 0x8000 + 128 * 99);
	writel(0x3, ccmbase + 0x4000 + 16 * 152 + 0x4);
	writel(0x3, iomuxbase + 0x18);
	writel(0x3, iomuxbase + 0x1c);

	imx7_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	pr_debug("Phytec phyCORE i.MX7\n");

	fdt = __dtb_imx7d_phyboard_zeta_start + get_runtime_offset();

	barebox_arm_entry(0x80000000, SZ_512M, fdt);
}

ENTRY_FUNCTION(start_phytec_phycore_imx7, r0, r1, r2)
{
	imx7_cpu_lowlevel_init();

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();
	barrier();

	phytec_phycore_imx7_start();
}
