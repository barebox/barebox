#include <debug_ll.h>
#include <common.h>
#include <sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx6-mmdc.h>
#include <mach/imx6.h>

static inline void setup_uart(void)
{
	/* Enable UART for lowlevel debugging purposes */
	writel(0x00000000, 0x021e8080);
	writel(0x00004027, 0x021e8084);
	writel(0x00000704, 0x021e8088);
	writel(0x00000a81, 0x021e8090);
	writel(0x0000002b, 0x021e809c);
	writel(0x00013880, 0x021e80b0);
	writel(0x0000047f, 0x021e80a4);
	writel(0x0000c34f, 0x021e80a8);
	writel(0x00000001, 0x021e8080);
}

extern char __dtb_imx6s_riotboard_start[];

ENTRY_FUNCTION(start_imx6s_riotboard, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x4, 0x020e016c);
		setup_uart();
		putc_ll('a');
	}

	fdt = __dtb_imx6s_riotboard_start - get_runtime_offset();
	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
