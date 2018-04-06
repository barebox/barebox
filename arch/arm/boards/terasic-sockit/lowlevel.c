#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/generic.h>
#include <debug_ll.h>
#include <asm/cache.h>
#include "sdram_config.h"
#include <mach/cyclone5-sdram-config.h>
#include "pinmux_config.c"
#include "pll_config.h"
#include <mach/pll_config.h>
#include "sequencer_defines.h"
#include "sequencer_auto.h"
#include <mach/cyclone5-sequencer.c>
#include "sequencer_auto_inst_init.c"
#include "sequencer_auto_ac_init.c"
#include "iocsr_config_cyclone5.c"

static inline void ledon(int led)
{
	u32 val;

	val = readl(0xFF709000);
	val |= 1 << (led + 24);
	writel(val, 0xFF709000);

	val = readl(0xFF709004);
	val |= 1 << (led + 24);
	writel(val, 0xFF709004);
}

static inline void ledoff(int led)
{
	u32 val;

	val = readl(0xFF709000);
	val &= ~(1 << (led + 24));
	writel(val, 0xFF709000);

	val = readl(0xFF709004);
	val &= ~(1 << (led + 24));
	writel(val, 0xFF709004);
}

extern char __dtb_socfpga_cyclone5_sockit_start[];

ENTRY_FUNCTION(start_socfpga_sockit, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_socfpga_cyclone5_sockit_start + get_runtime_offset();

	barebox_arm_entry(0x0, SZ_1G, fdt);
}

static noinline void sockit_entry(void)
{
	struct socfpga_io_config io_config;
	int ret;

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();

	io_config.pinmux = sys_mgr_init_table;
	io_config.num_pin = ARRAY_SIZE(sys_mgr_init_table);
	io_config.iocsr_emac_mixed2 = iocsr_scan_chain0_table;
	io_config.iocsr_mixed1_flash = iocsr_scan_chain1_table;
	io_config.iocsr_general = iocsr_scan_chain2_table;
	io_config.iocsr_ddr = iocsr_scan_chain3_table;

	socfpga_lowlevel_init(&cm_default_cfg, &io_config);

	puts_ll("lowlevel init done\n");
	puts_ll("SDRAM setup...\n");

	socfpga_sdram_mmr_init();

	puts_ll("SDRAM calibration...\n");

	ret = socfpga_mem_calibration();
	if (!ret)
		hang();

	puts_ll("done\n");

	barebox_arm_entry(0x0, SZ_1G, NULL);
}

ENTRY_FUNCTION(start_socfpga_sockit_xload, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(0xffff0000 + SZ_64K - SZ_4K - 16);

	sockit_entry();
}
