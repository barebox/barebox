// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <mach/generic.h>
#include <asm/barebox-arm.h>
#include <mach/esdctl.h>
#include <asm/cache.h>


extern char __dtb_z_imx6ul_ccimx6ulsbcpro_start[];

ENTRY_FUNCTION(start_imx6ul_ccimx6ulsbcpro, r0, r1, r2)
{
	void *fdt;

	imx6ul_cpu_lowlevel_init();

	arm_setup_stack(0x00910000);

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();
	barrier();

	/* disable all watchdog powerdown counters */
	writew(0x0, 0x020bc008);
	writew(0x0, 0x020c0008);
	writew(0x0, 0x021e4008);

	fdt = __dtb_z_imx6ul_ccimx6ulsbcpro_start;

	imx6ul_barebox_entry(fdt);
}
