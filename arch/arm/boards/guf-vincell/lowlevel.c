#include <common.h>
#include <debug_ll.h>
#include <io.h>
#include <init.h>
#include <mach/imx53-regs.h>
#include <mach/clock-imx51_53.h>
#include <mach/imx5.h>
#include <mach/iomux-v3.h>
#include <mach/esdctl-v4.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/cache.h>

extern char __dtb_imx53_guf_vincell_lt_start[];
extern char __dtb_imx53_guf_vincell_start[];

static noinline void imx53_guf_vincell_init(void *fdt)
{
	void __iomem *ccm = (void *)MX53_CCM_BASE_ADDR;
	void __iomem *uart = IOMEM(MX53_UART2_BASE_ADDR);

	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);

	writel(0x0088494c, ccm + MX5_CCM_CBCDR);
	writel(0x02b12f0a, ccm + MX5_CCM_CSCMR2);
	imx53_ungate_all_peripherals();

	imx53_init_lowlevel_early(800);

	writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x27c);
	writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x278);
	imx53_uart_setup(uart);
	pbl_set_putc(imx_uart_putc, uart);
	pr_debug("GuF Vincell\n");

	imx53_barebox_entry(fdt);
}

static noinline void __imx53_guf_vincell_init(void *fdt)
{
	arm_early_mmu_cache_invalidate();
	imx5_cpu_lowlevel_init();
	relocate_to_current_adr();
	setup_c();
	barrier();

	imx53_guf_vincell_init(fdt);
}

ENTRY_FUNCTION(start_imx53_guf_vincell_lt, r0, r1, r2)
{
	void *fdt = __dtb_imx53_guf_vincell_lt_start + get_runtime_offset();

	__imx53_guf_vincell_init(fdt);
}

ENTRY_FUNCTION(start_imx53_guf_vincell, r0, r1, r2)
{
	void *fdt = __dtb_imx53_guf_vincell_start + get_runtime_offset();

	__imx53_guf_vincell_init(fdt);
}
