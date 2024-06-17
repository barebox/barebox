// SPDX-License-Identifier: GPL-2.0-only

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
#include <mach/socfpga/arria10-sdram.h>
#include <mach/socfpga/arria10-regs.h>
#include <mach/socfpga/arria10-reset-manager.h>
#include <mach/socfpga/arria10-clock-manager.h>
#include <mach/socfpga/arria10-pinmux.h>
#include <mach/socfpga/arria10-fpga.h>
#include <mach/socfpga/init.h>
#include "pll-config-arria10.c"
#include "pinmux-config-arria10.c"
#include <mach/socfpga/generic.h>
#include <mach/socfpga/init.h>

#define BAREBOX_PART 0
// the bitstream is located in the second partition in the partition table
#define BITSTREAM_PART 1
#define BAREBOX1_OFFSET    SZ_1M
#define BAREBOX2_OFFSET    (BAREBOX1_OFFSET + SZ_1M)
// Offset from the start of the second partition on the eMMC.
#define BITSTREAM1_OFFSET  0x0
#define BITSTREAM2_OFFSET  (BITSTREAM1_OFFSET + SZ_32M)

extern char __dtb_z_socfpga_arria10_mercury_aa1_start[];

#define ARRIA10_STACKTOP   (ARRIA10_OCRAM_ADDR + SZ_256K)

ENTRY_FUNCTION_WITHSTACK(start_socfpga_aa1_xload, ARRIA10_STACKTOP, r0, r1, r2)
{
	int pbl_index = 0;
	int barebox = 0;
	int bitstream = 0;

	arm_cpu_lowlevel_init();
	arria10_cpu_lowlevel_init();

	relocate_to_current_adr();

	setup_c();

	arria10_init(&mainpll_cfg, &perpll_cfg, pinmux);

	arria10_prepare_mmc(BAREBOX_PART, BITSTREAM_PART);

	pbl_index = readl(ARRIA10_SYSMGR_ROM_INITSWLASTLD);

	/* Allow booting from both PBL0 and PBL1 to allow atomic updates.
	 * Bitstreams redundant too and expected to reside in the second
	 * partition.
	 * There is a fixed relation between the PBL/barebox instance and its
	 * bitstream location (offset) that requires to update them together */
	switch (pbl_index) {
	case 0:
		barebox = BAREBOX1_OFFSET;
		bitstream = BITSTREAM1_OFFSET;
		break;
	case 1:
		barebox = BAREBOX2_OFFSET;
		bitstream = BITSTREAM2_OFFSET;
		break;
	case 2:
	case 3:
		/* Left blank for future extension */
		break;
	default:
		/* If we get an undefined pbl index, use the first and hope for the best.
		 * We could bail out, but user wouldn't see anything on the console
		 * and wouldn't know what happend anyway. */
		barebox = BAREBOX1_OFFSET;
		bitstream = BITSTREAM1_OFFSET;
		break;
	}

	arria10_load_fpga(bitstream, SZ_32M);

	arria10_finish_io(&mainpll_cfg, &perpll_cfg, pinmux);

	arria10_ddr_calibration_sequence();

	arria10_start_image(barebox);
}

ENTRY_FUNCTION(start_socfpga_aa1, r0, r1, r2)
{
	void *fdt;

	fdt = __dtb_z_socfpga_arria10_mercury_aa1_start + get_runtime_offset();

	barebox_arm_entry(0x0, SZ_2G, fdt);
}

ENTRY_FUNCTION_WITHSTACK(start_socfpga_aa1_bringup, ARRIA10_STACKTOP, r0, r1, r2)
{
	void *fdt;

	arria10_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	arria10_init(&mainpll_cfg, &perpll_cfg, pinmux);

	/* wait for fpga_usermode */
	a10_wait_for_usermode(0x1000000);

	arria10_finish_io(&mainpll_cfg, &perpll_cfg, pinmux);

	arria10_ddr_calibration_sequence();

	fdt = __dtb_z_socfpga_arria10_mercury_aa1_start + get_runtime_offset();

	barebox_arm_entry(0x0, SZ_2G, fdt);
}
