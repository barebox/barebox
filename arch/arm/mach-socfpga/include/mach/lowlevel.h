#ifndef __MACH_SOCFPGA_LOWLEVEL_H
#define __MACH_SOCFPGA_LOWLEVEL_H

#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/generic.h>
#include <debug_ll.h>
#include <asm/cache.h>
#include <mach/cyclone5-sdram-config.h>
#include <mach/pll_config.h>
#include <mach/cyclone5-sequencer.c>

static noinline void SECT(start_socfpga_c5_common)(uint32_t size, void *fdt_blob)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = fdt_blob + get_runtime_offset();

	barebox_arm_entry(0x0, size, fdt);
}

#define SOCFPGA_C5_ENTRY(name, fdt_name, memory_size)			\
	ENTRY_FUNCTION(name, r0, r1, r2)				\
	{								\
		extern char __dtb_##fdt_name##_start[];			\
									\
		start_socfpga_c5_common(memory_size, __dtb_##fdt_name##_start); \
	}

static noinline void SECT(start_socfpga_c5_xload_common)(uint32_t size)
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

	barebox_arm_entry(0x0, size, NULL);
}

#define SOCFPGA_C5_XLOAD_ENTRY(name, memory_size)				\
	ENTRY_FUNCTION(name, r0, r1, r2)				\
	{								\
		arm_cpu_lowlevel_init();				\
									\
		arm_setup_stack(0xffff0000 + SZ_64K - SZ_4K - 16);	\
									\
		start_socfpga_c5_xload_common(memory_size);		\
	}

#endif
