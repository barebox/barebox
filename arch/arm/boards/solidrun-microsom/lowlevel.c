#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_imx6dl_hummingboard_start[];
extern char __dtb_imx6q_hummingboard_start[];

ENTRY_FUNCTION(start_hummingboard_microsom_i1, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_hummingboard_start - get_runtime_offset();
	barebox_arm_entry(0x10000000, SZ_512M, fdt);
}

ENTRY_FUNCTION(start_hummingboard_microsom_i2, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_hummingboard_start - get_runtime_offset();
	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}

ENTRY_FUNCTION(start_hummingboard_microsom_i2ex, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_hummingboard_start - get_runtime_offset();
	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}

ENTRY_FUNCTION(start_hummingboard_microsom_i4, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_hummingboard_start - get_runtime_offset();
	barebox_arm_entry(0x10000000, SZ_2G, fdt);
}
