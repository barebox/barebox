#include <common.h>
#include <sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/cache.h>
#include <mach/generic.h>
#include <debug_ll.h>
#include "sdram_config.h"
#include <mach/sdram_config.h>
#include "pinmux_config.c"
#include "pll_config.h"
#include <mach/pll_config.h>
#include "sequencer_defines.h"
#include "sequencer_auto.h"
#include <mach/sequencer.c>
#include "sequencer_auto_inst_init.c"
#include "sequencer_auto_ac_init.c"

static inline void ledon(void)
{
	u32 val;

	val = readl(0xFF708000);
	val &= ~(1 << 28);
	writel(val, 0xFF708000);

	val = readl(0xFF708004);
	val |= 1 << 28;
	writel(val, 0xFF708004);
}

static inline void ledoff(void)
{
	u32 val;

	val = readl(0xFF708000);
	val |= 1 << 28;
	writel(val, 0xFF708000);

	val = readl(0xFF708004);
	val |= 1 << 28;
	writel(val, 0xFF708004);
}

extern char __dtb_socfpga_cyclone5_socrates_start[];

ENTRY_FUNCTION(start_socfpga_socrates)(void)
{
	uint32_t fdt;

	__barebox_arm_head();

	arm_cpu_lowlevel_init();

	fdt = (uint32_t)__dtb_socfpga_cyclone5_socrates_start - get_runtime_offset();

	barebox_arm_entry(0x0, SZ_1G, fdt);
}

static noinline void socrates_entry(void)
{
	int ret;

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();

	socfpga_lowlevel_init(&cm_default_cfg,
			sys_mgr_init_table, ARRAY_SIZE(sys_mgr_init_table));

	puts_ll("lowlevel init done\n");
	puts_ll("SDRAM setup...\n");

	socfpga_sdram_mmr_init();

	puts_ll("SDRAM calibration...\n");

	ret = socfpga_sdram_calibration(inst_rom_init, inst_rom_init_size,
				ac_rom_init, ac_rom_init_size);
	if (ret)
		hang();

	puts_ll("done\n");

	barebox_arm_entry(0x0, SZ_1G, 0);
}

ENTRY_FUNCTION(start_socfpga_socrates_xload)(void)
{
	__barebox_arm_head();

	arm_cpu_lowlevel_init();

	arm_setup_stack(0xffff0000 + SZ_64K - SZ_4K - 16);

	socrates_entry();
}
