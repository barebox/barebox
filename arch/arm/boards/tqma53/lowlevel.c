#include <common.h>
#include <debug_ll.h>
#include <io.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx5.h>
#include <mach/imx53-regs.h>
#include <mach/generic.h>
#include <image-metadata.h>

extern char __dtb_imx53_mba53_start[];

static inline void setup_uart(void __iomem *base)
{
	/* Enable UART for lowlevel debugging purposes */
	writel(0x00000000, base + 0x80);
	writel(0x00004027, base + 0x84);
	writel(0x00000704, base + 0x88);
	writel(0x00000a81, base + 0x90);
	writel(0x0000002b, base + 0x9c);
	writel(0x0001046a, base + 0xb0);
	writel(0x0000047f, base + 0xa4);
	writel(0x0000a2c1, base + 0xa8);
	writel(0x00000001, base + 0x80);
}

static void __noreturn start_imx53_tqma53_common(void *fdt)
{
	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x278);
		writel(0x3, MX53_IOMUXC_BASE_ADDR + 0x27c);
		setup_uart((void *)MX53_UART2_BASE_ADDR);
		putc_ll('>');
	}

	imx53_barebox_entry(fdt);
}

BAREBOX_IMD_TAG_STRING(tqma53_memsize_512M, IMD_TYPE_PARAMETER, "memsize=512", 0);

ENTRY_FUNCTION(start_imx53_mba53_512mib, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();

	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);

	IMD_USED(tqma53_memsize_512M);

	imx53_init_lowlevel_early(800);

	fdt = __dtb_imx53_mba53_start + get_runtime_offset();

	start_imx53_tqma53_common(fdt);
}

BAREBOX_IMD_TAG_STRING(tqma53_memsize_1G, IMD_TYPE_PARAMETER, "memsize=1024", 0);

ENTRY_FUNCTION(start_imx53_mba53_1gib, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();

	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);

	IMD_USED(tqma53_memsize_1G);

	imx53_init_lowlevel_early(800);

	fdt = __dtb_imx53_mba53_start + get_runtime_offset();

	start_imx53_tqma53_common(fdt);
}
