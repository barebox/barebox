#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx6-regs.h>
#include <io.h>
#include <debug_ll.h>
#include <mach/esdctl.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <image-metadata.h>

#define MX6_UART6_BASE_ADDR             (MX6_AIPS2_OFF_BASE_ADDR + 0x7C000)

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *uart = IOMEM(MX6_UART6_BASE_ADDR);

	imx6_ungate_all_peripherals();

	writel(0x8, iomuxbase + 0x1d4);
	writel(0x1b0b1, iomuxbase + 0x0460);
	writel(0x8, iomuxbase + 0x1d8);
	writel(0x1b0b1, iomuxbase + 0x0460);

	imx6_uart_setup(uart);
	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

static void __noreturn start_imx6_pico_hobbit_common(uint32_t size,
						bool do_early_uart_config,
						void *fdt_blob_fixed_offset)
{
	void *fdt;

	imx6ul_cpu_lowlevel_init();

	arm_setup_stack(0x00910000 - 8);

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();
	barrier();

	if (do_early_uart_config && IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	/* disable all watchdog powerdown counters */
	writew(0x0, 0x020bc008);
	writew(0x0, 0x020c0008);
	writew(0x0, 0x021e4008);

	fdt = fdt_blob_fixed_offset + get_runtime_offset();

	imx6ul_barebox_entry(fdt);
}

BAREBOX_IMD_TAG_STRING(pico_hobbit_mx6_memsize_SZ_256M, IMD_TYPE_PARAMETER, "memsize=256", 0);
BAREBOX_IMD_TAG_STRING(pico_hobbit_mx6_memsize_SZ_512M, IMD_TYPE_PARAMETER, "memsize=512", 0);

#define EDM1_ENTRY(name, fdt_name, memory_size, do_early_uart_config)	\
	ENTRY_FUNCTION(name, r0, r1, r2)				\
	{								\
		extern char __dtb_##fdt_name##_start[];			\
									\
		IMD_USED(pico_hobbit_mx6_memsize_##memory_size);		\
									\
		start_imx6_pico_hobbit_common(memory_size, do_early_uart_config, \
					 __dtb_##fdt_name##_start);	\
	}

EDM1_ENTRY(start_imx6ul_pico_hobbit_256mb, imx6ul_pico_hobbit, SZ_256M, true);
EDM1_ENTRY(start_imx6ul_pico_hobbit_512mb, imx6ul_pico_hobbit, SZ_512M, true);
