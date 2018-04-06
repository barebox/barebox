#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx5.h>

extern char __dtb_imx51_genesi_efika_sb_start[];

ENTRY_FUNCTION(start_imx51_genesi_efikasb, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(0x20000000 - 16);
	imx51_init_lowlevel(800);

	fdt = __dtb_imx51_genesi_efika_sb_start + get_runtime_offset();

	imx51_barebox_entry(fdt);
}
