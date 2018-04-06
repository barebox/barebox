#include <debug_ll.h>
#include <mach/clock-imx51_53.h>
#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/cache.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX51_IOMUXC_BASE_ADDR);
	void __iomem *ccmbase = IOMEM(MX51_CCM_BASE_ADDR);

	/*
	 * Restore CCM values that might be changed by the Mask ROM
	 * code.
	 *
	 * Source: RealView debug scripts provided by Freescale
	 */
	writel(MX5_CCM_CBCDR_RESET_VALUE,  ccmbase + MX5_CCM_CBCDR);
	writel(MX5_CCM_CSCMR1_RESET_VALUE, ccmbase + MX5_CCM_CSCMR1);
	writel(MX5_CCM_CSCDR1_RESET_VALUE, ccmbase + MX5_CCM_CSCDR1);

	/*
	 * The code below should be more or less a "moral equivalent"
	 * of:
	 *	 MX51_PAD_UART1_TXD__UART1_TXD 0x1c5
	 *
	 * in device tree
	 */
	writel(0x00000000, iomuxbase + 0x022c);
	writel(0x000001c5, iomuxbase + 0x061c);

	imx51_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_imx51_babbage_start[];

ENTRY_FUNCTION(start_imx51_babbage, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	arm_setup_stack(0x20000000 - 16);

	fdt = __dtb_imx51_babbage_start + get_runtime_offset();

	imx51_barebox_entry(fdt);
}

static noinline void babbage_entry(void)
{
	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();

	puts_ll("lowlevel init done\n");

	imx51_barebox_entry(NULL);
}

ENTRY_FUNCTION(start_imx51_babbage_xload, r0, r1, r2)
{
	imx5_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	arm_setup_stack(0x20000000 - 16);

	babbage_entry();
}
