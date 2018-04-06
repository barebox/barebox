#include <common.h>
#include <mach/generic.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm.h>

extern char __dtb_imx6q_nitrogen6x_start[];

ENTRY_FUNCTION(start_imx6q_nitrogen6x_1g, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_nitrogen6x_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6q_nitrogen6x_2g, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_nitrogen6x_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

extern char __dtb_imx6dl_nitrogen6x_start[];

ENTRY_FUNCTION(start_imx6dl_nitrogen6x_1g, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_nitrogen6x_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_nitrogen6x_2g, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6dl_nitrogen6x_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

extern char __dtb_imx6qp_nitrogen6_max_start[];

ENTRY_FUNCTION(start_imx6qp_nitrogen6_max, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6qp_nitrogen6_max_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}
