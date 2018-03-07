#include <debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx6.h>

extern char __dtb_imx6s_riotboard_start[];

ENTRY_FUNCTION(start_imx6s_riotboard, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x4, 0x020e016c);
		imx6_uart_setup_ll();
		putc_ll('a');
	}

	fdt = __dtb_imx6s_riotboard_start + get_runtime_offset();
	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
