#include <common.h>
#include <sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx6-regs.h>
#include <debug_ll.h>

static inline void setup_uart(void)
{
	void __iomem *uartbase = (void *)MX6_UART2_BASE_ADDR;
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	writel(0x1, iomuxbase + 0x2b0);

	writel(0x00000000, uartbase + 0x80);
	writel(0x00004027, uartbase + 0x84);
	writel(0x00000704, uartbase + 0x88);
	writel(0x00000a81, uartbase + 0x90);
	writel(0x0000002b, uartbase + 0x9c);
	writel(0x00013880, uartbase + 0xb0);
	writel(0x0000047f, uartbase + 0xa4);
	writel(0x0000c34f, uartbase + 0xa8);
	writel(0x00000001, uartbase + 0x80);

	putc_ll('>');
}

extern char __dtb_imx6q_guf_santaro_start[];

ENTRY_FUNCTION(start_imx6q_guf_santaro, r0, r1, r2)
{
	uint32_t fdt;
	int i;

	arm_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	for (i = 0x68; i <= 0x80; i += 4)
		writel(0xffffffff, MX6_CCM_BASE_ADDR + i);

	setup_uart();

	fdt = (uint32_t)__dtb_imx6q_guf_santaro_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
