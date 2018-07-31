#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <memory.h>
#include <asm/barebox-arm.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/unaligned.h>
#include <debug_ll.h>
#include <pbl.h>
#include <mach/arria10-sdram.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-reset-manager.h>
#include <mach/arria10-clock-manager.h>
#include <mach/arria10-pinmux.h>
#include <mach/arria10-fpga.h>
#include "pll-config-arria10.c"
#include "pinmux-config-arria10.c"
#include <mach/generic.h>

#define BAREBOX_PART 0
#define BITSTREAM_PART 1
#define BAREBOX1_OFFSET    SZ_1M
#define BAREBOX2_OFFSET    BAREBOX1_OFFSET + SZ_512K
#define BAREBOX3_OFFSET    BAREBOX2_OFFSET + SZ_512K
#define BAREBOX4_OFFSET    BAREBOX3_OFFSET + SZ_512K
#define BITSTREAM1_OFFSET  0x0
#define BITSTREAM2_OFFSET  BITSTREAM1_OFFSET + SZ_64M

extern char __dtb_socfpga_arria10_achilles_start[];

static noinline void achilles_start(void)
{
	int pbl_index = 0;
	int barebox = 0;
	int bitstream = 0;

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();

	arria10_init(&mainpll_cfg, &perpll_cfg, pinmux);

	arria10_prepare_mmc(BAREBOX_PART, BITSTREAM_PART);

	pbl_index = readl(ARRIA10_SYSMGR_ROM_INITSWLASTLD);

	switch (pbl_index) {
	case 0:
		barebox = BAREBOX1_OFFSET;
		bitstream = BITSTREAM1_OFFSET;
		break;
	case 1:
		barebox = BAREBOX2_OFFSET;
		bitstream = BITSTREAM1_OFFSET;
		break;
	case 2:
		barebox = BAREBOX3_OFFSET;
		bitstream = BITSTREAM2_OFFSET;
		break;
	case 3:
		barebox = BAREBOX4_OFFSET;
		bitstream = BITSTREAM2_OFFSET;
		break;
	}

	arria10_load_fpga(bitstream, SZ_64M);

	arria10_finish_io(&mainpll_cfg, &perpll_cfg, pinmux);

	arria10_ddr_calibration_sequence();

	arria10_start_image(barebox);
}

ENTRY_FUNCTION(start_socfpga_achilles, r0, r1, r2)
{
	void *fdt;

	if (get_pc() > ARRIA10_OCRAM_ADDR) {
		arm_cpu_lowlevel_init();

		arm_setup_stack(ARRIA10_OCRAM_ADDR + SZ_256K - 32);

		achilles_start();
	}

	fdt = __dtb_socfpga_arria10_achilles_start + get_runtime_offset();

	barebox_arm_entry(0x0, SZ_2G + SZ_1G, fdt);
}

ENTRY_FUNCTION(start_socfpga_achilles_bringup, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	arm_setup_stack(ARRIA10_OCRAM_ADDR + SZ_256K - 16);

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();

	arria10_init(&mainpll_cfg, &perpll_cfg, pinmux);

	/* wait for fpga_usermode */
	a10_wait_for_usermode(0x1000000);

	arria10_finish_io(&mainpll_cfg, &perpll_cfg, pinmux);

	arria10_ddr_calibration_sequence();

	fdt = __dtb_socfpga_arria10_achilles_start + get_runtime_offset();

	barebox_arm_entry(0x0, SZ_2G + SZ_1G, fdt);
}
