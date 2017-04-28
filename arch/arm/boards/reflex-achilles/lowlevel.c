#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/cache.h>
#include <debug_ll.h>
#include <mach/arria10-sdram.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-reset-manager.h>
#include <mach/arria10-clock-manager.h>
#include <mach/arria10-pinmux.h>
#include "pll-config-arria10.c"
#include "pinmux-config-arria10.c"
#include <mach/generic.h>

extern char __dtb_socfpga_arria10_achilles_start[];

static noinline void achilles_entry(void)
{
	void *fdt;

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();

	arria10_init(&mainpll_cfg, &perpll_cfg, pinmux);

	puts_ll("lowlevel init done\n");

	arria10_ddr_calibration_sequence();

	puts_ll("SDRAM setup done\n");

	fdt = __dtb_socfpga_arria10_achilles_start - get_runtime_offset();

	barebox_arm_entry(0x0, SZ_2G + SZ_1G, fdt);
}

ENTRY_FUNCTION(start_socfpga_achilles, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(0xffe00000 + SZ_256K - SZ_32K - SZ_4K - 16);

	achilles_entry();
}
