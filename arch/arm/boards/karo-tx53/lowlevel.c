#include <common.h>
#include <debug_ll.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx5.h>
#include <mach/imx53-regs.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/cache.h>

extern char __dtb_imx53_tx53_xx30_start[];
extern char __dtb_imx53_tx53_1011_start[];

static inline void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX53_UART1_BASE_ADDR);

	writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x270);
	writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x274);
	writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x878);

	imx53_ungate_all_peripherals();
	imx53_uart_setup(uart);
	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

static void __imx53_tx53_init(int is_xx30)
{
	void *fdt;
	void *fdt_blob_fixed_offset = __dtb_imx53_tx53_1011_start;

	arm_early_mmu_cache_invalidate();
	imx5_cpu_lowlevel_init();
	relocate_to_current_adr();
	setup_c();
	barrier();

	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);

	if (is_xx30) {
		imx53_init_lowlevel_early(800);
		fdt_blob_fixed_offset = __dtb_imx53_tx53_xx30_start;
	}

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = fdt_blob_fixed_offset + get_runtime_offset();

	imx53_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx53_tx53_xx30_samsung, r0, r1, r2)
{
	__imx53_tx53_init(1);
}

ENTRY_FUNCTION(start_imx53_tx53_xx30, r0, r1, r2)
{
	__imx53_tx53_init(1);
}

ENTRY_FUNCTION(start_imx53_tx53_1011, r0, r1, r2)
{
	__imx53_tx53_init(0);
}
