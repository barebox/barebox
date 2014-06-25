#include <common.h>
#include <sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_imx6q_sabrelite_start[];

ENTRY_FUNCTION(start_imx6q_sabrelite, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_sabrelite_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}

extern char __dtb_imx6dl_sabrelite_start[];

ENTRY_FUNCTION(start_imx6dl_sabrelite, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_sabrelite_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
