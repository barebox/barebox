#include <asm/barebox-arm.h>
#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>

extern char __dtb_imx6dl_hummingboard_start[];
extern char __dtb_imx6q_hummingboard_start[];
extern char __dtb_imx6dl_hummingboard2_start[];
extern char __dtb_imx6q_hummingboard2_start[];
extern char __dtb_imx6q_h100_start[];

ENTRY_FUNCTION(start_hummingboard_microsom_i1, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_hummingboard_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_hummingboard_microsom_i2, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_hummingboard_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_hummingboard_microsom_i2ex, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_hummingboard_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_hummingboard_microsom_i4, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_hummingboard_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_hummingboard2_microsom_i1, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_hummingboard2_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_hummingboard2_microsom_i2, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_hummingboard2_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_hummingboard2_microsom_i2ex, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_hummingboard2_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_hummingboard2_microsom_i4, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_hummingboard2_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_h100_microsom_i2ex, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_h100_start + get_runtime_offset();
	imx6q_barebox_entry(fdt);
}
